#include "SVGIcon.hpp"
#include "Helpers/ConvertVec.hpp"
#include <include/core/SkColorFilter.h>
#include <include/core/SkColorSpace.h>
#include <iostream>

namespace GUIStuff {

void SVGIcon::update(UpdateInputData& io, const std::string& newSvgPath, bool newIsHighlighted, const std::function<void()>& elemUpdate) {
    auto findSVGData = io.svgData.find(newSvgPath);
    if(findSVGData == io.svgData.end())
        throw std::runtime_error("[SVGIcon::update] Could not load icon " + newSvgPath);
    else
        svgDom = findSVGData->second;

    highlighted = newIsHighlighted;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
        },
        .custom = { .customData = this }
    }) {
        if(elemUpdate)
            elemUpdate();
    }
}

void SVGIcon::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
    if(!svgDom)
        return;

    auto bb = get_bb(command);

    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());
    SkRect r = SkRect::MakeXYWH(0.0f, 0.0f, bb.width(), bb.height());
    canvas->clipRect(r);
    SkPaint blendingPaint;
    blendingPaint.setColorFilter(SkColorFilters::Blend(highlighted ? io.theme->frontColor1 : io.theme->frontColor2, SkColorSpace::MakeSRGB(), SkBlendMode::kSrcIn));
    canvas->saveLayer(r, &blendingPaint);
    canvas->scale(bb.width() / svgDom->containerSize().width(), bb.height() / svgDom->containerSize().height());
    svgDom->render(canvas);
    canvas->restore();

    canvas->restore();
}

}
