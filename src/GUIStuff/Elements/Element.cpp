#include "Element.hpp"

namespace GUIStuff {
    Element::Element(GUIManager& initGUI):
        gui(initGUI) {}

    void Element::tick_update() {}

    void Element::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) { }
    void Element::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) { }
    void Element::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel, bool mouseHovering) { }
    void Element::input_key_callback(const InputManager::KeyCallbackArgs& key) {}

    void Element::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {}

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
        return {{command->boundingBox.x, command->boundingBox.y}, {command->boundingBox.x + command->boundingBox.width, command->boundingBox.y + command->boundingBox.height}};
    }
}
