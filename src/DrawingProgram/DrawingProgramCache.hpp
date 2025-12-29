#pragma once
#include "../CanvasComponents/CanvasComponentContainer.hpp"
#include <unordered_map>

struct DrawingProgramCacheBVHNode {
    public:
        SCollision::AABB<WorldScalar> bounds;
        CoordSpaceHelper coords;
        Vector2i resolution;
    private:
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> components;
        std::vector<std::shared_ptr<DrawingProgramCacheBVHNode>> children;
        friend class DrawingProgramCache;
};

class DrawingProgramCache {
    public:
        static constexpr size_t MINIMUM_COMPONENTS_TO_START_REBUILD = 10;
        static constexpr size_t MAXIMUM_COMPONENTS_IN_SINGLE_NODE = 3;
        static constexpr size_t MAXIMUM_DRAW_CACHE_SURFACES = 40;

        DrawingProgramCache(DrawingProgram& initDrawP);
        void add_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void clear_own_cached_surfaces();
        void preupdate_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void build(const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToExclude);
        void traverse_bvh_run_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node)> f);
        void traverse_bvh_run_function_starting_at_node(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node)> f);
        void traverse_bvh_run_function_starting_at_node_no_collision_check(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node)> f);
        void node_loop_erase_if_components(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, std::function<bool(const CanvasComponentContainer::ObjInfoSharedPtr& comp)> f);
        void node_loop_components(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, std::function<void(const CanvasComponentContainer::ObjInfoSharedPtr& comp)> f);
        bool should_rebuild() const;
        void draw_components_to_canvas(SkCanvas* canvas, const DrawData& drawData, const std::optional<SCollision::AABB<WorldScalar>>& drawBounds);
        bool unsorted_components_exist() const;
        void invalidate_cache_at_aabb(const SCollision::AABB<WorldScalar>& aabb);
        void refresh_all_draw_cache(const DrawData& drawData);
        CanvasComponentContainer::ObjInfoSharedPtr get_front_object_colliding_with_in_editing_layer(const SCollision::ColliderCollection<float>& cC);
        ~DrawingProgramCache();
    private:
        struct NodeCache {
            sk_sp<SkSurface> surface;
            std::chrono::steady_clock::time_point lastRenderTime;
            DrawingProgramCache* attachedDrawingProgramCache;
            std::optional<SCollision::AABB<WorldScalar>> invalidBounds;
        };
        static std::unordered_map<std::shared_ptr<DrawingProgramCacheBVHNode>, NodeCache> nodeCacheMap;

        void internal_build(std::vector<CanvasComponentContainer::ObjInfoSharedPtr> componentsToBuild, const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToNotInclude);
        void build_bvh_node(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components);
        void build_bvh_node_coords_and_resolution(DrawingProgramCacheBVHNode& node);
        void refresh_draw_cache(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const DrawData& drawData);
        void draw_cache_image_to_canvas(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode);
        void recursive_draw_layer_item_to_canvas(const DrawingProgramLayerListItem& layerListItem, SkCanvas* canvas, const DrawData& drawData, const std::optional<SCollision::AABB<WorldScalar>>& drawBounds, const std::vector<std::shared_ptr<DrawingProgramCacheBVHNode>>& nodesToDraw);

        std::shared_ptr<DrawingProgramCacheBVHNode> bvhRoot;
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> unsortedComponents;
        DrawingProgram& drawP;
};
