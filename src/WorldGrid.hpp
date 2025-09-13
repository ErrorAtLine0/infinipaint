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
    private:
        static sk_sp<SkShader> get_shader(GridType gType, const SkColor4f& gridColor, float gridScale, const Vector2f& gridClosestPoint, float gridPointSize);
        static sk_sp<SkRuntimeEffect> compile_effect_shader_init(const char* shaderName, const char* shaderCode);
        static Vector2f oldWindowSize;
        static sk_sp<SkRuntimeEffect> circlePointEffect;
        static sk_sp<SkRuntimeEffect> squarePointEffect;
        static sk_sp<SkRuntimeEffect> squareLinesEffect;
        static sk_sp<SkRuntimeEffect> ruledEffect;
};
