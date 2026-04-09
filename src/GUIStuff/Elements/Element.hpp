#pragma once
#include "GUIStuffHelpers.hpp"

namespace GUIStuff {
    class GUIManager;

    class Element {
        public:
            Element(GUIManager& initGUI);

            virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA);
            virtual bool collides_with_point(const Vector2f& p) const;

            void set_bounding_box_from_elem_data(const Clay_ElementData& elemData);
            void set_parent_clipping_region(const std::optional<SCollision::AABB<float>>& bb);
            const std::optional<SCollision::AABB<float>>& get_bb() const;
            virtual ~Element() = default;

            int16_t zIndex;
            bool mouseHovering = false;
            bool childMouseHovering = false;

            Element* parent = nullptr;

            virtual void update();
            virtual void deselect();

            virtual void input_paste_callback(const CustomEvents::PasteEventData& paste);
            virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
            virtual void input_text_callback(const InputManager::TextCallbackArgs& text);
            virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button);
            virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
            virtual void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel);
            virtual void input_key_callback(const InputManager::KeyCallbackArgs& key);
        protected:
            static SCollision::AABB<float> get_bb_from_command(Clay_RenderCommand* command);
            std::optional<SCollision::AABB<float>> boundingBox;
            std::optional<SCollision::AABB<float>> parentClippingRegion;

            GUIManager& gui;
    };
}
