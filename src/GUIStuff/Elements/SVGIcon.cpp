#include "SVGIcon.hpp"
#include "Helpers/ConvertVec.hpp"
#include <include/core/SkColorFilter.h>
#include <include/core/SkColorSpace.h>
#include <iostream>
#include "../GUIManager.hpp"
#include "Helpers/MathExtras.hpp"

namespace GUIStuff {

SVGIcon::SVGIcon(GUIManager& gui):
    Element(gui) {}

void SVGIcon::layout(const Clay_ElementId& id, const std::string& newSvgPath, bool newIsHighlighted) {
    auto& io = gui.io;
    auto findSVGData = io.svgData.find(newSvgPath);
    if(findSVGData == io.svgData.end())
        throw std::runtime_error("[SVGIcon::update] Could not load icon " + newSvgPath);
    else
        svgDom = findSVGData->second;

    highlighted = newIsHighlighted;

    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
        },
        .custom = { .customData = this }
    }) {}
}

void SVGIcon::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    if(!svgDom)
        return;

    auto& bb = boundingBox.value();

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
