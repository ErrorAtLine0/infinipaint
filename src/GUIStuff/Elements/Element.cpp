/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Element.hpp"
#include "GUIStuffHelpers.hpp"

namespace GUIStuff {
    Element::Element(GUIManager& initGUI):
        gui(initGUI) {}

    void Element::update() {}
    void Element::deselect() {}

    void Element::input_paste_callback(const CustomEvents::PasteEvent& paste) { }
    void Element::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) { }
    void Element::input_text_key_callback(const InputManager::KeyCallbackArgs& key) { }
    void Element::input_text_callback(const InputManager::TextCallbackArgs& text) { }
    void Element::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) { }
    void Element::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) { }
    void Element::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) { }
    void Element::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
        input_mouse_button_callback(InputManager::convert_finger_touch_to_mouse_button(touch));
    }
    void Element::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
        input_mouse_motion_callback(InputManager::convert_finger_motion_to_mouse_motion(motion));
    }
    void Element::input_key_callback(const InputManager::KeyCallbackArgs& key) {}
    std::optional<InputManager::TextBoxStartInfo> Element::get_text_box_start_info() { return std::nullopt; }

    void Element::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {}

    void Element::clear_bb() {
        boundingBox = std::nullopt;
    }

    void Element::set_bounding_box_from_elem_data(const Clay_ElementData& elemData) {
        if(elemData.found)
            boundingBox = {{elemData.boundingBox.x, elemData.boundingBox.y}, {elemData.boundingBox.x + elemData.boundingBox.width, elemData.boundingBox.y + elemData.boundingBox.height}};
        else
            boundingBox = std::nullopt;
    }

    void Element::set_parent_clipping_region(const std::optional<SCollision::AABB<float>>& bb) {
        parentClippingRegion = bb;
    }

    const std::optional<SCollision::AABB<float>>& Element::get_bb() const {
        return boundingBox;
    }

    bool Element::collides_with_point(const Vector2f& p) const {
        if(!boundingBox.has_value())
            return false;
        bool inClippingRegion = !parentClippingRegion.has_value() || SCollision::collide(p, parentClippingRegion.value());
        return inClippingRegion && SCollision::collide(p, boundingBox.value());
    }

    SCollision::AABB<float> Element::get_bb_from_command(Clay_RenderCommand* command) {
        return clay_bounding_box_to_aabb(command->boundingBox);
    }
}
