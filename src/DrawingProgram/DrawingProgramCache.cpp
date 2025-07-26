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

#define MAXIMUM_DRAW_CACHE_SURFACES 40

std::unordered_set<std::shared_ptr<DrawingProgramCacheBVHNode>> DrawingProgramCache::nodesWithCachedSurfaces;

DrawingProgramCache::DrawingProgramCache(DrawingProgram& initDrawP):
    drawP(initDrawP)
{
    lastBvhBuildTime = std::chrono::steady_clock::now();
}

void DrawingProgramCache::build(std::vector<CollabListType::ObjectInfoPtr> components) {
    bvhRoot = std::make_shared<DrawingProgramCacheBVHNode>();
    unsortedComponents.clear();
    clear_own_cached_surfaces();
    std::erase_if(components, [&](auto& c) {
        if(!c->obj->worldAABB) {
            unsortedComponents.emplace_back(c);
            c->obj->parentBvhNode.reset();
            return true;
        }
        return false;
    });
    build_bvh_node(bvhRoot, components);
    lastBvhBuildTime = std::chrono::steady_clock::now();
}

void DrawingProgramCache::clear() {
    bvhRoot = nullptr;
    unsortedComponents.clear();
    clear_own_cached_surfaces();
}

void DrawingProgramCache::clear_own_cached_surfaces() {
    std::erase_if(nodesWithCachedSurfaces, [&](auto& s) {
        return s->drawCache.value().attachedCache == this;
    });
}

bool DrawingProgramCache::check_if_rebuild_should_occur() {
    return (unsortedComponents.size() >= MINIMUM_COMPONENTS_TO_START_REBUILD && (std::chrono::steady_clock::now() - lastBvhBuildTime) >= std::chrono::seconds(5));
}

void DrawingProgramCache::test_rebuild(const std::vector<CollabListType::ObjectInfoPtr>& comps, bool force) {
    if(force || check_if_rebuild_should_occur())
        force_rebuild(comps);
}

void DrawingProgramCache::test_rebuild_dont_include_set(const std::vector<CollabListType::ObjectInfoPtr>& comps, const std::unordered_set<CollabListType::ObjectInfoPtr>& objsToNotInclude, bool force) {
    if(force || check_if_rebuild_should_occur())
        force_rebuild_dont_include_objs(comps, objsToNotInclude);
}

void DrawingProgramCache::test_rebuild_dont_include_set_dont_include_nodes(const std::vector<CollabListType::ObjectInfoPtr>& comps, const std::unordered_set<CollabListType::ObjectInfoPtr>& objsToNotInclude, const std::unordered_set<std::shared_ptr<DrawingProgramCacheBVHNode>>& nodesToNotInclude, bool force) {
    if(force || check_if_rebuild_should_occur()) {
        auto objsToNotIncludeNew = objsToNotInclude;
        move_components_from_bvh_nodes_to_set(objsToNotIncludeNew, nodesToNotInclude);
        force_rebuild_dont_include_objs(comps, objsToNotIncludeNew);
    }
}

void DrawingProgramCache::move_components_from_bvh_nodes_to_set(std::unordered_set<CollabListType::ObjectInfoPtr>& s, const std::unordered_set<std::shared_ptr<DrawingProgramCacheBVHNode>>& bvhNodes) {
    for(auto& b : bvhNodes)
        move_components_from_bvh_node_to_set(s, b);
}

void DrawingProgramCache::move_components_from_bvh_node_to_set(std::unordered_set<CollabListType::ObjectInfoPtr>& s, const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode) {
    for(auto& c : bvhNode->components)
        s.emplace(c);
    for(auto& p : bvhNode->children)
        move_components_from_bvh_node_to_set(s, p);
}

void DrawingProgramCache::force_rebuild_dont_include_objs(std::vector<CollabListType::ObjectInfoPtr> componentsToBuild, const std::unordered_set<CollabListType::ObjectInfoPtr>& objsToNotInclude) {
    std::erase_if(componentsToBuild, [&objsToNotInclude](auto& c) {
        return objsToNotInclude.contains(c);
    });
    build(componentsToBuild);
}

void DrawingProgramCache::force_rebuild(const std::vector<CollabListType::ObjectInfoPtr>& componentsToBuild) {
    build(componentsToBuild);
}

const std::vector<CollabListType::ObjectInfoPtr>& DrawingProgramCache::get_unsorted_component_list() const {
    return unsortedComponents;
}

const std::chrono::steady_clock::time_point& DrawingProgramCache::get_last_bvh_build_time() const {
    return lastBvhBuildTime;
}

void DrawingProgramCache::add_component(const CollabListType::ObjectInfoPtr& c) {
    unsortedComponents.emplace_back(c);
    if(c->obj->worldAABB)
        invalidate_cache_at_aabb_before_pos(c->obj->worldAABB.value(), c->pos);
    else
        invalidate_cache_before_pos(c->pos);
}

void DrawingProgramCache::erase_component(const CollabListType::ObjectInfoPtr& c) {
    auto parentBvhNode = c->obj->parentBvhNode.lock();
    if(parentBvhNode) {
        std::erase(parentBvhNode->components, c);
        c->obj->parentBvhNode.reset();
    }
    else
        std::erase(unsortedComponents, c);
    invalidate_cache_before_pos(c->pos);
}

std::pair<std::shared_ptr<DrawingProgramCacheBVHNode>, std::vector<CollabListType::ObjectInfoPtr>::iterator> DrawingProgramCache::get_bvh_node_fully_containing_recursive(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, DrawComponent* c) {
    SCollision::AABB<WorldScalar>& cAABB = c->worldAABB.value();
    if(!bvhNode->bounds.fully_contains_aabb(cAABB))
        return {nullptr, bvhNode->components.end()};
    auto it = std::find_if(bvhNode->components.begin(), bvhNode->components.end(), [&](const auto& comp) {
        return comp->obj.get() == c;
    });
    if(it != bvhNode->components.end())
        return {bvhNode, it};
    for(auto& p : bvhNode->children) {
        auto toRet = get_bvh_node_fully_containing_recursive(p, c);
        if(toRet.first)
            return toRet;
    }
    return {nullptr, bvhNode->components.end()};
}

void DrawingProgramCache::traverse_bvh_erase_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node, std::vector<CollabListType::ObjectInfoPtr>& components)> f) {
    f(nullptr, unsortedComponents);
    if(traverse_bvh_erase_function_recursive(bvhRoot, aabb, f))
        bvhRoot = nullptr;
}

bool DrawingProgramCache::traverse_bvh_erase_function_recursive(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node, std::vector<CollabListType::ObjectInfoPtr>& components)> f) {
    if(bvhNode && SCollision::collide(aabb, bvhNode->bounds)) {
        if(f(bvhNode, bvhNode->components))
            return true;
        std::erase_if(bvhNode->children, [&](auto& p) {
            return traverse_bvh_erase_function_recursive(p, aabb, f);
        });
    }
    return false;
}

void DrawingProgramCache::traverse_bvh_run_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node, const std::vector<CollabListType::ObjectInfoPtr>& components)> f) {
    f(nullptr, unsortedComponents);
    traverse_bvh_run_function_recursive(bvhRoot, aabb, f);
}

void DrawingProgramCache::traverse_bvh_run_function_recursive(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node, const std::vector<CollabListType::ObjectInfoPtr>& components)> f) {
    if(bvhNode && SCollision::collide(aabb, bvhNode->bounds) && f(bvhNode, bvhNode->components)) {
        for(auto& p : bvhNode->children)
            traverse_bvh_run_function_recursive(p, aabb, f);
    }
}

void DrawingProgramCache::preupdate_component(const CollabListType::ObjectInfoPtr& c) {
    auto parentBvhNode = c->obj->parentBvhNode.lock();
    if(parentBvhNode) {
        unsortedComponents.emplace_back(c);
        std::erase(parentBvhNode->components, c);
        c->obj->parentBvhNode.reset();
    }
    invalidate_cache_before_pos(c->pos);
}

void DrawingProgramCache::build_bvh_node(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, std::vector<CollabListType::ObjectInfoPtr> components) {
    if(components.empty())
        return;

    bvhNode->bounds = components.front()->obj->worldAABB.value();
    for(auto& c : components)
        bvhNode->bounds.include_aabb_in_bounds(c->obj->worldAABB.value());

    build_bvh_node_coords_and_resolution(*bvhNode);

    if(components.size() < MAXIMUM_COMPONENTS_IN_SINGLE_NODE) {
        bvhNode->components = components;
        for(auto& c : bvhNode->components)
            c->obj->parentBvhNode = bvhNode;
        return;
    }

    WorldVec boundsCenter = bvhNode->bounds.center();

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
        else {
            bvhNode->components.emplace_back(c);
            c->obj->parentBvhNode = bvhNode;
        }
    }

    for(auto& p : parts) {
        if(!p.empty())
            build_bvh_node(bvhNode->children.emplace_back(std::make_shared<DrawingProgramCacheBVHNode>()), p);
    }
}

void DrawingProgramCache::build_bvh_node_coords_and_resolution(DrawingProgramCacheBVHNode& node) {
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

void DrawingProgramCache::invalidate_cache_at_aabb_before_pos(const SCollision::AABB<WorldScalar>& aabb, uint64_t placementToInvalidateAt) {
    std::erase_if(nodesWithCachedSurfaces, [&](auto& n) {
        if(placementToInvalidateAt <= n->drawCache.value().lastDrawnComponentPlacement && SCollision::collide(aabb, n->bounds)) {
            n->drawCache = std::nullopt;
            return true;
        }
        return false;
    });
}

void DrawingProgramCache::refresh_all_draw_cache(const DrawData& drawData) {
    std::deque<std::shared_ptr<DrawingProgramCacheBVHNode>> nodeFlatList; // We want to render children before parents, so that parents can make use of the cached children
    traverse_bvh_run_function(drawData.cam.viewingAreaGenerousCollider, [&](std::shared_ptr<DrawingProgramCacheBVHNode> node, const std::vector<CollabListType::ObjectInfoPtr>& comps) {
        if(node && node->coords.inverseScale <= drawData.cam.c.inverseScale) {
            if(!node->drawCache)
                nodeFlatList.emplace_front(node);
            return false;
        }
        return true;
    });
    if(!nodeFlatList.empty())
        refresh_draw_cache(nodeFlatList.front(), drawData);
}

void DrawingProgramCache::refresh_draw_cache(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const DrawData& drawData) {
    if(bvhNode->children.empty() && bvhNode->components.empty())
        return;

    bvhNode->drawCache = std::nullopt;
    DrawingProgramCacheBVHNode::DrawCacheData drawCache;

    SkImageInfo imgInfo = SkImageInfo::MakeN32Premul(bvhNode->resolution.x(), bvhNode->resolution.y());
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
    cacheDrawData.cam.c = bvhNode->coords;
    cacheDrawData.cam.changed = nullptr;
    cacheDrawData.cam.set_viewing_area(bvhNode->resolution.cast<float>());
    cacheDrawData.refresh_draw_optimizing_values();

    draw_components_to_canvas(cacheCanvas, cacheDrawData, &drawCache.lastDrawnComponentPlacement);
    drawCache.lastRenderTime = std::chrono::steady_clock::now();
    drawCache.attachedCache = this;

    bvhNode->drawCache = drawCache; // Set the drawCache after rendering is done, so that the draw function doesnt assume we have this node cached

    while(nodesWithCachedSurfaces.size() >= MAXIMUM_DRAW_CACHE_SURFACES) {
        std::shared_ptr<DrawingProgramCacheBVHNode> leastUsedNode = *nodesWithCachedSurfaces.begin();
        for(const std::shared_ptr<DrawingProgramCacheBVHNode>& n : nodesWithCachedSurfaces) {
            if(n->drawCache.value().lastRenderTime < leastUsedNode->drawCache.value().lastRenderTime)
                leastUsedNode = n;
        }
        leastUsedNode->drawCache = std::nullopt;
        nodesWithCachedSurfaces.erase(leastUsedNode);
    }

    nodesWithCachedSurfaces.emplace(bvhNode);
}

void DrawingProgramCache::draw_components_to_canvas(SkCanvas* canvas, const DrawData& drawData, uint64_t* lastDrawnComponentPlacement) {
    uint64_t lastComponentDrawn = 0;

    std::vector<std::shared_ptr<DrawingProgramCacheBVHNode>> cachedNodesToDraw;
    std::vector<CollabListType::ObjectInfoPtr> uncachedCompsToDraw;

    traverse_bvh_run_function(drawData.cam.viewingAreaGenerousCollider, [&](const std::shared_ptr<DrawingProgramCacheBVHNode>& node, const std::vector<CollabListType::ObjectInfoPtr>& comps) {
        if(node && node->drawCache && node->coords.inverseScale <= drawData.cam.c.inverseScale) {
            cachedNodesToDraw.emplace_back(node);
            return false;
        }

        for(auto& c : comps)
            c->obj->calculate_draw_transform(drawData);
        
        uncachedCompsToDraw.insert(uncachedCompsToDraw.end(), comps.begin(), comps.end());

        return true;
    });

    std::sort(uncachedCompsToDraw.begin(), uncachedCompsToDraw.end(), [](auto& a, auto& b) {
        return a->pos < b->pos;
    });
    std::sort(cachedNodesToDraw.begin(), cachedNodesToDraw.end(), [](auto& a, auto& b) {
        return a->drawCache.value().lastDrawnComponentPlacement < b->drawCache.value().lastDrawnComponentPlacement;
    });
    size_t nextCacheToRender = 0;
    for(auto& c : uncachedCompsToDraw) {
        for(;;) {
            if(nextCacheToRender >= cachedNodesToDraw.size() || c->pos < cachedNodesToDraw[nextCacheToRender]->drawCache.value().lastDrawnComponentPlacement)
                break;
            auto& bvhNode = cachedNodesToDraw[nextCacheToRender];
            auto& drawCache = bvhNode->drawCache.value();
            canvas->save();
            bvhNode->coords.transform_sk_canvas(canvas, drawData);
            drawCache.lastRenderTime = std::chrono::steady_clock::now();

            lastComponentDrawn = std::max(lastComponentDrawn, drawCache.lastDrawnComponentPlacement);
        
            SkPaint p;
            p.setBlendMode(SkBlendMode::kSrc);

            // Note, no mipmaps here. You might need to generate mipmaps in the future (withDefaultMipmaps)
            canvas->drawImage(drawCache.surface->makeTemporaryImage(), 0, 0, {SkFilterMode::kLinear, SkMipmapMode::kLinear}, &p);

            canvas->restore();
            nextCacheToRender++;
        }
        c->obj->draw(canvas, drawData);
        c->obj->drawSetupData.shouldDraw = false;
        lastComponentDrawn = std::max(lastComponentDrawn, c->pos);
    }
    for(;nextCacheToRender < cachedNodesToDraw.size(); nextCacheToRender++) {
        auto& bvhNode = cachedNodesToDraw[nextCacheToRender];
        auto& drawCache = bvhNode->drawCache.value();
        canvas->save();
        bvhNode->coords.transform_sk_canvas(canvas, drawData);
        drawCache.lastRenderTime = std::chrono::steady_clock::now();

        lastComponentDrawn = std::max(lastComponentDrawn, drawCache.lastDrawnComponentPlacement);

        SkPaint p;
        p.setBlendMode(SkBlendMode::kSrc);

        // Note, no mipmaps here. You might need to generate mipmaps in the future (withDefaultMipmaps)
        canvas->drawImage(drawCache.surface->makeTemporaryImage(), 0, 0, {SkFilterMode::kLinear, SkMipmapMode::kLinear}, &p);

        canvas->restore();
    }

    if(lastDrawnComponentPlacement)
        *lastDrawnComponentPlacement = lastComponentDrawn;
}

DrawingProgramCache::~DrawingProgramCache() {
    clear_own_cached_surfaces();
}
