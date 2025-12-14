#pragma once
#include <chrono>
#include <vector>
#include <include/core/SkSurface.h>
#include <unordered_set>
#include "../DrawData.hpp"
#include "../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgram;
class DrawingProgramCache;

class DrawingProgramCacheBVHNode {
    public:
        SCollision::AABB<WorldScalar> bounds;
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> components;
        std::vector<std::shared_ptr<DrawingProgramCacheBVHNode>> children;

        CoordSpaceHelper coords;
        Vector2i resolution;
        struct DrawCacheData {
            uint32_t lastDrawnComponentPlacement;
            sk_sp<SkSurface> surface;
            std::chrono::steady_clock::time_point lastRenderTime;
            DrawingProgramCache* attachedCache;
            std::optional<SCollision::AABB<WorldScalar>> invalidBounds;
            bool isInvalid = false;
        };

        std::optional<DrawCacheData> drawCache;
};

class DrawingProgramCache {
    public:
        DrawingProgramCache(DrawingProgram& initDrawP);
        ~DrawingProgramCache();

        static constexpr size_t MINIMUM_COMPONENTS_TO_START_REBUILD = 1000;
        static constexpr size_t MAXIMUM_COMPONENTS_IN_SINGLE_NODE = 50;

        bool check_if_rebuild_should_occur();
        void test_rebuild(const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& comps, bool force = false);
        void test_rebuild_dont_include_set(const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& comps, const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToNotInclude, bool force = false);
        void test_rebuild_dont_include_set_dont_include_nodes(const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& comps, const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToNotInclude, const std::unordered_set<std::shared_ptr<DrawingProgramCacheBVHNode>>& nodesToNotInclude, bool force = false);

        static void move_components_from_bvh_nodes_to_set(std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& s, const std::unordered_set<std::shared_ptr<DrawingProgramCacheBVHNode>>& bvhNodes);
        static void move_components_from_bvh_node_to_set(std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& s, const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode);

        void erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void add_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void refresh_all_draw_cache(const DrawData& drawData);
        void traverse_bvh_run_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components)> f);
        void traverse_bvh_erase_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node, std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components)> f);
        // Run before actually updating the component
        void preupdate_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void invalidate_cache_before_pos(uint32_t placementToInvalidateAt);
        void invalidate_cache_at_aabb_before_pos(const SCollision::AABB<WorldScalar>& aabb, uint32_t placementToInvalidateAt);
        void clear();

        struct DrawComponentsToCanvasOptionalData {
            uint32_t* lastDrawnComponentPlacement = nullptr;
            std::optional<SCollision::AABB<WorldScalar>> drawBounds;
        };

        void draw_components_to_canvas(SkCanvas* canvas, const DrawData& drawData, DrawComponentsToCanvasOptionalData optData);
        const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& get_unsorted_component_list() const;
        const std::chrono::steady_clock::time_point& get_last_bvh_build_time() const;
        void clear_own_cached_surfaces();

        CanvasComponentContainer::ObjInfoSharedPtr get_front_object_colliding_with(const SCollision::ColliderCollection<float>& cC);

    private:
        static std::unordered_set<std::shared_ptr<DrawingProgramCacheBVHNode>> nodesWithCachedSurfaces;

        void draw_cache_image_to_canvas(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode);

        void force_rebuild_dont_include_objs(std::vector<CanvasComponentContainer::ObjInfoSharedPtr> componentsToBuild, const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToNotInclude);
        void force_rebuild(const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& componentsToBuild);

        bool traverse_bvh_erase_function_recursive(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node, std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components)> f);
        void build_bvh_node_coords_and_resolution(DrawingProgramCacheBVHNode& node);
        void refresh_draw_cache(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const DrawData& drawData);
        void build(std::vector<CanvasComponentContainer::ObjInfoSharedPtr> components);
        void traverse_bvh_run_function_recursive(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>& node, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components)> f);

        void build_bvh_node(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components);

        std::chrono::steady_clock::time_point lastBvhBuildTime;
        std::shared_ptr<DrawingProgramCacheBVHNode> bvhRoot;
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> unsortedComponents;
        DrawingProgram& drawP;
};
