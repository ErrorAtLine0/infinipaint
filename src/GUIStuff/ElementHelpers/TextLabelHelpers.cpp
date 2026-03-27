#include "TextLabelHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

void text_label_size(GUIManager& gui, std::string_view val, float modifier) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io->theme->frontColor1), .fontSize = static_cast<uint16_t>(gui.io->fontSize * modifier)}));
}

void text_label_color(GUIManager& gui, std::string_view val, const SkColor4f& color) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(color), .fontSize = gui.io->fontSize }));
}

void text_label_light(GUIManager& gui, std::string_view val) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io->theme->frontColor2), .fontSize = gui.io->fontSize }));
}

void text_label(GUIManager& gui, std::string_view val) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io->theme->frontColor1), .fontSize = gui.io->fontSize }));
}

void text_label_centered(GUIManager& gui, std::string_view val) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        }
    }) {
        CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io->theme->frontColor1), .fontSize = gui.io->fontSize, .textAlignment = CLAY_TEXT_ALIGN_CENTER}));
    }
}

void text_label_light_centered(GUIManager& gui, std::string_view val) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        }
    }) {
        CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io->theme->frontColor2), .fontSize = gui.io->fontSize, .textAlignment = CLAY_TEXT_ALIGN_CENTER}));
    }
}

}}
