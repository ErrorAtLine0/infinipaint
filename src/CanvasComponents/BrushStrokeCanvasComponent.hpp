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

struct BrushStrokeCanvasComponentPointDouble {
    BrushStrokeCanvasComponentPointDouble();
    BrushStrokeCanvasComponentPointDouble(const BrushStrokeCanvasComponentPoint& p);
    Vector2d pos;
    double width;
};

class BrushStrokeCanvasComponent : public CanvasComponent {
    public:
        constexpr static float DRAW_MINIMUM_LIMIT = 0.3;

        virtual CanvasComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;
        virtual std::unique_ptr<CanvasComponent> get_data_copy() const override;
        virtual void change_stroke_color(const Vector4f& newStrokeColor) override;
        virtual std::optional<Vector4f> get_stroke_color() const override;

        struct Data {
            std::shared_ptr<std::vector<BrushStrokeCanvasComponentPoint>> points = std::make_shared<std::vector<BrushStrokeCanvasComponentPoint>>(); // It's a pointer here since brush strokes cant be edited
            Vector4f color;
            bool hasRoundCaps;
        } d;

        virtual void set_data_from(const CanvasComponent& other) override;
    private:
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const override;
        virtual bool accurate_draw(SkCanvas* canvas, const DrawData& drawData, const CoordSpaceHelper& coords, const std::shared_ptr<void>& predrawData) const override;
        virtual std::shared_ptr<void> get_predraw_data_accurate(const DrawData& drawData, const CoordSpaceHelper& coords) const override;

        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const override;
        void create_collider();
        bool should_draw_extra(const DrawData& drawData, const CoordSpaceHelper& coords) const override;

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        std::shared_ptr<SkPath> brushPath;
        SCollision::AABB<float> bounds;

        std::array<std::shared_ptr<SkPath>, 2> brushPathLOD;

        std::vector<BrushStrokeCanvasComponentPointDouble> every_nth_point_include_front_and_back(const std::vector<BrushStrokeCanvasComponentPointDouble>& pts, size_t n) const;

        void add_precheck_aabb_level(size_t level, const std::vector<SCollision::BVHContainer<float>>& levelArray);
        std::vector<SCollision::AABB<float>> precheckAABBLevels;
        void create_triangles(const std::function<bool(Vector2d, Vector2d, Vector2d)>& passTriangleFunc, const std::vector<BrushStrokeCanvasComponentPointDouble>& smoothedPoints, size_t skipVertexCount, std::shared_ptr<SkPathBuilder> bPath) const;
        std::vector<size_t> get_wedge_indices(const std::vector<BrushStrokeCanvasComponentPointDouble>& points) const;
        std::vector<BrushStrokeCanvasComponentPointDouble> smooth_points(size_t beginIndex, size_t endIndex, unsigned numOfDivisions) const;
        //std::vector<BrushStrokeCanvasComponentPoint> smooth_points_avg(size_t beginIndex, size_t endIndex, unsigned numOfDivisions) const;
};

