#include "Element.hpp"

namespace GUIStuff {
    Element::Element(GUIManager& initGUI):
        gui(initGUI) {}

    void Element::tick_update() {}

    bool Element::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) { return mouseHovering; }
    bool Element::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) { return mouseHovering; }
    bool Element::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel, bool mouseHovering) { return mouseHovering; }
    void Element::input_key_callback(const InputManager::KeyCallbackArgs& key) {}

    void Element::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {}

    void Element::update_bounding_box(Clay_RenderCommand* command) {
        boundingBox = get_bb_from_command(command);
    }

    void Element::clear_bounding_box() {
        boundingBox = std::nullopt;
    }

    const std::optional<SCollision::AABB<float>>& Element::get_bb() const {
        return boundingBox;
    }

    bool Element::collides_with_point(const Vector2f& p) const {
        if(!boundingBox.has_value())
            return false;
        return SCollision::collide(p, boundingBox.value());
    }

    SCollision::AABB<float> Element::get_bb_from_command(Clay_RenderCommand* command) {
        return {{command->boundingBox.x, command->boundingBox.y}, {command->boundingBox.x + command->boundingBox.width, command->boundingBox.y + command->boundingBox.height}};
    }
}
