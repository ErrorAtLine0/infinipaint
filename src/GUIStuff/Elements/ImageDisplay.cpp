#include "ImageDisplay.hpp"
#include "../GUIManager.hpp"
#include <include/codec/SkJpegDecoder.h>

namespace GUIStuff {

ImageDisplay::ImageDisplay(GUIManager& gui): Element(gui) {}

void ImageDisplay::layout(const Clay_ElementId& id, const std::filesystem::path& imgPath) {
    if(imgPath != oldImgPath) {
        std::string imgData;
        try {
            imgData = read_file_to_string(imgPath);
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
        canvas->drawImageRect(img, boundingBox.value().get_sk_rect(), {SkFilterMode::kLinear, SkMipmapMode::kNone});
    }
    else {
        SkPaint p{io.theme->frontColor1};
        p.setAntiAlias(skiaAA);
        canvas->drawRect(boundingBox.value().get_sk_rect(), p);
    }
}

}
