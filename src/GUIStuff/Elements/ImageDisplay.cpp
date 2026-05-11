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

#include "ImageDisplay.hpp"
#include "../GUIManager.hpp"
#include <include/codec/SkJpegDecoder.h>
#include <include/core/SkRRect.h>

namespace GUIStuff {

ImageDisplay::ImageDisplay(GUIManager& gui): Element(gui) {}

void ImageDisplay::layout(const Clay_ElementId& id, const Data& data) {
    if(data != d) {
        d = data;
        std::string imgData;
        try {
            imgData = read_file_to_string(data.imgPath);
            auto codec = SkCodec::MakeFromData(SkData::MakeWithoutCopy(imgData.c_str(), imgData.size()), {
                SkJpegDecoder::Decoder()
            });
            img = std::get<0>(codec->getImage());
        } catch(...) {
            img = nullptr;
        }
        gui.invalidate_draw_element(this);
    }

    CLAY(id, {
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}},
        .custom = {.customData = this}
    }) {
    }
}

void ImageDisplay::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    if(img) {
        SkPaint p;
        p.setAntiAlias(skiaAA);
        if(d.radius != 0.0f) {
            canvas->save();
            SkRRect r = SkRRect::MakeRectXY(boundingBox.value().get_sk_rect(), d.radius, d.radius);
            canvas->clipRRect(r, skiaAA);
            canvas->drawImageRect(img, boundingBox.value().get_sk_rect(), {SkFilterMode::kLinear, SkMipmapMode::kNone}, &p);
            canvas->restore();
        }
        else
            canvas->drawImageRect(img, boundingBox.value().get_sk_rect(), {SkFilterMode::kLinear, SkMipmapMode::kNone}, &p);
    }
    else {
        SkPaint p{io.theme->frontColor1};
        p.setAntiAlias(skiaAA);
        SkRRect r = SkRRect::MakeRectXY(boundingBox.value().get_sk_rect(), d.radius, d.radius);
        canvas->drawRRect(r, p);
    }
}

}
