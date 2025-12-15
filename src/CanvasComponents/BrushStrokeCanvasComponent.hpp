#pragma once
#include "CanvasComponent.hpp"
#include "Helpers/SCollision.hpp"
#include <Helpers/Serializers.hpp>
#include <include/core/SkPath.h>
#include <include/core/SkPathBuilder.h>

struct BrushStrokeCanvasComponentPoint {
    Vector2f pos;
    float width;
    template <typename Archive> void serialize(Archive& a) {
        a(pos, width);
    }
};

class BrushStrokeCanvasComponent : public CanvasComponent {
    public:
        constexpr static float DRAW_MINIMUM_LIMIT = 0.3;

        virtual CanvasComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;

        struct Data {
            std::vector<BrushStrokeCanvasComponentPoint> points;
            Vector4f color;
            bool hasRoundCaps;
        };
        std::shared_ptr<Data> d = std::make_shared<Data>(); // It's a pointer here since brush strokes cant be edited

        virtual void set_data_from(const CanvasComponent& other) override;
    private:
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) const override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const override;
        void create_collider();

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        std::shared_ptr<SkPath> brushPath;
        SCollision::AABB<float> bounds;

        std::array<std::shared_ptr<SkPath>, 2> brushPathLOD;

        std::vector<BrushStrokeCanvasComponentPoint> every_nth_point_include_front_and_back(const std::vector<BrushStrokeCanvasComponentPoint>& pts, size_t n) const;

        void add_precheck_aabb_level(size_t level, const std::vector<SCollision::BVHContainer<float>>& levelArray);
        std::vector<SCollision::AABB<float>> precheckAABBLevels;
        void create_triangles(const std::function<bool(Vector2f, Vector2f, Vector2f)>& passTriangleFunc, const std::vector<BrushStrokeCanvasComponentPoint>& smoothedPoints, size_t skipVertexCount, std::shared_ptr<SkPathBuilder> bPath) const;
        std::vector<size_t> get_wedge_indices(const std::vector<BrushStrokeCanvasComponentPoint>& points) const;
        std::vector<BrushStrokeCanvasComponentPoint> smooth_points(size_t beginIndex, size_t endIndex, unsigned numOfDivisions) const;
        std::vector<BrushStrokeCanvasComponentPoint> smooth_points_avg(size_t beginIndex, size_t endIndex, unsigned numOfDivisions) const;
};

