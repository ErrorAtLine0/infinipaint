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
#include "Element.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

class ScrollArea : public Element {
    public:
        ScrollArea(GUIManager& gui);

        struct InnerContentParameters {
            Vector2f contentDimensions;
            Vector2f containerDimensions;
            Vector2f* scrollOffset;
        };

        enum class ScrollbarType {
            NONE,
            NO_INTERACTION,
            NORMAL
        };

        struct Options {
            bool scrollVertical = false;
            bool scrollHorizontal = false;
            bool clipHorizontal = false;
            bool clipVertical = false;
            ScrollbarType scrollbarX = ScrollbarType::NONE;
            ScrollbarType scrollbarY = ScrollbarType::NONE;
            Clay_LayoutDirection layoutDirection = CLAY_TOP_TO_BOTTOM;
            Clay_LayoutAlignmentX xAlign = CLAY_ALIGN_X_LEFT;
            Clay_LayoutAlignmentY yAlign = CLAY_ALIGN_Y_TOP;
            std::function<void(const InnerContentParameters&)> innerContent;
        };

        void layout(const Clay_ElementId& id, const Options& options);
        void reset_scroll();


        virtual void update() override;
        virtual void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) override;
        virtual void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) override;
    private:
        Options opts;
        bool scrollAreaHeld = false;
        Vector2f scrollAreaMotion = {0.0f, 0.0f};
        Vector2f scrollAreaMotionMax = {0.0f, 0.0f};

        struct ScrollbarData {
            bool isScrollbarHeld = false;
            bool isScrollbarHovered = false;
            bool isHoveringOverScroller = false;
            Element* scroller = nullptr;
            Element* scrollbar = nullptr;
            float scrollerStartPos;
            float mouseStartPos;
        };

        ScrollbarData xScrollbar;
        ScrollbarData yScrollbar;

        void y_scroll_bar();
        void x_scroll_bar();

        Vector2f contentDimensions = {0.0f, 0.0f};
        Vector2f scrollOffset = {0.0f, 0.0f};
        void clamp_scroll();
};

}
