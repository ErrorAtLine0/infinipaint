#include "Element.hpp"
#include "GUIStuffHelpers.hpp"

namespace GUIStuff {
    Element::Element(GUIManager& initGUI):
        gui(initGUI) {}

    void Element::update() {}
    void Element::deselect() {}

    void Element::input_paste_callback(const CustomEvents::PasteEvent& paste) { }
    void Element::input_text_key_callback(const InputManager::KeyCallbackArgs& key) { }
    void Element::input_text_callback(const InputManager::TextCallbackArgs& text) { }
    void Element::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) { }
    void Element::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) { }
    void Element::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) { }
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
        return clay_bounding_box_to_aabb(command->boundingBox);
    }
}
