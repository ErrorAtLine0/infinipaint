#include "FileResourceDisplay.hpp"
#include "../FontData.hpp"
#include "../GUIStuff/Elements/Element.hpp"
#include "../DrawData.hpp"
#include "../Toolbar.hpp"
#include "../MainProgram.hpp"
#include <include/core/SkStream.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFontMetrics.h>

std::string FILE_ICON_PATH = "data/icons/file.svg";

bool FileResourceDisplay::update_draw() const {
    return false;
}

void FileResourceDisplay::update(World& w) {
}

bool FileResourceDisplay::load(ResourceManager& rMan, const std::string& fileName, const std::string& fileData) {
    this->fileName = fileName;
    return true;
}

void FileResourceDisplay::draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) {
    std::shared_ptr<GUIStuff::UpdateInputData>& io = drawData.main->toolbar.io;
    auto findSVGData = io->svgData.find(FILE_ICON_PATH);
    sk_sp<SkSVGDOM> svgDom;
    if(findSVGData == io->svgData.end()) {
        auto stream = SkStream::MakeFromFile(FILE_ICON_PATH.c_str());
        if(!stream)
            throw std::runtime_error("[FileResourceDisplay::draw] Could not open file " + FILE_ICON_PATH);
        svgDom = SkSVGDOM::Builder().make(*stream);
        if(!svgDom)
            throw std::runtime_error("[SVGIcon::update] Could not parse SVG " + FILE_ICON_PATH);
    }
    else
        svgDom = findSVGData->second;

    SkPaint blendingPaint;
    blendingPaint.setColorFilter(SkColorFilters::Blend(drawData.main->world->canvasTheme.toolFrontColor, SkColorSpace::MakeSRGB(), SkBlendMode::kSrcIn));
    canvas->saveLayer(imRect, &blendingPaint);
    canvas->translate(imRect.x(), imRect.y());
    canvas->scale(imRect.width() / svgDom->containerSize().width(), imRect.height() / svgDom->containerSize().height());
    svgDom->render(canvas);
    canvas->restore();
}

Vector2f FileResourceDisplay::get_dimensions() const {
    return {100.0f, 100.0f};
}

float FileResourceDisplay::get_dimension_scale() const {
    return 0.1f;
}
