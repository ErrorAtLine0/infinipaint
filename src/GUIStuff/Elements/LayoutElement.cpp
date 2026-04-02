#include "LayoutElement.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

LayoutElement::LayoutElement(GUIManager& gui): Element(gui) {}

void LayoutElement::layout(const Clay_ElementId& id, const std::function<void(const Clay_ElementId&)>& layout, const Callbacks& c) {
    this->c = c;
    layout(id);
}

void LayoutElement::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
    if(c.mouseButton) c.mouseButton(button, mouseHovering);
    return Element::input_mouse_button_callback(button, mouseHovering);
}

void LayoutElement::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) {
    if(c.mouseMotion) c.mouseMotion(motion, mouseHovering);
    return Element::input_mouse_motion_callback(motion, mouseHovering);
}

void LayoutElement::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel, bool mouseHovering) {
    if(c.mouseWheel) c.mouseWheel(wheel, mouseHovering);
    return Element::input_mouse_wheel_callback(wheel, mouseHovering);
}

void LayoutElement::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(c.key) c.key(key);
}

}
