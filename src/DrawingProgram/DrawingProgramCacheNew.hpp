#pragma once
#include "../CanvasComponents/CanvasComponentContainer.hpp"
#include <unordered_map>

class DrawingProgramCacheNew {
    public:
        static constexpr size_t MINIMUM_COMPONENTS_TO_START_REBUILD = 1000;
        static constexpr size_t MAXIMUM_COMPONENTS_IN_SINGLE_NODE = 50;
        static constexpr size_t MAXIMUM_DRAW_CACHE_SURFACES = 40;

        DrawingProgramCacheNew(DrawingProgram& initDrawP);
        void add_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void clear_own_cached_surfaces();
        void preupdate_component(const CanvasComponentContainer::ObjInfoSharedPtr& c);
        void build(const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToExclude);
        bool should_rebuild() const;
    private:
        struct BVHNode {
            SCollision::AABB<WorldScalar> bounds;
            std::vector<CanvasComponentContainer::ObjInfoSharedPtr> components;
            std::vector<std::shared_ptr<BVHNode>> children;

            CoordSpaceHelper coords;
            Vector2i resolution;
            struct DrawCacheData {
                sk_sp<SkSurface> surface;
                std::chrono::steady_clock::time_point lastRenderTime;
                DrawingProgramCacheNew* attachedCache;
                std::optional<SCollision::AABB<WorldScalar>> invalidBounds;
            };

            std::optional<DrawCacheData> drawCache;
        };
        static std::unordered_set<std::shared_ptr<BVHNode>> nodesWithCachedSurfaces;

        void internal_build(std::vector<CanvasComponentContainer::ObjInfoSharedPtr> componentsToBuild, const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& objsToNotInclude);
        void invalidate_cache_at_aabb(const SCollision::AABB<WorldScalar>& aabb);
        void build_bvh_node(const std::shared_ptr<BVHNode>& bvhNode, const std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& components);
        void build_bvh_node_coords_and_resolution(BVHNode& node);

        std::unordered_map<CanvasComponentContainer::ObjInfoSharedPtr, std::shared_ptr<BVHNode>> objToBVHNode;
        std::shared_ptr<BVHNode> bvhRoot;
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> unsortedComponents;
        DrawingProgram& drawP;
};
