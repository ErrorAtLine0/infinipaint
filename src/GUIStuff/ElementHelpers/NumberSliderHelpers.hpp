#pragma once
#include "../GUIManager.hpp"
#include "TextBoxHelpers.hpp"
#include "../Elements/NumberSlider.hpp"

namespace GUIStuff { namespace ElementHelpers {
    template <typename T> void slider_scalar_field(GUIManager& gui, const char* id, std::string_view name, T* val, T min, T max, TextBoxScalarOptions options = {}) {
        gui.new_id(id, [&] {
            options.onEdit = [&, oE = options.onEdit] {
                if(oE) oE();
                gui.set_to_layout();
            };
            left_to_right_line_layout(gui, [&]() {
                text_label(gui, name);
                input_scalar(gui, "textbox", val, min, max, options);
            });
            gui.element<NumberSlider<T>>("slider", val, min, max, NumberSliderData{
                .onChange = options.onEdit,
                .onHold = options.onSelect,
                .onRelease = options.onDeselect
            });
        });
    }
}}
