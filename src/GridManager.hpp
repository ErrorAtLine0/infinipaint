#pragma once
#include <Eigen/Dense>
#include <include/core/SkCanvas.h>
#include "DrawData.hpp"
#include <include/core/SkPath.h>
#include <include/effects/SkRuntimeEffect.h>
#include <nlohmann/json.hpp>

using namespace Eigen;

class MainProgram;

class GridManager {
    public:
        GridManager(MainProgram& initMain);
        void draw(SkCanvas* canvas, const DrawData& drawData);

        enum GridType : unsigned {
            GRIDTYPE_NONE = 0,
            GRIDTYPE_CIRCLEDOT,
            GRIDTYPE_SQUAREDOT,
            GRIDTYPE_LINE
        };

        GridType gridType = GRIDTYPE_CIRCLEDOT;
    private:
        Vector2f oldWindowSize{0.0f, 0.0f};
        WorldScalar size{50000000};
        SkPath gridPath;
        sk_sp<SkShader> circleDotShader;
        sk_sp<SkShader> squareDotShader;
        sk_sp<SkShader> lineShader;
        MainProgram& main;
};

NLOHMANN_JSON_SERIALIZE_ENUM(GridManager::GridType, {
    {GridManager::GRIDTYPE_NONE, "None"},
    {GridManager::GRIDTYPE_CIRCLEDOT, "Circle Dot"},
    {GridManager::GRIDTYPE_SQUAREDOT, "Square Dot"},
    {GridManager::GRIDTYPE_LINE, "Line"}
})
