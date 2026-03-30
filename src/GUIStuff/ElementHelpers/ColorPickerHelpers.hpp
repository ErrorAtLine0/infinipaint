#pragma once
#include "ColorPickerItemsHelpers.hpp"
#include "../Elements/ColorPickerButton.hpp"

namespace GUIStuff { namespace ElementHelpers {

template <typename T> void color_picker_button_field(GUIManager& gui, const char* id, std::string_view name, T* val, const ColorPickerButtonData& options = {}) {
    left_to_right_line_layout(gui, [&]() {
        gui.element<ColorPickerButton<T>>(id, val, options);
        text_label(gui, name);
    });
}

} }
