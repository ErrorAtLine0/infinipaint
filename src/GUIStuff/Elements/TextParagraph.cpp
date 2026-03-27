#include "TextParagraph.hpp"
#include <include/core/SkPath.h>
#include <iostream>

namespace GUIStuff {

TextParagraph::TextParagraph(GUIManager& gui):
    Element(gui) {}

void TextParagraph::layout(std::unique_ptr<skia::textlayout::Paragraph> paragraph, float width) {
    data = std::move(paragraph);
    data->layout(width);
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(width), .height = CLAY_SIZING_FIXED(data->getHeight())}
        },
        .custom = { .customData = this }
    }) {}
}

void TextParagraph::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    if(data) {
        auto& bb = boundingBox.value();
        data->paint(canvas, bb.min.x(), bb.min.y());
    }
}

}
