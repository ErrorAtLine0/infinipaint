#include "WorldGrid.hpp"
#include "GridManager.hpp"
#include "MainProgram.hpp"

sk_sp<SkRuntimeEffect> WorldGrid::circlePointEffect;
sk_sp<SkRuntimeEffect> WorldGrid::squarePointEffect;
sk_sp<SkRuntimeEffect> WorldGrid::squareLinesEffect;
sk_sp<SkRuntimeEffect> WorldGrid::ruledEffect;
Vector2f WorldGrid::oldWindowSize = Vector2f{0.0f, 0.0f};

#define GRID_CACHE_SIZE 10.0f
#define GRID_CACHE_SIZE_INT 10

// NOTE: Skia shaders return premultiplied alpha colors

const char* circlePointShaderCode = R"V(
uniform vec4 gridColor;
uniform float gridScale;

vec4 main(float2 fragCoord) {
    float circleRadius = min(5.0 / gridScale, 0.25);
    float2 modCoord = mod(fragCoord + float2(5.0), float2(10.0));
    float fin = step(distance(float2(5.0), modCoord), circleRadius);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* squarePointShaderCode = R"V(
uniform vec4 gridColor;
uniform float gridScale;

vec4 main(float2 fragCoord) {
    float squareSize = min(10.0 / gridScale, 0.5);
    float gridUnit = 1.0 / gridScale;
  	float m1 = step(mod(fragCoord.x + gridUnit + squareSize * 0.5, 10.0), squareSize);
    float m2 = step(mod(fragCoord.y + gridUnit + squareSize * 0.5, 10.0), squareSize);
    float fin = m1 * m2;

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* squareLinesShaderCode = R"V(
uniform vec4 gridColor;
uniform float gridScale;

vec4 main(float2 fragCoord) {
    float gridUnit = 1.0 / gridScale;
  	float m1 = step(mod(fragCoord.x + gridUnit, 10.0), gridUnit);
    float m2 = step(mod(fragCoord.y + gridUnit, 10.0), gridUnit);
    float fin = max(m1, m2);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* ruledShaderCode = R"V(
uniform vec4 gridColor;
uniform float gridScale;

vec4 main(float2 fragCoord) {
    float gridUnit = 1.0 / gridScale;
    float fin = step(mod(fragCoord.y + gridUnit, 10.0), gridUnit);

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

sk_sp<SkShader> WorldGrid::get_shader(GridType gType, const SkColor4f& gridColor, float gridScale) {
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
    builder.uniform("gridScale") = gridScale;
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

    float alteredZoom = static_cast<float>(((size / WorldScalar(GRID_CACHE_SIZE_INT)) / drawData.cam.c.inverseScale));
    if(alteredZoom > 0.4f) {
        Vector2f fracCamPos;

        fracCamPos.x() = static_cast<float>((drawData.cam.c.pos.x() % size) / drawData.cam.c.inverseScale);
        fracCamPos.y() = static_cast<float>((drawData.cam.c.pos.y() % size) / drawData.cam.c.inverseScale);

        SkPaint linePaint;
        SkColor4f pointColor = gMan.world.canvasTheme.toolFrontColor;
        pointColor.fA = std::clamp(alteredZoom / 20.0f, 0.0f, 0.4f);
        linePaint.setShader(get_shader(gridType, pointColor, alteredZoom));

        canvas->save();

        canvas->rotate(-drawData.cam.c.rotation * 180.0 / std::numbers::pi);
        canvas->translate(-fracCamPos.x(), -fracCamPos.y());
        canvas->scale(alteredZoom, alteredZoom);

        canvas->drawPaint(linePaint);

        canvas->restore();
    }
}
