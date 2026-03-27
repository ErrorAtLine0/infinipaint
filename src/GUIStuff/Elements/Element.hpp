#pragma once
#include "GUIStuffHelpers.hpp"

namespace GUIStuff {
    class GUIManager;

    class Element {
        public:
            Element(GUIManager& initGUI);

            virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA);
            virtual bool collides_with_point(const Vector2f& p) const;

            void update_bounding_box(Clay_RenderCommand* command);
            void clear_bounding_box();
            const std::optional<SCollision::AABB<float>>& get_bb() const;
            virtual ~Element() = default;

            int16_t zIndex;

            virtual void tick_update();

            // Return true when mouse obstructs what's behind it
            virtual bool input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering);
            virtual bool input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering);
            virtual bool input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel, bool mouseHovering);
            virtual void input_key_callback(const InputManager::KeyCallbackArgs& key);
        protected:
            static SCollision::AABB<float> get_bb_from_command(Clay_RenderCommand* command);
            std::optional<SCollision::AABB<float>> boundingBox;

            GUIManager& gui;
    };
}
