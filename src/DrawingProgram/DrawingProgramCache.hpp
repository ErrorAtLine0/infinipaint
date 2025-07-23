#pragma once
#include <vector>
#include "../CollabList.hpp"
#include <include/core/SkSurface.h>
#include <unordered_set>
#include "../DrawData.hpp"

class DrawingProgram;

class DrawComponent;

class DrawingProgramCache {
    public:
        typedef CollabList<std::shared_ptr<DrawComponent>, ServerClientID> CollabListType;
        DrawingProgramCache(DrawingProgram& initDrawP);

        void update();
        void erase_component(const CollabListType::ObjectInfoPtr& c);
        void add_component(const CollabListType::ObjectInfoPtr& c);
        void refresh_all_draw_cache(SkCanvas* canvas, const DrawData& drawData);

        struct BVHNode {
            SCollision::AABB<WorldScalar> bounds;
            std::vector<CollabListType::ObjectInfoPtr> components;
            std::vector<std::shared_ptr<BVHNode>> children;

            CoordSpaceHelper coords;
            Vector2i resolution;
            struct DrawCacheData {
                uint64_t lastDrawnComponentPlacement;
                uint64_t firstDrawnComponentPlacement;
                sk_sp<SkSurface> surface;
                std::chrono::steady_clock::time_point lastRenderTime;
                DrawingProgram* attachedDrawP;
            };

            std::optional<DrawCacheData> drawCache;
        };

        static std::unordered_set<std::shared_ptr<BVHNode>> nodesWithCachedSurfaces;

        bool disableRefresh = false;

        void traverse_bvh_run_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<BVHNode>& node, const std::vector<CollabListType::ObjectInfoPtr>& components)> f);
        void traverse_bvh_collision_check(const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, const SCollision::ColliderCollection<float>& checkAgainstCam);

        void traverse_bvh_erase_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<BVHNode>& node, std::vector<CollabListType::ObjectInfoPtr>& components)> f);

        void force_rebuild();

        // Run before actually updating the component
        void preupdate_component(const CollabListType::ObjectInfoPtr& c);

        void invalidate_cache_before_pos(uint64_t placementToInvalidateAt);
        void invalidate_cache_at_aabb_before_pos(const SCollision::AABB<WorldScalar>& aabb, uint64_t placementToInvalidateAt);
    private:
        void clear_own_cached_surfaces();
        bool traverse_bvh_erase_function_recursive(const std::shared_ptr<BVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<BVHNode>& node, std::vector<CollabListType::ObjectInfoPtr>& components)> f);

        void traverse_bvh_collision_check_recursive(const std::shared_ptr<BVHNode>& bvhNode, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, const SCollision::ColliderCollection<float>& checkAgainstCam);
        void traverse_bvh_collision_check_recursive_tiny(const std::shared_ptr<BVHNode>& bvhNode);

        void build_bvh_node_coords_and_resolution(BVHNode& node);
        void refresh_draw_cache(const std::shared_ptr<BVHNode>& bvhNode, const DrawData& drawData);

        void build(std::vector<CollabListType::ObjectInfoPtr> components);

        std::chrono::steady_clock::time_point lastBvhBuildTime;

        void traverse_bvh_run_function_recursive(const std::shared_ptr<BVHNode>& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(const std::shared_ptr<BVHNode>& node, const std::vector<CollabListType::ObjectInfoPtr>& components)> f);
        std::pair<std::shared_ptr<DrawingProgramCache::BVHNode>, std::vector<DrawingProgramCache::CollabListType::ObjectInfoPtr>::iterator> get_bvh_node_fully_containing_recursive(const std::shared_ptr<BVHNode>& bvhNode, DrawComponent* c);

        void build_bvh_node(const std::shared_ptr<BVHNode>& bvhNode, std::vector<CollabListType::ObjectInfoPtr> components);

        std::shared_ptr<BVHNode> bvhRoot;
        std::vector<CollabListType::ObjectInfoPtr> unsortedComponents;

        std::unordered_set<CollabListType::ObjectInfoPtr> componentsSetToErase;

        DrawingProgram& drawP;
};
