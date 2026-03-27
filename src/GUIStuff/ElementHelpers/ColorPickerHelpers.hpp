#pragma once
#include "../GUIManager.hpp"
#include "TextBoxHelpers.hpp"
#include "../Elements/ColorPicker.hpp"
#include "../Elements/ColorPickerButton.decl.hpp"

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
    gui.element<ColorPicker>("c", val, options.hasAlpha, fullOnEdit);
    left_to_right_line_layout([&]() {
        text_label(gui, "R");
        input_color_component_255("r", &(*val)[0], { .onEdit = fullOnEdit });
        text_label(gui, "G");
        input_color_component_255("g", &(*val)[1], { .onEdit = fullOnEdit });
        text_label(gui, "B");
        input_color_component_255("b", &(*val)[2], { .onEdit = fullOnEdit });
        if(options.hasAlpha) {
            text_label(gui, "A");
            input_color_component_255("a", &(*val)[3], { .onEdit = fullOnEdit });
        }
    });
    left_to_right_line_layout([&]() {
        text_label(gui, "Hex");
        input_color_hex("h", val, { .onEdit = fullOnEdit, .hasAlpha = options.hasAlpha });
    });
    gui.pop_id();
}

template <typename T> void color_picker_button_field(GUIManager& gui, const char* id, std::string_view name, T* val, const ColorPickerButtonData& options = {}) {
    left_to_right_line_layout([&]() {
        gui.element<ColorPickerButton<T>>(id, val, options);
        text_label(gui, name);
    });
}

} }
