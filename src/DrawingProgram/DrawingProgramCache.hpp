#pragma once
#include "../CanvasComponents/CanvasComponentContainer.hpp"
#include <unordered_map>

class DrawingProgramCache {
    public:
        static constexpr size_t MINIMUM_COMPONENTS_TO_START_REBUILD = 1000;
        static constexpr size_t MAXIMUM_COMPONENTS_IN_SINGLE_NODE = 50;
        static constexpr size_t MAXIMUM_DRAW_CACHE_SURFACES = 40;

        struct BVHNode {
            SCollision::AABB<WorldScalar> bounds;
            std::vector<CanvasComponentContainer::ObjInfoSharedPtr> components;
            std::vector<std::shared_ptr<BVHNode>> children;
            CoordSpaceHelper coords;
            Vector2i resolution;
        };

        DrawingProgramCache(DrawingProgram& initDrawP);
        void add_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void clear_own_cached_surfaces();
        void preupdate_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void build(const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToExclude);
        void traverse_bvh_run_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<BVHNode>& node, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components)> f);
        void traverse_bvh_erase_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<BVHNode>& node, std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components)> f);
        bool should_rebuild() const;
        void draw_components_to_canvas(SkCanvas* canvas, const DrawData& drawData, const std::optional<SCollision::AABB<WorldScalar>>& drawBounds);
        bool unsorted_components_exist() const;
        void invalidate_cache_at_aabb(const SCollision::AABB<WorldScalar>& aabb);
        void move_components_from_bvh_node_to_set(std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& s, const std::shared_ptr<BVHNode>& bvhNode);
        void refresh_all_draw_cache(const DrawData& drawData);
    private:
        struct NodeCache {
            sk_sp<SkSurface> surface;
            std::chrono::steady_clock::time_point lastRenderTime;
            DrawingProgramCache* attachedDrawingProgramCache;
            std::optional<SCollision::AABB<WorldScalar>> invalidBounds;
        };
        static std::unordered_map<std::shared_ptr<BVHNode>, NodeCache> nodeCacheMap;

        void internal_build(std::vector<CanvasComponentContainer::ObjInfoSharedPtr> componentsToBuild, const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToNotInclude);
        void build_bvh_node(const std::shared_ptr<BVHNode>& bvhNode, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components);
        void build_bvh_node_coords_and_resolution(BVHNode& node);
        void refresh_draw_cache(const std::shared_ptr<BVHNode>& bvhNode, const DrawData& drawData);
        void traverse_bvh_run_function_recursive(const std::shared_ptr<BVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<BVHNode>& node, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components)> f);
        bool traverse_bvh_erase_function_recursive(const std::shared_ptr<BVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<BVHNode>& node, std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components)> f);
        void draw_cache_image_to_canvas(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<BVHNode>& bvhNode);
        void recursive_draw_layer_item_to_canvas(const DrawingProgramLayerListItem& layerListItem, SkCanvas* canvas, const DrawData& drawData, const std::optional<SCollision::AABB<WorldScalar>>& drawBounds, const std::vector<std::shared_ptr<BVHNode>>& nodesToDraw);

        std::unordered_map<CanvasComponentContainer::ObjInfoSharedPtr, std::shared_ptr<BVHNode>> objToBVHNode;
        std::shared_ptr<BVHNode> bvhRoot;
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> unsortedComponents;
        DrawingProgram& drawP;
};
