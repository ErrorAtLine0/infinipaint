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
            void clear_bb();
            virtual ~Element() = default;

            int16_t zIndex;
            bool mouseHovering = false;
            bool childMouseHovering = false;

            Element* parent = nullptr;

            virtual void update();
            virtual void deselect();

            virtual void input_paste_callback(const CustomEvents::PasteEvent& paste);
            virtual void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput);
            virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
            virtual void input_text_callback(const InputManager::TextCallbackArgs& text);
            virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button);
            virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
            virtual void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel);
            virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch);
            virtual void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion);
            virtual void input_key_callback(const InputManager::KeyCallbackArgs& key);
            virtual std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info();
        protected:
            static SCollision::AABB<float> get_bb_from_command(Clay_RenderCommand* command);
            std::optional<SCollision::AABB<float>> boundingBox;
            std::optional<SCollision::AABB<float>> parentClippingRegion;

            GUIManager& gui;
    };
}
