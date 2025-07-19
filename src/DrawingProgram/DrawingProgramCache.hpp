#pragma once
#include <vector>
#include "../CollabList.hpp"
#include "../DrawComponents/DrawComponent.hpp"
#include <include/core/SkSurface.h>

class DrawingProgramCache {
    public:
        typedef CollabList<std::shared_ptr<DrawComponent>, ServerClientID> CollabListType;
        void update(const std::vector<CollabListType::ObjectInfoPtr>& components);
        void erase_component(const CollabListType::ObjectInfoPtr& c);
        void add_component(const CollabListType::ObjectInfoPtr& c);

        struct BVHNode {
            SCollision::AABB<WorldScalar> bounds;
            std::vector<CollabListType::ObjectInfoPtr> components;
            std::vector<BVHNode> children;
            struct DrawCacheData {
                uint64_t lastDrawnComponentPlacement;
                sk_sp<SkSurface> surface;
            };
            std::optional<DrawCacheData> drawCache;
        };

        std::vector<BVHNode*> nodesWithCachedSurfaces;

        void traverse_bvh_run_function(const SCollision::AABB<WorldScalar>& aabb, std::function<bool(BVHNode* node, const std::vector<CollabListType::ObjectInfoPtr>& components)> f);

        // Run before actually updating the component
        void preupdate_component(DrawComponent* c);

        void invalidate_cache_before_pos(uint64_t placementToInvalidateAt);
    private:

        void build(std::vector<CollabListType::ObjectInfoPtr> components);

        void traverse_bvh_run_function_recursive(BVHNode& bvhNode, const SCollision::AABB<WorldScalar>& aabb, std::function<bool(BVHNode* node, const std::vector<CollabListType::ObjectInfoPtr>& components)> f);
        std::pair<DrawingProgramCache::BVHNode*, std::vector<DrawingProgramCache::CollabListType::ObjectInfoPtr>::iterator> get_bvh_node_fully_containing_recursive(BVHNode& bvhNode, DrawComponent* c);

        void build_bvh_node(BVHNode& node, std::vector<CollabListType::ObjectInfoPtr> components);
        BVHNode bvhRoot;
        std::vector<CollabListType::ObjectInfoPtr> unsortedComponents;
};
