#pragma once
#include "CanvasComponent.hpp"
#include "../CoordSpaceHelper.hpp"
#include <include/core/SkPath.h>

class EllipseCanvasComponent : public CanvasComponent {
    public:
        virtual CanvasComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;
        virtual void change_stroke_color(const Vector4f& newStrokeColor) override;
        virtual std::optional<Vector4f> get_stroke_color() const override;
        virtual std::unique_ptr<CanvasComponent> get_data_copy() const override;
        virtual void set_data_from(const CanvasComponent& other) override;

        // User input data
        struct Data {
            Vector4f strokeColor;
            Vector4f fillColor;
            float strokeWidth;
            Vector2f p1;
            Vector2f p2;
            uint8_t fillStrokeMode;
            bool operator==(const Data& o) const = default;
        } d;

    private:
        void create_draw_data();
        void create_collider();
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) const override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const override;
        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;
        void create_triangles(const std::function<bool(Vector2f, Vector2f, Vector2f)>& passTriangleFunc);
        SCollision::BVHContainer<float> collisionTree;
        SkPath ellipsePath;
};
