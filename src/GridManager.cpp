#include "GridManager.hpp"
#include "MainProgram.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkPathTypes.h>
#include <Helpers/MathExtras.hpp>

#define GRID_CACHE_SIZE 10.0f
#define GRID_CACHE_SIZE_INT 10
#define GRID_DOT_SIZE 0.5f

const char* squareDotGridSkSl =
R"V(half4 main(float2 fragCoord) {
  	float m1 = step(mod(fragCoord.x + 0.25, 10.0), 0.5);
    float m2 = step(mod(fragCoord.y + 0.25, 10.0), 0.5);
    float fin = m1 * m2;
  	return half4(fin);
})V";

const char* circleDotGridSkSl =
R"V(half4 main(float2 fragCoord) {
    float2 modCoord = mod(fragCoord + float2(5.0), float2(10.0));
    float fin = step(distance(float2(5.0), modCoord), 0.25);

  	return half4(fin);
})V";

GridManager::GridManager(MainProgram& initMain):
    main(initMain)
{
    auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(circleDotGridSkSl));
    if(!err.isEmpty()) {
       std::cout << "[GridManager::GridManager] Circle dot shader construction error\n";
       std::cout << err.c_str() << std::endl;
    }
    else
       circleDotShader = effect->makeShader(nullptr, {nullptr, 0});

    auto [effect2, err2] = SkRuntimeEffect::MakeForShader(SkString(squareDotGridSkSl));
    if(!err2.isEmpty()) {
       std::cout << "[GridManager::GridManager] Square dot shader construction error\n";
       std::cout << err2.c_str() << std::endl;
    }
    else
       squareDotShader = effect2->makeShader(nullptr, {nullptr, 0});
}

void GridManager::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(gridType == GRIDTYPE_NONE)
        return;
    if(oldWindowSize != main.window.size.cast<float>()) {
        gridPath = SkPath();
        float maxDim = std::max(main.window.size.x(), main.window.size.y()) * 2.0f;
        for(float i = 0.0f; i < maxDim; i += GRID_CACHE_SIZE) {
            gridPath.moveTo(i, -maxDim);
            gridPath.lineTo(i, maxDim);
            gridPath.moveTo(-maxDim, i);
            gridPath.lineTo(maxDim,  i);
        }
        for(float i = 0.0f; i > -maxDim; i -= GRID_CACHE_SIZE) {
            gridPath.moveTo(i, -maxDim);
            gridPath.lineTo(i, maxDim);
            gridPath.moveTo(-maxDim, i);
            gridPath.lineTo(maxDim,  i);
        }
        oldWindowSize = main.window.size.cast<float>();
    }
    float alteredZoom = static_cast<float>(((size / WorldScalar(GRID_CACHE_SIZE_INT)) / drawData.cam.c.inverseScale));
    if(alteredZoom > 0.5f) {
        Vector2f fracCamPos;

        fracCamPos.x() = static_cast<float>((drawData.cam.c.pos.x() % size) / drawData.cam.c.inverseScale);
        fracCamPos.y() = static_cast<float>((drawData.cam.c.pos.y() % size) / drawData.cam.c.inverseScale);

        SkPaint linePaint;
        if(gridType == GRIDTYPE_CIRCLEDOT || gridType == GRIDTYPE_SQUAREDOT) {
            linePaint.setShader(gridType == 1 ? circleDotShader : squareDotShader);
            linePaint.setAlphaf(std::clamp(alteredZoom / 20.0f, 0.0f, 0.4f));
        }
        else if(gridType == GRIDTYPE_LINE) {
            linePaint.setColor4f({1.0f, 1.0f, 1.0f, 1.0f});
            linePaint.setStyle(SkPaint::kStroke_Style);
            linePaint.setStrokeWidth(0.0f);
            linePaint.setAlphaf(std::clamp(alteredZoom / 35.0f, 0.0f, 0.3f));
        }

        canvas->save();

        canvas->rotate(-drawData.cam.c.rotation * 180.0 / std::numbers::pi);
        canvas->translate(-fracCamPos.x(), -fracCamPos.y());
        canvas->scale(alteredZoom, alteredZoom);

        if(gridType == GRIDTYPE_CIRCLEDOT || gridType == GRIDTYPE_SQUAREDOT)
            canvas->drawPaint(linePaint);
        else if(gridType == GRIDTYPE_LINE)
            canvas->drawPath(gridPath, linePaint);

        canvas->restore();
    }
}
