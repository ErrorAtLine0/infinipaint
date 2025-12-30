#pragma once
#include "../CoordSpaceHelper.hpp"
#include <Helpers/NetworkingObjects/NetObjManagerTypeList.hpp>
#include "CanvasComponentType.hpp"
#include "../VersionConstants.hpp"
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>
#include "CanvasComponentAllocator.hpp"

class DrawingProgram;
class DrawingProgramLayerListItem;
struct DrawingProgramCacheBVHNode;

class CanvasComponentContainer {
    public:
        typedef NetworkingObjects::NetObjOrderedList<CanvasComponentContainer> NetList;
        typedef NetworkingObjects::NetObjOwnerPtr<NetList> NetListOwnerPtr;
        typedef NetworkingObjects::NetObjOrderedListObjectInfo<CanvasComponentContainer> ObjInfo;
        typedef NetworkingObjects::NetObjOrderedListIterator<CanvasComponentContainer> ObjInfoIterator;

        constexpr static int COMP_MAX_SHIFT_BEFORE_STOP_COLLISIONS = 14;
        constexpr static int COMP_MAX_SHIFT_BEFORE_STOP_SCALING = 14;
        constexpr static int COMP_MIN_SHIFT_BEFORE_DISAPPEAR = 11;
        constexpr static int COMP_COLLIDE_MIN_SHIFT_TINY = 9;
        constexpr static int COMP_MIPMAP_LEVEL_ONE = 2;
        constexpr static int COMP_MIPMAP_LEVEL_TWO = 5;

        struct CopyData {
            CoordSpaceHelper coords;
            std::unique_ptr<CanvasComponent> obj;
        };
        
        CanvasComponentContainer();
        CanvasComponentContainer(NetworkingObjects::NetObjManager& objMan, CanvasComponentType type);
        CanvasComponentContainer(NetworkingObjects::NetObjManager& objMan, const CopyData& copyData);

        static void register_class(NetworkingObjects::NetObjManager& t);
        std::shared_ptr<CopyData> get_data_copy() const;
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);
        CanvasComponent& get_comp() const;
        SCollision::AABB<WorldScalar> get_world_bounds() const;
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void commit_update(DrawingProgram& drawP);
        void commit_transform_dont_invalidate_cache(DrawingProgram& drawP); // Must be thread safe
        void commit_transform(DrawingProgram& drawP);
        void commit_update_dont_invalidate_cache(DrawingProgram& drawP); // Must be thread safe
        bool should_draw(const DrawData& drawData) const;
        bool collides_with_world_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld) const;
        bool collides_with_cam_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<float>& checkAgainstCam) const;
        bool collides_with(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, const SCollision::ColliderCollection<float>& checkAgainstCam) const;
        void send_comp_update(DrawingProgram& drawP, bool finalUpdate);

        std::weak_ptr<DrawingProgramCacheBVHNode> cacheParentBvhNode;
        DrawingProgramLayerListItem* parentLayer = nullptr;
        CoordSpaceHelper coords;
        ObjInfoIterator objInfo;
    private:
        friend class BrushStrokeCanvasComponent;
        friend class ImageCanvasComponent;

        static void write_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentContainer>& o, cereal::PortableBinaryOutputArchive& a);
        static void read_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentContainer>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c);

        struct TransformDrawData {
            Vector2f translation;
            float rotation;
            float scale;
        };

        unsigned get_mipmap_level(const DrawData& drawData) const;
        CanvasComponent* allocate_comp(CanvasComponentType type);
        void canvas_do_transform(SkCanvas* canvas, const TransformDrawData& transformData) const;
        TransformDrawData calculate_draw_transform(const DrawData& drawData) const;
        void calculate_world_bounds();

        std::optional<SCollision::AABB<WorldScalar>> worldAABB;
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentAllocator> compAllocator;
};
