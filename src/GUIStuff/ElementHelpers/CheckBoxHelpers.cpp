#include "CheckBoxHelpers.hpp"
#include "LayoutHelpers.hpp"
#include "TextLabelHelpers.hpp"
#include "../Elements/CheckBox.hpp"

namespace GUIStuff { namespace ElementHelpers {

void checkbox_boolean(GUIManager& gui, const char* id, bool* d, const std::function<void()>& onClick) {
    gui.element<CheckBox>(id, [d]{ return *d; }, [d, onClick] {
        *d = !(*d);
        if(onClick)
            onClick();
    });
}

void checkbox_field(GUIManager& gui, const char* id, std::string_view name, const std::function<bool()>& isTicked, const std::function<void()>& onClick) {
    left_to_right_line_layout(gui, [&]() {
        gui.element<CheckBox>(id, isTicked, onClick);
        text_label(gui, name);
    });
}

void checkbox_boolean_field(GUIManager& gui, const char* id, std::string_view name, bool* d, const std::function<void()>& onClick) {
    left_to_right_line_layout(gui, [&]() {
        checkbox_boolean(gui, id, d, onClick);
        text_label(gui, name);
    });
}

}}
