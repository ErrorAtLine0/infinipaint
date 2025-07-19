#include "DrawingProgramCache.hpp"

void DrawingProgramCache::build(std::vector<CollabListType::ObjectInfoPtr> components) {
    bvhRoot = BVHNode();
    unsortedComponents.clear();
    std::erase_if(components, [&](auto& c) {
        if(!c->obj->worldAABB || c->obj->updateDraw) {
            unsortedComponents.emplace_back(c);
            return true;
        }
        return false;
    });
    build_bvh_node(bvhRoot, components);
}

void DrawingProgramCache::update(const std::vector<CollabListType::ObjectInfoPtr>& components) {
    if(unsortedComponents.size() >= 1)
        build(components);
}

void DrawingProgramCache::add_component(const CollabListType::ObjectInfoPtr& c) {
    unsortedComponents.emplace_back(c);
    invalidate_cache_before_pos(c->pos);
}

void DrawingProgramCache::erase_component(const CollabListType::ObjectInfoPtr& c) {
    auto it = std::find(unsortedComponents.begin(), unsortedComponents.end(), c);
    if(it != unsortedComponents.end())
        unsortedComponents.erase(it);
    else {
        if(!c->obj->worldAABB.has_value())
            return;
        auto bvhNodePair = get_bvh_node_fully_containing_recursive(bvhRoot, c->obj.get());
        if(bvhNodePair.first)
            bvhNodePair.first->components.erase(bvhNodePair.second);
    }
    invalidate_cache_before_pos(c->pos);
}

std::pair<DrawingProgramCache::BVHNode*, std::vector<DrawingProgramCache::CollabListType::ObjectInfoPtr>::iterator> DrawingProgramCache::get_bvh_node_fully_containing_recursive(BVHNode& bvhNode, DrawComponent* c) {
    SCollision::AABB<WorldScalar>& cAABB = c->worldAABB.value();
    if(!bvhNode.bounds.fully_contains_aabb(cAABB))
        return {nullptr, bvhNode.components.end()};
    auto it = std::find_if(bvhNode.components.begin(), bvhNode.components.end(), [&](const auto& comp) {
        return comp->obj.get() == c;
    });
    if(it != bvhNode.components.end())
        return {&bvhNode, it};
    for(auto& p : bvhNode.children) {
        auto toRet = get_bvh_node_fully_containing_recursive(p, c);
        if(toRet.first)
            return toRet;
    }
    return {nullptr, bvhNode.components.end()};
}

void DrawingProgramCache::traverse_bvh_run_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(BVHNode* node, const std::vector<CollabListType::ObjectInfoPtr>& components)> f) {
    f(nullptr, unsortedComponents);
    traverse_bvh_run_function_recursive(bvhRoot, aabb, f);
}

void DrawingProgramCache::traverse_bvh_run_function_recursive(BVHNode& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(BVHNode* node, const std::vector<CollabListType::ObjectInfoPtr>& components)> f) {
    if(SCollision::collide(aabb, bvhNode.bounds) && f(&bvhNode, bvhNode.components)) {
        for(auto& p : bvhNode.children)
            traverse_bvh_run_function_recursive(p, aabb, f);
    }
}

void DrawingProgramCache::preupdate_component(DrawComponent* c) {
    auto it = std::find_if(unsortedComponents.begin(), unsortedComponents.end(), [&](const auto& comp) {
        return comp->obj.get() == c;
    });
    if(it != unsortedComponents.end()) {
        invalidate_cache_before_pos((*it)->pos);
        return;
    }
    else {
        if(!c->worldAABB.has_value())
            return;
        auto bvhNodePair = get_bvh_node_fully_containing_recursive(bvhRoot, c);
        if(bvhNodePair.first) {
            unsortedComponents.emplace_back(*bvhNodePair.second);
            invalidate_cache_before_pos((*bvhNodePair.second)->pos);
            bvhNodePair.first->components.erase(bvhNodePair.second);
        }
    }
}

void DrawingProgramCache::build_bvh_node(BVHNode& node, std::vector<CollabListType::ObjectInfoPtr> components) {
    if(components.empty())
        return;

    node.bounds = components.front()->obj->worldAABB.value();
    for(auto& c : components)
        node.bounds.include_aabb_in_bounds(c->obj->worldAABB.value());

    if(components.size() < 50) {
        node.components = components;
        return;
    }

    WorldVec boundsCenter = node.bounds.center();

    std::array<std::vector<CollabListType::ObjectInfoPtr>, 4> parts;

    for(auto& c : components) {
        const auto& cAABB = c->obj->worldAABB.value();
        if(cAABB.min.x() < boundsCenter.x() && cAABB.max.x() < boundsCenter.x() && cAABB.min.y() < boundsCenter.y() && cAABB.max.y() < boundsCenter.y())
            parts[0].emplace_back(c);
        else if(cAABB.min.x() > boundsCenter.x() && cAABB.max.x() > boundsCenter.x() && cAABB.min.y() < boundsCenter.y() && cAABB.max.y() < boundsCenter.y())
            parts[1].emplace_back(c);
        else if(cAABB.min.x() < boundsCenter.x() && cAABB.max.x() < boundsCenter.x() && cAABB.min.y() > boundsCenter.y() && cAABB.max.y() > boundsCenter.y())
            parts[2].emplace_back(c);
        else if(cAABB.min.x() > boundsCenter.x() && cAABB.max.x() > boundsCenter.x() && cAABB.min.y() > boundsCenter.y() && cAABB.max.y() > boundsCenter.y())
            parts[3].emplace_back(c);
        else
            node.components.emplace_back(c);
    }

    for(auto& p : parts) {
        if(!p.empty())
            build_bvh_node(node.children.emplace_back(), p);
    }
}

void DrawingProgramCache::invalidate_cache_before_pos(uint64_t placementToInvalidateAt) {
    // We could use worldAABB of component to invalidate specific areas. However, that comes with some issues:
    // - The AABB couldve been changed from the last frame, so the current worldAABB doesnt actually represent what we should update
    // - The worldAABB might not be calculated yet, or mightve been invalidated (worldAABB = nullopt)
    // For now, we will ignore worldAABB and just clear cache by placement in list

    std::erase_if(nodesWithCachedSurfaces, [&](auto& n) {
        if(placementToInvalidateAt <= n->drawCache.value().lastDrawnComponentPlacement) {
            n->drawCache = std::nullopt;
            return true;
        }
        return false;
    });
}
