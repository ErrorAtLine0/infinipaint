#pragma once
#include "../SharedTypes.hpp"
#include "CanvasComponent.hpp"
#include "../CoordSpaceHelper.hpp"
#include <include/core/SkPath.h>

class RectangleCanvasComponent : public CanvasComponent {
    public:
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;
        virtual CanvasComponentType get_type() const override;
        std::unique_ptr<CanvasComponent> get_data_copy() const override;
        virtual void set_data_from(const CanvasComponent& other) override;

        // User input data
        struct Data {
            Vector4f strokeColor;
            Vector4f fillColor;
            float cornerRadius;
            float strokeWidth;
            Vector2f p1;
            Vector2f p2;
            uint8_t fillStrokeMode;
            bool operator==(const Data& o) const = default;
        } d;

    private:
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) const override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const override;
        void create_draw_data();
        void create_collider();
        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        SkPath rectPath;

        SCollision::BVHContainer<float> collisionTree;
};

