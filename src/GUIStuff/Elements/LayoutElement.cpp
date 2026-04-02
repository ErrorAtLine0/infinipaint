#include "LayoutElement.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

LayoutElement::LayoutElement(GUIManager& gui): Element(gui) {}

void LayoutElement::layout(const Clay_ElementId& id, const std::function<void(LayoutElement*, const Clay_ElementId&)>& layout, const Callbacks& c) {
    this->c = c;
    layout(this, id);
}

void LayoutElement::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(c.mouseButton) c.mouseButton(this, button);
}

void LayoutElement::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(c.mouseMotion) c.mouseMotion(this, motion);
}

void LayoutElement::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    if(c.mouseWheel) c.mouseWheel(this, wheel);
}

void LayoutElement::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(c.key) c.key(this, key);
}

}
