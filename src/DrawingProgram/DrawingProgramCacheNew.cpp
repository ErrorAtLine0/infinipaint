#include "DrawingProgramCacheNew.hpp"
#include "DrawingProgram.hpp"

std::unordered_set<std::shared_ptr<DrawingProgramCacheNew::BVHNode>> DrawingProgramCacheNew::nodesWithCachedSurfaces;

DrawingProgramCacheNew::DrawingProgramCacheNew(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

void DrawingProgramCacheNew::add_component(const CanvasComponentContainer::ObjInfoSharedPtr& c) {
    unsortedComponents.emplace_back(c);
    invalidate_cache_at_aabb(c->obj->get_world_bounds());
}

void DrawingProgramCacheNew::erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& c) {
    auto it = objToBVHNode.find(c);
    if(it != objToBVHNode.end())
        objToBVHNode.erase(it);
    else
        std::erase(unsortedComponents, c);
    invalidate_cache_at_aabb(c->obj->get_world_bounds());
}

void DrawingProgramCacheNew::invalidate_cache_at_aabb(const SCollision::AABB<WorldScalar>& aabb) {
    std::for_each(nodesWithCachedSurfaces.begin(), nodesWithCachedSurfaces.end(), [&](auto& n) {
        if(n->drawCache.value().attachedCache == this && SCollision::collide(aabb, n->bounds)) {
            auto& dCache = n->drawCache.value();
            if(dCache.invalidBounds.has_value()) {
                auto& iBounds = dCache.invalidBounds.value();
                iBounds.include_aabb_in_bounds(aabb);
            }
            else
                dCache.invalidBounds = aabb;
        }
    });
}

bool DrawingProgramCacheNew::should_rebuild() const {
    return (unsortedComponents.size() >= MINIMUM_COMPONENTS_TO_START_REBUILD);
}

void DrawingProgramCacheNew::build(const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToExclude) {
    internal_build(drawP.layerMan.get_flattened_component_list(), objsToExclude);
}

void DrawingProgramCacheNew::internal_build(std::vector<CanvasComponentContainer::ObjInfoSharedPtr> componentsToBuild, const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToNotInclude) {
    std::erase_if(componentsToBuild, [&objsToNotInclude](auto& c) {
        return objsToNotInclude.contains(c);
    });
    bvhRoot = std::make_shared<BVHNode>();
    unsortedComponents.clear();
    clear_own_cached_surfaces();
    build_bvh_node(bvhRoot, componentsToBuild);
}

void DrawingProgramCacheNew::clear_own_cached_surfaces() {
    std::erase_if(nodesWithCachedSurfaces, [&](auto& n) {
        if(n->drawCache.value().attachedCache == this) {
            n->drawCache = std::nullopt;
            return true;
        }
        return false;
    });
}

void DrawingProgramCacheNew::preupdate_component(const CanvasComponentContainer::ObjInfoSharedPtr& c) {
    auto it = objToBVHNode.find(c);
    if(it != objToBVHNode.end()) {
        unsortedComponents.emplace_back(c);
        std::erase(it->second->components, c);
    }
    invalidate_cache_at_aabb(c->obj->get_world_bounds());
}

void DrawingProgramCacheNew::build_bvh_node(const std::shared_ptr<BVHNode>& bvhNode, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components) {
    if(components.empty())
        return;

    bvhNode->bounds = components.front()->obj->get_world_bounds();
    for(auto& c : components)
        bvhNode->bounds.include_aabb_in_bounds(c->obj->get_world_bounds());

    build_bvh_node_coords_and_resolution(*bvhNode);

    if(components.size() < MAXIMUM_COMPONENTS_IN_SINGLE_NODE) {
        bvhNode->components = components;
        for(auto& c : bvhNode->components)
            objToBVHNode.emplace(c, bvhNode);
        return;
    }

    WorldVec boundsCenter = bvhNode->bounds.center();

    std::array<std::vector<CanvasComponentContainer::ObjInfoSharedPtr>, 4> parts;

    for(auto& c : components) {
        const auto& cAABB = c->obj->get_world_bounds();
        if(cAABB.min.x() < boundsCenter.x() && cAABB.max.x() < boundsCenter.x() && cAABB.min.y() < boundsCenter.y() && cAABB.max.y() < boundsCenter.y())
            parts[0].emplace_back(c);
        else if(cAABB.min.x() > boundsCenter.x() && cAABB.max.x() > boundsCenter.x() && cAABB.min.y() < boundsCenter.y() && cAABB.max.y() < boundsCenter.y())
            parts[1].emplace_back(c);
        else if(cAABB.min.x() < boundsCenter.x() && cAABB.max.x() < boundsCenter.x() && cAABB.min.y() > boundsCenter.y() && cAABB.max.y() > boundsCenter.y())
            parts[2].emplace_back(c);
        else if(cAABB.min.x() > boundsCenter.x() && cAABB.max.x() > boundsCenter.x() && cAABB.min.y() > boundsCenter.y() && cAABB.max.y() > boundsCenter.y())
            parts[3].emplace_back(c);
        else {
            bvhNode->components.emplace_back(c);
            objToBVHNode.emplace(c, bvhNode);
        }
    }

    for(auto& p : parts) {
        if(!p.empty())
            build_bvh_node(bvhNode->children.emplace_back(std::make_shared<BVHNode>()), p);
    }
}

void DrawingProgramCacheNew::build_bvh_node_coords_and_resolution(BVHNode& node) {
    // These are part of the draw cache data, but we're putting it out here, because:
    //  - We dont have to recalculate it every time we refresh the cache
    //  - Some of the data here is needed to determine whether to generate the cache in the first place

    constexpr int MAX_DIMENSION_RESOLUTION = 2048;

    WorldVec cacheBoundDim = node.bounds.dim();
    if(cacheBoundDim.x() > cacheBoundDim.y()) {
        node.resolution.x() = MAX_DIMENSION_RESOLUTION;
        node.resolution.y() = MAX_DIMENSION_RESOLUTION * static_cast<double>(cacheBoundDim.y() / cacheBoundDim.x());
    }
    else {
        node.resolution.y() = MAX_DIMENSION_RESOLUTION;
        node.resolution.x() = MAX_DIMENSION_RESOLUTION * static_cast<double>(cacheBoundDim.x() / cacheBoundDim.y());
    }
    node.coords.rotation = 0.0;
    node.coords.pos = node.bounds.min;
    // Not necessary, but we choose the number that had less operations done on it for more accuracy
    if(node.resolution.x() > node.resolution.y())
        node.coords.inverseScale = cacheBoundDim.x().divide_double(node.resolution.x());
    else 
        node.coords.inverseScale = cacheBoundDim.y().divide_double(node.resolution.y());
}
