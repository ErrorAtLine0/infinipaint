#pragma once
#include "CanvasComponent.hpp"
#include "Helpers/SCollision.hpp"
#include <Helpers/Serializers.hpp>
#include <include/core/SkPath.h>
#include <include/core/SkPathBuilder.h>

struct DrawBrushStrokePoint {
    Vector2f pos;
    float width;

    template <typename Archive> void serialize(Archive& a) {
        a(pos, width);
    }
};

class CanvasComponentBrushStroke : public CanvasComponent {
    public:
        constexpr static float DRAW_MINIMUM_LIMIT = 0.3;

        virtual DrawComponentType get_type() const override;
        virtual void write_update(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void read_update(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;

        struct Data {
            std::vector<DrawBrushStrokePoint> points;
            Vector4f color;
            bool hasRoundCaps;
        };
        std::shared_ptr<Data> d = std::make_shared<Data>(); // It's a pointer here since brush strokes cant be edited

#ifndef IS_SERVER
        virtual std::shared_ptr<DrawComponent> copy(DrawingProgram& drawP) const override;
        virtual std::shared_ptr<DrawComponent> deep_copy(DrawingProgram& drawP) const override;

        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual void update(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) override;
        void create_collider();
        virtual void update_from_delayed_ptr(const std::shared_ptr<DrawComponent>& delayedUpdatePtr) override;

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        std::shared_ptr<SkPath> brushPath;
        SCollision::AABB<float> bounds;

        std::array<std::shared_ptr<SkPath>, 2> brushPathLOD;
    private:
        std::vector<DrawBrushStrokePoint> every_nth_point_include_front_and_back(const std::vector<DrawBrushStrokePoint>& pts, size_t n);

        void add_precheck_aabb_level(size_t level, const std::vector<SCollision::BVHContainer<float>>& levelArray);
        std::vector<SCollision::AABB<float>> precheckAABBLevels;
        void create_triangles(const std::function<bool(Vector2f, Vector2f, Vector2f)>& passTriangleFunc, const std::vector<DrawBrushStrokePoint>& smoothedPoints, size_t skipVertexCount, std::shared_ptr<SkPathBuilder> bPath);
        std::vector<size_t> get_wedge_indices(const std::vector<DrawBrushStrokePoint>& points);
        std::vector<DrawBrushStrokePoint> smooth_points(size_t beginIndex, size_t endIndex, unsigned numOfDivisions);
        std::vector<DrawBrushStrokePoint> smooth_points_avg(size_t beginIndex, size_t endIndex, unsigned numOfDivisions);
#endif
};

