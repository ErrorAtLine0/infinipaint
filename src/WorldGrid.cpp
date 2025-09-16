#include "WorldGrid.hpp"
#include "GridManager.hpp"
#include "Helpers/MathExtras.hpp"
#include "MainProgram.hpp"

sk_sp<SkRuntimeEffect> WorldGrid::circlePointEffect;
sk_sp<SkRuntimeEffect> WorldGrid::squarePointEffect;
sk_sp<SkRuntimeEffect> WorldGrid::squareLinesEffect;
sk_sp<SkRuntimeEffect> WorldGrid::ruledEffect;
Vector2f WorldGrid::oldWindowSize = Vector2f{0.0f, 0.0f};

// NOTE: Skia shaders return premultiplied alpha colors

const char* circlePointShaderCode = R"V(
uniform vec4 gridColor;
uniform float mainGridScale;
uniform vec2 mainGridClosestPoint;
uniform float mainGridPointSize;

uniform float divGridAlphaFrac;
uniform float divGridScale;
uniform vec2 divGridClosestPoint;
uniform float divGridPointSize;

float get_color_val_circle_point(float2 fCoord, float gScale, float gPointSize, float2 gClosestPoint) {
    float2 modCoord = mod(fCoord - gClosestPoint + float2(0.5 * gScale), float2(gScale));
    return 1.0 - smoothstep(gPointSize, gPointSize + 1.0, distance(modCoord, float2(0.5 * gScale)));
}

vec4 main(float2 fragCoord) {
    float mainGridColor = get_color_val_circle_point(fragCoord, mainGridScale, mainGridPointSize, mainGridClosestPoint);
    float divGridColor = get_color_val_circle_point(fragCoord, divGridScale, divGridPointSize, divGridClosestPoint) * divGridAlphaFrac;
    float fin = max(mainGridColor, divGridColor);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* squarePointShaderCode = R"V(
uniform vec4 gridColor;
uniform float mainGridScale;
uniform vec2 mainGridClosestPoint;
uniform float mainGridPointSize;

uniform float divGridAlphaFrac;
uniform float divGridScale;
uniform vec2 divGridClosestPoint;
uniform float divGridPointSize;

float get_color_val_square_point(float2 fCoord, float gScale, float gPointSize, float2 gClosestPoint) {
    float2 modCoord = mod(fCoord - gClosestPoint + float2(0.5 * gScale), float2(gScale));
    float2 difToPointCenter = abs(modCoord - float2(0.5 * gScale));
    float m1 = 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.x);
    float m2 = 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.y);
    return m1 * m2;
}

vec4 main(float2 fragCoord) {
    float mainGridColor = get_color_val_square_point(fragCoord, mainGridScale, mainGridPointSize, mainGridClosestPoint);
    float divGridColor = get_color_val_square_point(fragCoord, divGridScale, divGridPointSize, divGridClosestPoint) * divGridAlphaFrac;
    float fin = max(mainGridColor, divGridColor);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* squareLinesShaderCode = R"V(
uniform vec4 gridColor;
uniform float mainGridScale;
uniform vec2 mainGridClosestPoint;
uniform float mainGridPointSize;

uniform float divGridAlphaFrac;
uniform float divGridScale;
uniform vec2 divGridClosestPoint;
uniform float divGridPointSize;

float get_color_val_square_lines(float2 fCoord, float gScale, float gPointSize, float2 gClosestPoint) {
    float2 modCoord = mod(fCoord - gClosestPoint + float2(0.5 * gScale), float2(gScale));
    float2 difToPointCenter = abs(modCoord - float2(0.5 * gScale));
    float m1 = 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.x);
    float m2 = 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.y);
    return max(m1, m2);
}

vec4 main(float2 fragCoord) {
    float mainGridColor = get_color_val_square_lines(fragCoord, mainGridScale, mainGridPointSize, mainGridClosestPoint);
    float divGridColor = get_color_val_square_lines(fragCoord, divGridScale, divGridPointSize, divGridClosestPoint) * divGridAlphaFrac;
    float fin = max(mainGridColor, divGridColor);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* ruledShaderCode = R"V(
uniform vec4 gridColor;
uniform float mainGridScale;
uniform vec2 mainGridClosestPoint;
uniform float mainGridPointSize;

uniform float divGridAlphaFrac;
uniform float divGridScale;
uniform vec2 divGridClosestPoint;
uniform float divGridPointSize;

float get_color_val_ruled(float2 fCoord, float gScale, float gPointSize, float2 gClosestPoint) {
    float2 modCoord = mod(fCoord - gClosestPoint + float2(0.5 * gScale), float2(gScale));
    float2 difToPointCenter = abs(modCoord - float2(0.5 * gScale));
    return 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.y);
}
vec4 main(float2 fragCoord) {
    float mainGridColor = get_color_val_ruled(fragCoord, mainGridScale, mainGridPointSize, mainGridClosestPoint);
    float divGridColor = get_color_val_ruled(fragCoord, divGridScale, divGridPointSize, divGridClosestPoint) * divGridAlphaFrac;
    float fin = max(mainGridColor, divGridColor);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

sk_sp<SkRuntimeEffect> WorldGrid::compile_effect_shader_init(const char* shaderName, const char* shaderCode) {
    auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(shaderCode));
    if(!err.isEmpty()) {
        std::cout << "[GridManager::compile_effect_shader_init] " << shaderName << " shader construction error\n";
        std::cout << err.c_str() << std::endl;
        throw std::runtime_error("Shader Compile Failure");
    }
    return effect;
}

sk_sp<SkShader> WorldGrid::get_shader(GridType gType, const ShaderData& shaderData) {
    sk_sp<SkRuntimeEffect> runtimeEffect;
    switch(gType) {
        case GridType::CIRCLE_POINTS:
            runtimeEffect = circlePointEffect;
            break;
        case GridType::SQUARE_POINTS:
            runtimeEffect = squarePointEffect;
            break;
        case GridType::SQUARE_LINES:
            runtimeEffect = squareLinesEffect;
            break;
        case GridType::RULED:
            runtimeEffect = ruledEffect;
            break;
    }
    SkRuntimeShaderBuilder builder(runtimeEffect);
    builder.uniform("gridColor") = shaderData.gridColor;
    builder.uniform("mainGridScale") = std::min(shaderData.mainGridScale, 100000.0f); // Limit placed so that having a massive floating point value doesnt tank precision in the shader
    builder.uniform("mainGridClosestPoint").set(shaderData.mainGridClosestPoint.data(), 2);

    builder.uniform("divGridAlphaFrac") = shaderData.divGridAlphaFrac;
    builder.uniform("divGridScale") = std::min(shaderData.divGridScale, 100000.0f);
    builder.uniform("divGridClosestPoint").set(shaderData.divGridClosestPoint.data(), 2);
    switch(gType) {
        case GridType::CIRCLE_POINTS:
        case GridType::SQUARE_POINTS:
            builder.uniform("mainGridPointSize") = shaderData.mainGridPointSize;
            builder.uniform("divGridPointSize") = shaderData.divGridPointSize;
            break;
        case GridType::SQUARE_LINES:
        case GridType::RULED:
            builder.uniform("mainGridPointSize") = 0.5f;
            builder.uniform("divGridPointSize") = 0.5f;
            break;
    }
    sk_sp<SkShader> s = builder.makeShader();
    return s;
}

Vector2f WorldGrid::get_closest_grid_point(const WorldVec& gridOffset, const WorldScalar& gridSize, const DrawData& drawData) {
    WorldVec fracCamPosOnGrid;
    fracCamPosOnGrid.x() = (drawData.cam.c.pos.x() + gridOffset.x()) % gridSize;
    fracCamPosOnGrid.y() = (drawData.cam.c.pos.y() + gridOffset.y()) % gridSize;

    WorldVec closestGridPoint;
    closestGridPoint.x() = (fracCamPosOnGrid.x() < gridSize / WorldScalar(2)) ? WorldScalar(0) : gridSize;
    closestGridPoint.y() = (fracCamPosOnGrid.y() < gridSize / WorldScalar(2)) ? WorldScalar(0) : gridSize;

    Vector2f closestGridPointScreenPos;
    closestGridPointScreenPos.x() = static_cast<float>((closestGridPoint.x() - fracCamPosOnGrid.x()) / drawData.cam.c.inverseScale);
    closestGridPointScreenPos.y() = static_cast<float>((closestGridPoint.y() - fracCamPosOnGrid.y()) / drawData.cam.c.inverseScale);

    return closestGridPointScreenPos;
}

void WorldGrid::set_remove_divisions_outwards(bool v) {
    removeDivisionsOutwards = v;
    if(removeDivisionsOutwards)
        subdivisions = std::max<uint32_t>(subdivisions, 1);
}

bool WorldGrid::get_remove_divisions_outwards() {
    return removeDivisionsOutwards;
}

void WorldGrid::set_subdivisions(uint32_t v) {
    subdivisions = v;
    if(removeDivisionsOutwards)
        subdivisions = std::max<uint32_t>(subdivisions, 1);
}

uint32_t WorldGrid::get_subdivisions() {
    return subdivisions;
}

void WorldGrid::draw(GridManager& gMan, SkCanvas* canvas, const DrawData& drawData) {
    if(!visible)
        return;

    if(!circlePointEffect) {
        circlePointEffect = compile_effect_shader_init("Circle Point", circlePointShaderCode);
        squarePointEffect = compile_effect_shader_init("Square Point", squarePointShaderCode);
        squareLinesEffect = compile_effect_shader_init("Square Lines", squareLinesShaderCode);
        ruledEffect = compile_effect_shader_init("Ruled", ruledShaderCode);
    }

    unsigned subdivisionsTrue = get_subdivisions() + 1;

    WorldScalar sizeDetermineSubdivisionsToRemove = (drawData.cam.c.inverseScale / size) * WorldScalar(125);

    SkPaint linePaint;

    if(removeDivisionsOutwards && sizeDetermineSubdivisionsToRemove > WorldScalar(1)) {
        double divLogDouble, divLogFraction;
        divLogFraction = std::modf(static_cast<double>(FixedPoint::log(sizeDetermineSubdivisionsToRemove, WorldScalar(subdivisionsTrue))), &divLogDouble);
        uint64_t divLog = divLogDouble;
        WorldScalar logMultiplier = FixedPoint::exp_int_accurate<WorldScalar>(divLog, subdivisionsTrue);

        WorldScalar subDivSize = size * logMultiplier;
        WorldScalar divSize = subDivSize * WorldScalar(subdivisionsTrue);
        WorldScalar subDivWorldSize = subDivSize / drawData.cam.c.inverseScale;
        WorldScalar divWorldSize = divSize / drawData.cam.c.inverseScale;

        float floatDivWorldSize = static_cast<float>(divWorldSize);
        float floatSubDivWorldSize = static_cast<float>(subDivWorldSize);

        Vector2f closestGridPointScreenPos = get_closest_grid_point(offset, divSize, drawData);
        Vector2f subClosestGridPointScreenPos = get_closest_grid_point(offset, subDivSize, drawData);

        SkColor4f pointColor = gMan.world.canvasTheme.toolFrontColor;
        pointColor.fA = std::clamp(floatDivWorldSize / 100.0f, 0.0f, 0.6f);
        float gridPointSize = 5.0f;

        float finalDivMultiplier = (1.0 - divLogFraction) * ((divLog == 0) ? 0.5 : 1.0);

        linePaint.setShader(get_shader(gridType, {
            .gridColor = pointColor,
            .mainGridScale = floatDivWorldSize,
            .mainGridClosestPoint = closestGridPointScreenPos, 
            .mainGridPointSize = gridPointSize,

            .divGridAlphaFrac = finalDivMultiplier,
            .divGridScale = floatSubDivWorldSize,
            .divGridClosestPoint = subClosestGridPointScreenPos,
            .divGridPointSize = gridPointSize * finalDivMultiplier
        }));
    }
    else {
        WorldScalar subDivSize = size;
        WorldScalar divSize = subDivSize * WorldScalar(subdivisionsTrue);
        WorldScalar subDivWorldSize = subDivSize / drawData.cam.c.inverseScale;
        WorldScalar divWorldSize = divSize / drawData.cam.c.inverseScale;

        if(divWorldSize < WorldScalar(7))
            return;

        float floatDivWorldSize = static_cast<float>(divWorldSize);
        float floatSubDivWorldSize = static_cast<float>(subDivWorldSize);

        Vector2f closestGridPointScreenPos = get_closest_grid_point(offset, divSize, drawData);
        Vector2f subClosestGridPointScreenPos = get_closest_grid_point(offset, subDivSize, drawData);

        SkColor4f pointColor = gMan.world.canvasTheme.toolFrontColor;
        pointColor.fA = std::clamp(floatDivWorldSize / 100.0f, 0.0f, 0.6f);
        float gridPointSize = std::clamp(floatDivWorldSize / 10.0f, 0.0f, 5.0f);

        linePaint.setShader(get_shader(gridType, {
            .gridColor = pointColor,
            .mainGridScale = floatDivWorldSize,
            .mainGridClosestPoint = closestGridPointScreenPos, 
            .mainGridPointSize = gridPointSize,

            .divGridAlphaFrac = 0.5f,
            .divGridScale = floatSubDivWorldSize,
            .divGridClosestPoint = subClosestGridPointScreenPos,
            .divGridPointSize = gridPointSize * 0.5f
        }));
    }

    canvas->save();
    canvas->rotate(-drawData.cam.c.rotation * 180.0 / std::numbers::pi);
    canvas->drawPaint(linePaint);
    canvas->restore();
}
