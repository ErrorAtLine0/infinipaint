#pragma once
#include <cstdint>
#include <cereal/archives/portable_binary.hpp>
#include <Helpers/VersionNumber.hpp>
#include <include/core/SkCanvas.h>
#include <Helpers/SCollision.hpp>

class DrawingProgram;
class DrawData;
class CanvasComponentContainer;

class CanvasComponent {
    public:
        enum class CompType : uint8_t {
            BRUSHSTROKE = 0,
            RECTANGLE,
            ELLIPSE,
            TEXTBOX,
            IMAGE
        };
        virtual CompType get_type() const = 0;
        virtual ~CanvasComponent();
        static CanvasComponent* allocate_comp(CanvasComponent::CompType type);
        virtual void save(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load(cereal::PortableBinaryInputArchive& a) = 0;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) = 0;

    protected:
        friend class CanvasComponentContainer;

        virtual void draw(SkCanvas* canvas, const DrawData& drawData) const = 0;
        virtual void initialize_draw_data(DrawingProgram& drawP) = 0;
        virtual void update(DrawingProgram& drawP);
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const = 0;

        virtual SCollision::AABB<float> get_obj_coord_bounds() const = 0;

        CanvasComponentContainer* compContainer = nullptr;
};
