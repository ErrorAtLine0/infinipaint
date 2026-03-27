#include "RadioButtonHelpers.hpp"
#include "../Elements/RadioButton.hpp"
#include "LayoutHelpers.hpp"
#include "TextLabelHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

void radio_button_field(GUIManager& gui, const char* id, std::string_view name, const std::function<bool()>& isTicked, const std::function<void()>& onClick) {
    left_to_right_line_layout(gui, [&]() {
        gui.element<RadioButton>(id, isTicked, onClick);
        text_label(gui, name);
    });
}

}}
