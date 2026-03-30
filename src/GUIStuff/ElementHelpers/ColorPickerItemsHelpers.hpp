#pragma once
#include "../GUIManager.hpp"
#include "TextBoxHelpers.hpp"
#include "../Elements/ColorPicker.hpp"

namespace GUIStuff { namespace ElementHelpers {

struct ColorPickerItemsOptions {
    bool hasAlpha = true;
    std::function<void()> onEdit;
};

template <typename T> void color_picker_items(GUIManager& gui, const char* id, T* val, const ColorPickerItemsOptions& options = {}) {
    gui.push_id(id);
    auto fullOnEdit = [&gui, oE = options.onEdit, val]() {
        oE();
        gui.set_to_layout();
    };
    gui.element<ColorPicker<T>>("c", val, options.hasAlpha, fullOnEdit);
    left_to_right_line_layout(gui, [&]() {
        text_label(gui, "R");
        input_color_component_255(gui, "r", &(*val)[0], { .onEdit = fullOnEdit });
        text_label(gui, "G");
        input_color_component_255(gui, "g", &(*val)[1], { .onEdit = fullOnEdit });
        text_label(gui, "B");
        input_color_component_255(gui, "b", &(*val)[2], { .onEdit = fullOnEdit });
        if(options.hasAlpha) {
            text_label(gui, "A");
            input_color_component_255(gui, "a", &(*val)[3], { .onEdit = fullOnEdit });
        }
    });
    left_to_right_line_layout(gui, [&]() {
        text_label(gui, "Hex");
        input_color_hex(gui, "h", val, { .hasAlpha = options.hasAlpha, .onEdit = fullOnEdit });
    });
    gui.pop_id();
}

} }
