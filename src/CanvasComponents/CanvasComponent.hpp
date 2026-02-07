#pragma once
#include <cstdint>
#include <cereal/archives/portable_binary.hpp>
#include <Helpers/VersionNumber.hpp>
#include <include/core/SkCanvas.h>
#include <Helpers/SCollision.hpp>
#include "CanvasComponentType.hpp"
#include "CanvasComponentContainer.hpp"

class DrawingProgram;

class CanvasComponent {
    public:
        virtual CanvasComponentType get_type() const = 0;
        virtual ~CanvasComponent();
        static CanvasComponent* allocate_comp(CanvasComponentType type);
        virtual void save(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load(cereal::PortableBinaryInputArchive& a) = 0;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) = 0;
        virtual void update(DrawingProgram& drawP);
        virtual void get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const;
        virtual void remap_resource_ids(const std::unordered_map<NetworkingObjects::NetObjID, NetworkingObjects::NetObjID>& resourceOldToNewMap);
        virtual void change_stroke_color(const Vector4f& newStrokeColor);
        virtual std::optional<Vector4f> get_stroke_color() const;

        virtual void set_data_from(const CanvasComponent& other) = 0;
        virtual std::unique_ptr<CanvasComponent> get_data_copy() const = 0;
    protected:
        friend class CanvasComponentContainer;
        friend class CanvasComponentAllocator;

        virtual bool accurate_draw(SkCanvas* canvas, const DrawData& drawData, const CoordSpaceHelper& coords) const;

        virtual void draw(SkCanvas* canvas, const DrawData& drawData) const = 0;
        virtual void initialize_draw_data(DrawingProgram& drawP) = 0;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const = 0;

        virtual SCollision::AABB<float> get_obj_coord_bounds() const = 0;

        CanvasComponentContainer* compContainer = nullptr;
};
