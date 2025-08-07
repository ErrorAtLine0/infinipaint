#include "SvgResourceDisplay.hpp"
#include "../MainProgram.hpp"
#include <Helpers/Logger.hpp>
#include <include/core/SkStream.h>
#include <modules/svg/include/SkSVGRenderContext.h>

bool SvgResourceDisplay::update_draw() const {
    return mustUpdateDraw;
}

void SvgResourceDisplay::update(World& w) {
    mustUpdateDraw = false;
}

bool SvgResourceDisplay::load(const std::string& fileName, const std::string& fileData) {
    SkMemoryStream strm(fileData.c_str(), fileData.size());
    svgDom = SkSVGDOM::Builder().make(strm);

    mustUpdateDraw = true;

    if(!svgDom)
        return false;

    svgRootSize = svgDom->containerSize();

    return true;
}

void SvgResourceDisplay::draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) {
    canvas->save();
    canvas->translate(imRect.x(), imRect.y());
    canvas->scale(imRect.width() / svgRootSize.width(), imRect.height() / svgRootSize.height());
    svgDom->render(canvas);
    canvas->restore();
}

Vector2f SvgResourceDisplay::get_dimensions() const {
    return {svgRootSize.width(), svgRootSize.height()};
}

float SvgResourceDisplay::get_dimension_scale() const {
    return 0.3f;
}
