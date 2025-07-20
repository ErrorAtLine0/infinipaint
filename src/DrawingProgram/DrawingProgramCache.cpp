#include "DrawingProgramCache.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include <chrono>

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif

#define MAXIMUM_DRAW_CACHE_SURFACES 32

DrawingProgramCache::DrawingProgramCache(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

void DrawingProgramCache::build(std::vector<CollabListType::ObjectInfoPtr> components) {
    bvhRoot = BVHNode();
    unsortedComponents.clear();
    nodesWithCachedSurfaces.clear();
    std::erase_if(components, [&](auto& c) {
        if(!c->obj->worldAABB || c->obj->updateDraw) {
            unsortedComponents.emplace_back(c);
            return true;
        }
        return false;
    });
    build_bvh_node(bvhRoot, components);
}

void DrawingProgramCache::update() {
    if(unsortedComponents.size() >= 750)
        build(drawP.components.client_list());
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

    build_bvh_node_coords_and_resolution(node);

    if(components.size() < 100) {
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

void DrawingProgramCache::build_bvh_node_coords_and_resolution(BVHNode& node) {
    // These are part of the draw cache data, but we're putting it out here, because:
    //  - We dont have to recalculate it every time we refresh the cache
    //  - Some of the data here is needed to determine whether to generate the cache in the first place

    const int MAX_DIMENSION_RESOLUTION = 2000;

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

void DrawingProgramCache::refresh_all_draw_cache(SkCanvas* canvas, const DrawData& drawData) {
    std::deque<BVHNode*> nodeFlatList; // We want to render children before parents, so that parents can make use of the cached children
    traverse_bvh_run_function(drawData.cam.viewingAreaGenerousCollider, [&](DrawingProgramCache::BVHNode* node, const std::vector<CollabListType::ObjectInfoPtr>& comps) {
        if(node && node->coords.inverseScale <= drawData.cam.c.inverseScale) {
            if(!node->drawCache)
                nodeFlatList.emplace_front(node);
            return false;
        }
        return true;
    });
    if(!nodeFlatList.empty())
        refresh_draw_cache(*nodeFlatList.front(), drawData);
}

void DrawingProgramCache::refresh_draw_cache(BVHNode& bvhNode, const DrawData& drawData) {
    if(bvhNode.children.empty() && bvhNode.components.empty())
        return;

    bvhNode.drawCache = std::nullopt;
    BVHNode::DrawCacheData drawCache;

    SkImageInfo imgInfo = SkImageInfo::MakeN32Premul(bvhNode.resolution.x(), bvhNode.resolution.y());
    #ifdef USE_SKIA_BACKEND_GRAPHITE
        drawCache.surface = SkSurfaces::RenderTarget(drawP.world.main.window.recorder(), imgInfo, skgpu::Mipmapped::kNo, drawP.world.main.window.defaultMSAASurfaceProps);
    #elif USE_SKIA_BACKEND_GANESH
        drawCache.surface = SkSurfaces::RenderTarget(drawP.world.main.window.ctx.get(), skgpu::Budgeted::kNo, imgInfo, drawP.world.main.window.defaultMSAASampleCount, &drawP.world.main.window.defaultMSAASurfaceProps);
    #endif

    if(!drawCache.surface)
        throw std::runtime_error("[DrawingProgramCache::refresh_draw_cache] Could not make cache surface");

    SkCanvas* cacheCanvas = drawCache.surface->getCanvas();

    cacheCanvas->clear(SkColor4f{0, 0, 0, 0});

    DrawData cacheDrawData = drawData;
    cacheDrawData.cam.c = bvhNode.coords;
    cacheDrawData.cam.changed = nullptr;
    cacheDrawData.cam.set_viewing_area(bvhNode.resolution.cast<float>());
    cacheDrawData.refresh_draw_optimizing_values();
    drawP.draw_components_to_canvas(cacheCanvas, cacheDrawData, false);

    drawCache.lastDrawnComponentPlacement = drawP.components.client_list().size() - 1;
    drawCache.lastRenderTime = std::chrono::steady_clock::now();

    bvhNode.drawCache = drawCache; // Set the drawCache after rendering is done, so that the draw function doesnt assume we have this node cached

    while(nodesWithCachedSurfaces.size() >= MAXIMUM_DRAW_CACHE_SURFACES) {
        BVHNode* leastUsedNode = *nodesWithCachedSurfaces.begin();
        for(BVHNode* n : nodesWithCachedSurfaces) {
            if(n->drawCache.value().lastRenderTime < leastUsedNode->drawCache.value().lastRenderTime)
                leastUsedNode = n;
        }
        nodesWithCachedSurfaces.erase(leastUsedNode);
    }

    nodesWithCachedSurfaces.emplace(&bvhNode);
}
