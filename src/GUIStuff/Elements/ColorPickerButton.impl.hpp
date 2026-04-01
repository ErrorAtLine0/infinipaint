#include "ColorPickerButton.decl.hpp"
#include "../ElementHelpers/ColorPickerItemsHelpers.hpp"
#include "../ElementHelpers/ButtonHelpers.hpp"
#include "../ElementHelpers/LayoutHelpers.hpp"

namespace GUIStuff {

template <typename T> ColorPickerButton<T>::ColorPickerButton(GUIManager& gui): Element(gui) {}

template <typename T> void ColorPickerButton<T>::layout(const Clay_ElementId& id, T* val, const ColorPickerButtonData& d) {
    this->data = d;

    GUIStuff::ElementHelpers::color_button(gui, "Color Picker Button", val, {
        .isSelected = isOpen,
        .onClick = [&]{
            isOpen = !isOpen;
            gui.set_to_layout();
        }
    });
    if(isOpen) {
        gui.set_z_index(gui.get_z_index() + 1, [&] {
            ElementHelpers::top_to_bottom_window_popup_layout(gui, "Color Picker", CLAY_SIZING_FIT(300), CLAY_SIZING_FIT(0), [&, val, data = data]() {
                ElementHelpers::color_picker_items(gui, "c", val, {
                    .hasAlpha = data.hasAlpha,
                    .onEdit = data.onEdit
                });
            }, {
                .mouseButton = [&](const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
                    if(!mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
                        isOpen = false;
                        gui.set_to_layout();
                    }
                    return mouseHovering;
                }
            });
        });
    }
}

}
