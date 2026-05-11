/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

bool FileResourceDisplay::load(ResourceManager& rMan, const std::string& fileName, const std::shared_ptr<std::string>& fileData) {
    this->fileName = fileName;
    return true;
}

void FileResourceDisplay::draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) {
    GUIStuff::UpdateInputData& io = drawData.main->g.gui.io;
    auto findSVGData = io.svgData.find(FILE_ICON_PATH);
    sk_sp<SkSVGDOM> svgDom;
    if(findSVGData == io.svgData.end())
        throw std::runtime_error("[FileResourceDisplay::draw] Could not load icon " + FILE_ICON_PATH);
    else
        svgDom = findSVGData->second;

    SkPaint blendingPaint;
    blendingPaint.setColorFilter(SkColorFilters::Blend(drawData.main->world->canvasTheme.get_tool_front_color(), SkColorSpace::MakeSRGB(), SkBlendMode::kSrcIn));
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

ResourceDisplay::Type FileResourceDisplay::get_type() const {
    return ResourceDisplay::Type::FILE;
}
