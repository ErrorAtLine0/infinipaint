#pragma once
#include "../GUIManager.hpp"
#include "TextBoxHelpers.hpp"
#include "../Elements/NumberSlider.hpp"

namespace GUIStuff { namespace ElementHelpers {
    template <typename T> bool slider_scalar_field(GUIManager& gui, const char* id, std::string_view name, T* val, T min, T max, TextBoxScalarOptions options = {}) {
        gui.push_id(id);
        options.onEdit = [&, oE = options.onEdit] {
            oE();
            gui.set_to_layout();
        };
        left_to_right_line_layout([&]() {
            text_label(gui, name);
            input_scalar("textbox", val, min, max, options);
        });
        gui.element<NumberSlider<T>>("slider", val, min, max, options.onEdit);
        gui.pop_id();
    }
}}
