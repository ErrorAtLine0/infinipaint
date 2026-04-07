#include "TextParagraph.hpp"
#include <include/core/SkPath.h>
#include <iostream>
#include "../GUIManager.hpp"

namespace GUIStuff {

TextParagraph::TextParagraph(GUIManager& gui):
    Element(gui) {}

void TextParagraph::layout(const Clay_ElementId& id, const Data& newData) {
    if(!textbox) {
        d = newData;
        textbox = std::make_unique<RichText::TextBox>();
        textbox->set_width(d.width);
        textbox->set_rich_text_data(d.text);
        textbox->set_font_data(gui.io.fonts);
        textbox->set_allow_newlines(true);
    }

    if(d.width != newData.width) {
        d.width = newData.width;
        textbox->set_width(d.width);
    }

    if(d.text != newData.text) {
        d.text = newData.text;
        textbox->set_rich_text_data(d.text);
    }

    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(d.width), .height = CLAY_SIZING_FIXED(textbox->get_height())}
        },
        .custom = { .customData = this }
    }) {}
}

void TextParagraph::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();
    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());
    skia::textlayout::TextStyle tStyle;
    tStyle.setFontFamilies(io.fonts->get_default_font_families());
    tStyle.setFontSize(io.fontSize);
    tStyle.setForegroundPaint(SkPaint{io.theme->frontColor1});
    textbox->set_initial_text_style(tStyle);
    textbox->paint(canvas, { .skiaAA = skiaAA });
    canvas->restore();
}

}
