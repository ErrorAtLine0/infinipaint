#pragma once
#include "../SharedTypes.hpp"
#include "DrawComponent.hpp"
#include "../CoordSpaceHelper.hpp"

#ifndef IS_SERVER
#include <include/core/SkPath.h>
#endif

class DrawEllipse : public DrawComponent {
    public:
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual DrawComponentType get_type() const override;

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

#ifndef IS_SERVER
        virtual std::shared_ptr<DrawComponent> copy() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual void update(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) override;
        virtual void update_from_delayed_ptr() override;
        void create_draw_data();
        virtual void create_collider() override;

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        SkPath ellipsePath;

        // Cache
        SCollision::BVHContainer<float> collisionTree;
    private:
        void create_triangles(const std::function<bool(Vector2f, Vector2f, Vector2f)>& passTriangleFunc);
#endif
};
