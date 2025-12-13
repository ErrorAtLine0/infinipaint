#pragma once
#include "../CoordSpaceHelper.hpp"
#include "CanvasComponent.hpp"
#include <Helpers/NetworkingObjects/NetObjManagerTypeList.hpp>
#include "../VersionConstants.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>

class DrawingProgram;
class CanvasComponentAllocator;

class CanvasComponentContainer {
    public:
        typedef NetworkingObjects::NetObjOrderedListObjectInfo<CanvasComponentContainer> ObjInfo;
        typedef std::shared_ptr<ObjInfo> ObjInfoSharedPtr;
        typedef std::weak_ptr<ObjInfo> ObjInfoWeakPtr;

        constexpr static int COMP_MAX_SHIFT_BEFORE_STOP_COLLISIONS = 14;
        constexpr static int COMP_MAX_SHIFT_BEFORE_STOP_SCALING = 14;
        constexpr static int COMP_MIN_SHIFT_BEFORE_DISAPPEAR = 11;
        constexpr static int COMP_COLLIDE_MIN_SHIFT_TINY = 9;
        constexpr static int COMP_MIPMAP_LEVEL_ONE = 2;
        constexpr static int COMP_MIPMAP_LEVEL_TWO = 5;
        
        CanvasComponentContainer();
        CanvasComponentContainer(NetworkingObjects::NetObjManager& objMan, CanvasComponent::CompType type);
        static void register_class(NetworkingObjects::NetObjManager& t);
        static void delay_post_assignment_update(DrawingProgram& drawP, CanvasComponent& comp);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);
        CanvasComponent& get_comp() const;
        const CoordSpaceHelper& get_coords() const;
        std::optional<SCollision::AABB<WorldScalar>> get_world_bounds() const;
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void commit_update(DrawingProgram& drawP);
        void commit_transform(DrawingProgram& drawP);
        void commit_update_dont_invalidate_cache(DrawingProgram& drawP); // Must be thread safe
        bool should_draw(const DrawData& drawData) const;
        bool collides_with_world_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld) const;
        bool collides_with_cam_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<float>& checkAgainstCam) const;
        bool collides_with(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, const SCollision::ColliderCollection<float>& checkAgainstCam) const;
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
        CanvasComponent* allocate_comp(CanvasComponent::CompType type);
        void canvas_do_transform(SkCanvas* canvas, const TransformDrawData& transformData) const;
        TransformDrawData calculate_draw_transform(const DrawData& drawData) const;
        void calculate_world_bounds();

        ObjInfo* objInfo = nullptr;

        std::optional<SCollision::AABB<WorldScalar>> worldAABB;
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentAllocator> compAllocator;
        CoordSpaceHelper coords;
};
