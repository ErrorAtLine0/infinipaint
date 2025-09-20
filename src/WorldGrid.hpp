#pragma once
#include <include/core/SkCanvas.h>
#include "DrawData.hpp"
#include <include/effects/SkRuntimeEffect.h>

class GridManager;

class WorldGrid {
    public:
        void draw(GridManager& gMan, SkCanvas* canvas, const DrawData& drawData);
        enum class GridType : unsigned {
            CIRCLE_POINTS = 0,
            SQUARE_POINTS,
            SQUARE_LINES,
            RULED
        } gridType = GridType::CIRCLE_POINTS;
        WorldScalar size{50000000};
        bool visible = true;
        WorldVec offset{0, 0};
        std::optional<SCollision::AABB<WorldScalar>> bounds;
        Vector4f color{1.0f, 1.0f, 1.0f, 1.0f};

        void set_remove_divisions_outwards(bool v);
        bool get_remove_divisions_outwards();

        void set_subdivisions(uint32_t v);
        uint32_t get_subdivisions();

        static unsigned GRID_UNIT_PIXEL_SIZE;
    private:
        bool removeDivisionsOutwards = false;
        uint32_t subdivisions = 1;

        struct ShaderData {
            SkColor4f gridColor;
            float mainGridScale;
            Vector2f mainGridClosestPoint;
            float mainGridPointSize;

            float divGridAlphaFrac;
            float divGridScale;
            Vector2f divGridClosestPoint;
            float divGridPointSize;
        };
        void draw_coordinates(SkCanvas* canvas, const DrawData& drawData, const Vector2f& closestGridPointScreenPos, float floatDivWorldSize, const WorldVec& closestGridPointCoord, const WorldScalar& gridCoordDivSize);
        static Vector2f get_closest_grid_point(const WorldVec& gridOffset, const WorldScalar& gridSize, const DrawData& drawData);
        static WorldVec get_closest_grid_point_coordinate(const WorldVec& gridOffset, const WorldScalar& gridSize, const WorldScalar& gridSizeUnit, const DrawData& drawData);
        static sk_sp<SkShader> get_shader(GridType gType, const ShaderData& shaderData);
        static sk_sp<SkRuntimeEffect> compile_effect_shader_init(const char* shaderName, const char* shaderCode);
        static Vector2f oldWindowSize;
        static sk_sp<SkRuntimeEffect> circlePointEffect;
        static sk_sp<SkRuntimeEffect> squarePointEffect;
        static sk_sp<SkRuntimeEffect> squareLinesEffect;
        static sk_sp<SkRuntimeEffect> ruledEffect;
};
