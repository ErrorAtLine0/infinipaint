#include "ButtonHelpers.hpp"
#include "../Elements/SVGIcon.hpp"
#include "TextLabelHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

void text_button(GUIManager& gui, const char* id, std::string_view text, const TextButtonOptions& options) {
    SelectableButton::Data d = selectable_button_options_to_data(options);
    d.innerContent = [&gui, text, centered = options.centered] (const SelectableButton::InnerContentCallbackParameters&) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                .childAlignment = {.x = centered ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
            }
        }) {
            text_label(gui, text);
        }
    };
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = options.wide ? CLAY_SIZING_GROW(0) : CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = options.centered ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
        }
    }) {
        gui.element<SelectableButton>(id, d);
    }
}

void text_button_sized(GUIManager& gui, const char* id, std::string_view text, Clay_SizingAxis x, Clay_SizingAxis y, const TextButtonOptions& options) {
    SelectableButton::Data d = selectable_button_options_to_data(options);
    d.innerContent = [&gui, text] (const SelectableButton::InnerContentCallbackParameters&) {
        text_label(gui, text);
    };
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = x, .height = y },
            .childAlignment = {.x = options.centered ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
        }
    }) {
        gui.element<SelectableButton>(id, d);
    }
}

void svg_icon_button(GUIManager& gui, const char* id, const std::string& svgPath, const SVGButtonOptions& options) {
    SelectableButton::Data d = selectable_button_options_to_data(options);
    d.innerContent = [&gui, &svgPath] (const SelectableButton::InnerContentCallbackParameters& p) {
        gui.element<SVGIcon>("icon", svgPath, p.isSelected || p.isHovering || p.isHeld);
    };
    CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(options.size), .height = CLAY_SIZING_FIXED(options.size) } } }) {
        gui.element<SelectableButton>(id, d);
    }
}

}}
