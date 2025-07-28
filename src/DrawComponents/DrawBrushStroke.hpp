#pragma once
#include "../SharedTypes.hpp"
#include "DrawComponent.hpp"
#include "Helpers/SCollision.hpp"
#include <Helpers/Serializers.hpp>

#ifndef IS_SERVER
#include <include/core/SkPath.h>
#endif

#define DRAW_MINIMUM_LIMIT 0.75


struct DrawBrushStrokePoint {
    Vector2f pos;
    float width;

    template <typename Archive> void serialize(Archive& a) {
        a(pos, width);
    }
};

class DrawBrushStroke : public DrawComponent {
    public:
        virtual DrawComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;

        struct Data {
            std::vector<DrawBrushStrokePoint> points;
            Vector4f color;
            bool hasRoundCaps;
        } d;

#ifndef IS_SERVER
        virtual std::shared_ptr<DrawComponent> copy() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual void update(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) override;
        virtual void create_collider() override;
        virtual void update_from_delayed_ptr() override;

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        sk_sp<SkVertices> vertices;
        std::array<sk_sp<SkVertices>, 3> verticesMipMap;
        SCollision::AABB<float> bounds;

    private:
        void add_precheck_aabb_level(size_t level, const std::vector<SCollision::BVHContainer<float>>& levelArray);
        std::vector<SCollision::AABB<float>> precheckAABBLevels;
        void create_triangles(const std::function<bool(Vector2f, Vector2f, Vector2f)>& passTriangleFunc, const std::vector<DrawBrushStrokePoint>& points, unsigned skipVertexCount);
        void vertices_to_draw_data(sk_sp<SkVertices>& vertexDataPtr, const std::vector<SkPoint>& vertexData);
        std::vector<size_t> get_wedge_indices(const std::vector<DrawBrushStrokePoint>& points);
        std::vector<DrawBrushStrokePoint> smooth_points(size_t beginIndex, size_t endIndex, unsigned numOfDivisions);
        std::vector<DrawBrushStrokePoint> smooth_points_avg(size_t beginIndex, size_t endIndex, unsigned numOfDivisions);
#endif
};

