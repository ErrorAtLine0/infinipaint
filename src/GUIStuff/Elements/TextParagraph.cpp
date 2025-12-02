#include "TextParagraph.hpp"
#include <include/core/SkPath.h>
#include <iostream>

namespace GUIStuff {

void TextParagraph::update(UpdateInputData& io, std::unique_ptr<skia::textlayout::Paragraph> paragraph, float width, const std::function<void()>& elemUpdate) {
    data = std::move(paragraph);
    data->layout(width);
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(width), .height = CLAY_SIZING_FIXED(data->getHeight())}
        },
        .custom = { .customData = this }
    }) {
        if(elemUpdate)
            elemUpdate();
    }
}

void TextParagraph::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
    bb = get_bb(command);

    if(data)
        data->paint(canvas, bb.min.x(), bb.min.y());
}

}
