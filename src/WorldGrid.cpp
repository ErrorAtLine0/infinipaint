#include "WorldGrid.hpp"
#include "GridManager.hpp"
#include "MainProgram.hpp"

sk_sp<SkRuntimeEffect> WorldGrid::circlePointEffect;
sk_sp<SkRuntimeEffect> WorldGrid::squarePointEffect;
sk_sp<SkRuntimeEffect> WorldGrid::squareLinesEffect;
sk_sp<SkRuntimeEffect> WorldGrid::ruledEffect;
Vector2f WorldGrid::oldWindowSize = Vector2f{0.0f, 0.0f};

// NOTE: Skia shaders return premultiplied alpha colors

const char* circlePointShaderCode = R"V(
uniform vec4 gridColor;
uniform float gridScale;
uniform vec2 gridClosestPoint;
uniform float gridPointSize;

vec4 main(float2 fragCoord) {
    float2 modCoord = mod(fragCoord - gridClosestPoint + float2(0.5 * gridScale), float2(gridScale));
    float fin = step(distance(modCoord, float2(0.5 * gridScale)), gridPointSize);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* squarePointShaderCode = R"V(
uniform vec4 gridColor;
uniform float gridScale;
uniform vec2 gridClosestPoint;
uniform float gridPointSize;

vec4 main(float2 fragCoord) {
    float2 modCoord = mod(fragCoord - gridClosestPoint + float2(0.5 * gridScale), float2(gridScale));
    float2 difToPointCenter = abs(modCoord - float2(0.5 * gridScale));
    float m1 = step(difToPointCenter.x, gridPointSize);
    float m2 = step(difToPointCenter.y, gridPointSize);
    float fin = m1 * m2;

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* squareLinesShaderCode = R"V(
uniform vec4 gridColor;
uniform float gridScale;
uniform vec2 gridClosestPoint;
uniform float gridPointSize;

vec4 main(float2 fragCoord) {
    float2 modCoord = mod(fragCoord - gridClosestPoint + float2(0.5 * gridScale), float2(gridScale));
    float2 difToPointCenter = abs(modCoord - float2(0.5 * gridScale));
    float m1 = step(difToPointCenter.x, gridPointSize);
    float m2 = step(difToPointCenter.y, gridPointSize);
    float fin = max(m1, m2);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* ruledShaderCode = R"V(
uniform vec4 gridColor;
uniform float gridScale;
uniform vec2 gridClosestPoint;
uniform float gridPointSize;

vec4 main(float2 fragCoord) {
    float2 modCoord = mod(fragCoord - gridClosestPoint + float2(0.5 * gridScale), float2(gridScale));
    float2 difToPointCenter = abs(modCoord - float2(0.5 * gridScale));
    float fin = step(difToPointCenter.y, gridPointSize);

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

sk_sp<SkShader> WorldGrid::get_shader(GridType gType, const SkColor4f& gridColor, float gridScale, const Vector2f& gridClosestPoint, float gridPointSize) {
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
    builder.uniform("gridColor") = gridColor;
    builder.uniform("gridScale") = std::min(gridScale, 100000.0f); // Limit placed so that having a massive floating point value doesnt tank precision in the shader
    builder.uniform("gridClosestPoint").set(gridClosestPoint.data(), 2);
    switch(gType) {
        case GridType::CIRCLE_POINTS:
            builder.uniform("gridPointSize") = gridPointSize;
            break;
        case GridType::SQUARE_POINTS:
            builder.uniform("gridPointSize") = gridPointSize;
            break;
        case GridType::SQUARE_LINES:
            builder.uniform("gridPointSize") = 0.5f;
            break;
        case GridType::RULED:
            builder.uniform("gridPointSize") = 0.5f;
            break;
    }
    sk_sp<SkShader> s = builder.makeShader();
    return s;
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

    WorldScalar worldSize = size / drawData.cam.c.inverseScale;
    float floatWorldSize = static_cast<float>(worldSize);
    if(worldSize > WorldScalar(7)) {
        WorldVec fracCamPosOnGrid;
        fracCamPosOnGrid.x() = (drawData.cam.c.pos.x() + offset.x()) % size;
        fracCamPosOnGrid.y() = (drawData.cam.c.pos.y() + offset.y()) % size;

        WorldVec closestGridPoint;
        closestGridPoint.x() = (fracCamPosOnGrid.x() < size / WorldScalar(2)) ? WorldScalar(0) : size;
        closestGridPoint.y() = (fracCamPosOnGrid.y() < size / WorldScalar(2)) ? WorldScalar(0) : size;

        Vector2f closestGridPointScreenPos;
        closestGridPointScreenPos.x() = static_cast<float>((closestGridPoint.x() - fracCamPosOnGrid.x()) / drawData.cam.c.inverseScale);
        closestGridPointScreenPos.y() = static_cast<float>((closestGridPoint.y() - fracCamPosOnGrid.y()) / drawData.cam.c.inverseScale);

        std::cout << "frac cam point: " << vec_pretty(fracCamPosOnGrid) << std::endl;
        std::cout << "closest grid point: " << vec_pretty(closestGridPoint) << std::endl;
        std::cout << "closest grid point screen: " << vec_pretty(closestGridPointScreenPos) << std::endl;
        std::cout << "worldSize: " << floatWorldSize << std::endl;

        SkPaint linePaint;
        SkColor4f pointColor = gMan.world.canvasTheme.toolFrontColor;
        pointColor.fA = std::clamp(floatWorldSize / 50.0f, 0.0f, 0.4f);
        float gridPointSize = std::clamp(floatWorldSize / 10.0f, 0.0f, 5.0f);
        linePaint.setShader(get_shader(gridType, pointColor, static_cast<float>(worldSize), closestGridPointScreenPos, gridPointSize));
        canvas->save();
        canvas->rotate(-drawData.cam.c.rotation * 180.0 / std::numbers::pi);
        canvas->drawPaint(linePaint);
        canvas->restore();
    }
}
