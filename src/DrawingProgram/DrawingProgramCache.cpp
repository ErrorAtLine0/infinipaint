#include "DrawingProgramCache.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include "Layers/DrawingProgramLayerManager.hpp"

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif

std::unordered_map<std::shared_ptr<DrawingProgramCacheBVHNode>, DrawingProgramCache::NodeCache> DrawingProgramCache::nodeCacheMap;

DrawingProgramCache::DrawingProgramCache(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

void DrawingProgramCache::add_component(const CanvasComponentContainer::ObjInfoSharedPtr& c) {
    unsortedComponents.emplace_back(c);
    invalidate_cache_at_aabb(c->obj->get_world_bounds());
}

void DrawingProgramCache::erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& c) {
    auto cacheParentBvhNodeLock = c->obj->cacheParentBvhNode.lock();
    if(cacheParentBvhNodeLock) {
        std::erase(cacheParentBvhNodeLock->components, c);
        c->obj->cacheParentBvhNode.reset();
    }
    else
        std::erase(unsortedComponents, c);
    invalidate_cache_at_aabb(c->obj->get_world_bounds());
}

void DrawingProgramCache::invalidate_cache_at_aabb(const SCollision::AABB<WorldScalar>& aabb) {
    for(auto& [node, nodeCache] : nodeCacheMap) {
        if(nodeCache.attachedDrawingProgramCache == this && SCollision::collide(aabb, node->bounds)) {
            if(nodeCache.invalidBounds.has_value()) {
                auto& iBounds = nodeCache.invalidBounds.value();
                iBounds.include_aabb_in_bounds(aabb);
            }
            else
                nodeCache.invalidBounds = aabb;
        }
    }
}

bool DrawingProgramCache::should_rebuild() const {
    return (unsortedComponents.size() >= MINIMUM_COMPONENTS_TO_START_REBUILD);
}

bool DrawingProgramCache::unsorted_components_exist() const {
    return !unsortedComponents.empty();
}

void DrawingProgramCache::build(const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToExclude) {
    internal_build(drawP.layerMan.get_flattened_component_list(), objsToExclude);
}

void DrawingProgramCache::internal_build(std::vector<CanvasComponentContainer::ObjInfoSharedPtr> componentsToBuild, const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToNotInclude) {
    std::erase_if(componentsToBuild, [&objsToNotInclude](auto& c) {
        return objsToNotInclude.contains(c);
    });
    bvhRoot = std::make_shared<DrawingProgramCacheBVHNode>();
    unsortedComponents.clear();
    clear_own_cached_surfaces();
    build_bvh_node(bvhRoot, componentsToBuild);
}

void DrawingProgramCache::clear_own_cached_surfaces() {
    std::erase_if(nodeCacheMap, [&](auto& nodeCachePair) {
        return nodeCachePair.second.attachedDrawingProgramCache == this;
    });
}

CanvasComponentContainer::ObjInfoSharedPtr DrawingProgramCache::get_front_object_colliding_with_in_editing_layer(const SCollision::ColliderCollection<float>& cC) {
    auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);
    CanvasComponentContainer::ObjInfoSharedPtr p;
    traverse_bvh_run_function(cCWorld.bounds, [&](const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode) {
        node_loop_components(bvhNode, [&](const auto& c) {
            if(drawP.layerMan.component_passes_layer_selector(c, DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED) && (!p || c->pos >= p->pos) && c->obj->collides_with_world_coords(drawP.world.drawData.cam.c, cCWorld))
                p = c;
        });
        return true;
    });
    return p;
}

void DrawingProgramCache::node_loop_erase_if_components(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, std::function<bool(const CanvasComponentContainer::ObjInfoSharedPtr& comp)> f) {
    if(bvhNode) {
        std::erase_if(bvhNode->components, [&f](const auto& comp) {
            if(f(comp)) {
                comp->obj->cacheParentBvhNode.reset();
                return true;
            }
            return false;
        });
    }
    else
        std::erase_if(unsortedComponents, f);
}

void DrawingProgramCache::node_loop_components(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, std::function<void(const CanvasComponentContainer::ObjInfoSharedPtr& comp)> f) {
    if(bvhNode)
        std::for_each(bvhNode->components.begin(), bvhNode->components.end(), f);
    else
        std::for_each(unsortedComponents.begin(), unsortedComponents.end(), f);
}

void DrawingProgramCache::preupdate_component(const CanvasComponentContainer::ObjInfoSharedPtr& c) {
    // Can be called even if object isn't in cache yet. In that case, it'll just invalidate the cache at the object's AABB
    auto cacheParentBvhNodeLock = c->obj->cacheParentBvhNode.lock();
    if(cacheParentBvhNodeLock) {
        unsortedComponents.emplace_back(c);
        std::erase(cacheParentBvhNodeLock->components, c);
        c->obj->cacheParentBvhNode.reset();
    }
    invalidate_cache_at_aabb(c->obj->get_world_bounds());
}

void DrawingProgramCache::build_bvh_node(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components) {
    if(components.empty())
        return;

    bvhNode->bounds = components.front()->obj->get_world_bounds();
    for(auto& c : components)
        bvhNode->bounds.include_aabb_in_bounds(c->obj->get_world_bounds());

    build_bvh_node_coords_and_resolution(*bvhNode);

    if(components.size() < MAXIMUM_COMPONENTS_IN_SINGLE_NODE) {
        bvhNode->components = components;
        for(auto& c : bvhNode->components)
            c->obj->cacheParentBvhNode = bvhNode;
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
            c->obj->cacheParentBvhNode = bvhNode;
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

void DrawingProgramCache::refresh_all_draw_cache(const DrawData& drawData) {
    std::deque<std::shared_ptr<DrawingProgramCacheBVHNode>> nodeFlatList; // We want to render children before parents, so that parents can make use of the cached children
    traverse_bvh_run_function(drawData.cam.viewingAreaGenerousCollider, [&](std::shared_ptr<DrawingProgramCacheBVHNode> node) {
        if(node && node->coords.inverseScale <= drawData.cam.c.inverseScale) {
            auto it = nodeCacheMap.find(node);
            if((it == nodeCacheMap.end()) || it->second.invalidBounds.has_value())
                nodeFlatList.emplace_front(node);
            return false;
        }
        return true;
    });
    if(!nodeFlatList.empty())
        refresh_draw_cache(nodeFlatList.front(), drawData);
}

void DrawingProgramCache::traverse_bvh_run_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node)> f) {
    f(nullptr);
    traverse_bvh_run_function_starting_at_node(bvhRoot, aabb, f);
}

void DrawingProgramCache::traverse_bvh_run_function_starting_at_node(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node)> f) {
    if(bvhNode && SCollision::collide(aabb, bvhNode->bounds) && f(bvhNode)) {
        for(auto& p : bvhNode->children)
            traverse_bvh_run_function_starting_at_node(p, aabb, f);
    }
}

void DrawingProgramCache::traverse_bvh_run_function_starting_at_node_no_collision_check(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node)> f) {
    if(bvhNode && f(bvhNode)) {
        for(auto& p : bvhNode->children)
            traverse_bvh_run_function_starting_at_node_no_collision_check(p, f);
    }
}

void DrawingProgramCache::refresh_draw_cache(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const DrawData& drawData) {
    if(bvhNode->children.empty() && bvhNode->components.empty())
        return;

    NodeCache nodeCache;
    auto bvhNodeCacheIt = nodeCacheMap.find(bvhNode);
    if(bvhNodeCacheIt != nodeCacheMap.end()) {
        nodeCache = bvhNodeCacheIt->second;
        nodeCacheMap.erase(bvhNode); // Ensure nodeCache is erased, so that the draw_components_to_canvas function doesnt use it while drawing
    }
    else {
        #ifdef USE_BACKEND_OPENGLES_3_0
            SkImageInfo imgInfo = SkImageInfo::Make(bvhNode->resolution.x(), bvhNode->resolution.y(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        #else
            SkImageInfo imgInfo = SkImageInfo::MakeN32Premul(bvhNode->resolution.x(), bvhNode->resolution.y());
        #endif
        #ifdef USE_SKIA_BACKEND_GRAPHITE
            nodeCache.surface = SkSurfaces::RenderTarget(drawP.world.main.window.recorder(), imgInfo, skgpu::Mipmapped::kNo, drawP.world.main.window.defaultMSAASurfaceProps);
        #elif USE_SKIA_BACKEND_GANESH
            nodeCache.surface = SkSurfaces::RenderTarget(drawP.world.main.window.ctx.get(), skgpu::Budgeted::kNo, imgInfo, drawP.world.main.window.defaultMSAASampleCount, &drawP.world.main.window.defaultMSAASurfaceProps);
        #endif

        if(!nodeCache.surface)
            throw std::runtime_error("[DrawingProgramCache::refresh_draw_cache] Could not make cache surface");
    }

    SkCanvas* cacheCanvas = nodeCache.surface->getCanvas();

    DrawData cacheDrawData = drawData;
    cacheDrawData.cam.c = bvhNode->coords;
    cacheDrawData.cam.set_viewing_area(bvhNode->resolution.cast<float>());
    cacheDrawData.refresh_draw_optimizing_values();

    if(nodeCache.invalidBounds.has_value()) {
        auto iBounds = nodeCache.invalidBounds.value();
        iBounds = bvhNode->bounds.get_intersection_between_aabbs(iBounds);

        WorldVec bDim = bvhNode->bounds.dim();

        Vector2f clipBoundMin{static_cast<float>((iBounds.min.x() - bvhNode->bounds.min.x()) / bDim.x()) * bvhNode->resolution.x(),
                              static_cast<float>((iBounds.min.y() - bvhNode->bounds.min.y()) / bDim.y()) * bvhNode->resolution.y()};
        Vector2f clipBoundMax{static_cast<float>((iBounds.max.x() - bvhNode->bounds.min.x()) / bDim.x()) * bvhNode->resolution.x(),
                              static_cast<float>((iBounds.max.y() - bvhNode->bounds.min.y()) / bDim.y()) * bvhNode->resolution.y()};

        SkIRect clipRect = SkIRect::MakeLTRB(clipBoundMin.x() - 2,
                                             clipBoundMin.y() - 2,
                                             clipBoundMax.x() + 2,
                                             clipBoundMax.y() + 2);

        SCollision::AABB<float> clipRectBoundAABB{clipBoundMin - Vector2f{4, 4}, clipBoundMax + Vector2f{4, 4}};
        SCollision::AABB<WorldScalar> clipRectBoundAABBWorld{
            bvhNode->coords.from_space(clipRectBoundAABB.min),
            bvhNode->coords.from_space(clipRectBoundAABB.max)
        };

        cacheCanvas->save();
        cacheCanvas->clipIRect(clipRect);
        cacheCanvas->clear(SkColor4f{0, 0, 0, 0});
        draw_components_to_canvas(cacheCanvas, cacheDrawData, clipRectBoundAABBWorld);
        cacheCanvas->restore();
        nodeCache.invalidBounds = std::nullopt;
    }
    else {
        cacheCanvas->clear(SkColor4f{0, 0, 0, 0});
        draw_components_to_canvas(cacheCanvas, cacheDrawData, std::nullopt);
    }

    nodeCache.lastRenderTime = std::chrono::steady_clock::now();
    nodeCache.attachedDrawingProgramCache = this;

    while(nodeCacheMap.size() >= MAXIMUM_DRAW_CACHE_SURFACES) {
        auto leastUsedNodeIt = nodeCacheMap.begin();
        for(auto it = nodeCacheMap.begin(); it != nodeCacheMap.end(); ++it) {
            if(it->second.lastRenderTime < leastUsedNodeIt->second.lastRenderTime)
                leastUsedNodeIt = it;
        }
        nodeCacheMap.erase(leastUsedNodeIt);
    }

    nodeCacheMap.emplace(bvhNode, nodeCache); // Set the nodeCache after rendering is done, so that the draw function doesnt assume we have this node cached
}

void DrawingProgramCache::draw_components_to_canvas(SkCanvas* canvas, const DrawData& drawData, const std::optional<SCollision::AABB<WorldScalar>>& drawBounds) {
    if(drawP.layerMan.layer_tree_root_exists()) {
        std::vector<std::shared_ptr<DrawingProgramCacheBVHNode>> cachedNodesToDraw;
        std::vector<std::shared_ptr<DrawingProgramCacheBVHNode>> uncachedNodes;

        traverse_bvh_run_function(drawData.cam.viewingAreaGenerousCollider, [&](const std::shared_ptr<DrawingProgramCacheBVHNode>& node) {
            if(node) {
                auto it = nodeCacheMap.find(node);
                if(it != nodeCacheMap.end()) {
                    auto& nodeCache = it->second;
                    if(!nodeCache.invalidBounds.has_value() && node->coords.inverseScale <= drawData.cam.c.inverseScale) {
                        cachedNodesToDraw.emplace_back(node);
                        return false;
                    }
                }
            }
            if(node) // Not the "unsorted components" node (which is nullptr)
                uncachedNodes.emplace_back(node);
            return true;
        });

        recursive_draw_layer_item_to_canvas(drawP.layerMan.get_layer_root(), canvas, drawData, drawBounds, uncachedNodes);

        for(auto& nodeCacheToDraw : cachedNodesToDraw)
            draw_cache_image_to_canvas(canvas, drawData, nodeCacheToDraw);
    }
}

void DrawingProgramCache::recursive_draw_layer_item_to_canvas(const DrawingProgramLayerListItem& layerListItem, SkCanvas* canvas, const DrawData& drawData, const std::optional<SCollision::AABB<WorldScalar>>& drawBounds, const std::vector<std::shared_ptr<DrawingProgramCacheBVHNode>>& nodesToDraw) {
    if(layerListItem.get_visible()) {
        SkPaint layerPaint;
        layerPaint.setAlphaf(layerListItem.get_alpha());
        layerPaint.setBlendMode(serialized_blend_mode_to_sk_blend_mode(layerListItem.get_blend_mode()));
        canvas->saveLayer(nullptr, &layerPaint);
        if(layerListItem.is_folder()) {
            for(auto& p : layerListItem.get_folder().folderList->get_data() | std::views::reverse)
                recursive_draw_layer_item_to_canvas(*p->obj, canvas, drawData, drawBounds, nodesToDraw);
        }
        else {
            std::vector<CanvasComponentContainer::ObjInfoSharedPtr> compsToDraw;
            for(auto& c : unsortedComponents) {
                if(c->obj->parentLayer == &layerListItem && (!drawBounds.has_value() || SCollision::collide(drawBounds.value(), c->obj->get_world_bounds())))
                    compsToDraw.emplace_back(c);
            }
            for(auto& node : nodesToDraw) {
                for(auto& c : node->components) {
                    if(c->obj->parentLayer == &layerListItem && (!drawBounds.has_value() || SCollision::collide(drawBounds.value(), c->obj->get_world_bounds())))
                        compsToDraw.emplace_back(c);
                }
            }
            std::sort(compsToDraw.begin(), compsToDraw.end(), [](auto& a, auto& b) {
                return a->pos < b->pos;
            });
            for(auto& c : compsToDraw)
                c->obj->draw(canvas, drawData);
        }
        canvas->restore();
    }
}

void DrawingProgramCache::draw_cache_image_to_canvas(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode) {
    auto it = nodeCacheMap.find(bvhNode);
    if(it != nodeCacheMap.end()) {
        auto& nodeCache = it->second;
        canvas->save();
        bvhNode->coords.transform_sk_canvas(canvas, drawData);
        nodeCache.lastRenderTime = std::chrono::steady_clock::now();

        SkPaint srcPaint;
        srcPaint.setBlendMode(SkBlendMode::kSrc);

        canvas->drawImage(nodeCache.surface->makeTemporaryImage(), 0, 0, {SkFilterMode::kLinear, SkMipmapMode::kLinear}, &srcPaint);

        canvas->restore();
    }
}

DrawingProgramCache::~DrawingProgramCache() {
    clear_own_cached_surfaces();
}
