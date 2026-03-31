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
            Vector2f scrollOffset;
        };

        struct Options {
            bool scrollVertical = false;
            bool scrollHorizontal = false;
            bool clipHorizontal = false;
            bool clipVertical = false;
            bool showScrollbarX = false;
            bool showScrollbarY = false;
            Clay_LayoutDirection layoutDirection = CLAY_TOP_TO_BOTTOM;
            std::function<void(const InnerContentParameters&)> innerContent;
        };

        void layout(const Clay_ElementId& id, const Options& options);

        virtual bool input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) override;
        virtual bool input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) override;
        virtual bool input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel, bool mouseHovering) override;
    private:
        Options opts;
        bool isScrollbarXHeld = false;
        bool isScrollbarYHeld = false;
        Vector2f contentDimensions = {0.0f, 0.0f};
        Vector2f containerDimensions = {0.0f, 0.0f};
        Vector2f scrollOffset = {0.0f, 0.0f};
        void clamp_scroll();
};

}
