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
        void reset_scroll();

        virtual void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) override;
    private:
        Options opts;
        bool isScrollbarHeldY = false;
        bool isScrollbarHoveredY = false;
        bool isHoveringOverScrollerY = false;
        float scrollerStartPosY;
        float mouseStartPosY;

        Vector2f contentDimensions = {0.0f, 0.0f};
        Vector2f containerDimensions = {0.0f, 0.0f};
        Vector2f scrollOffset = {0.0f, 0.0f};
        void clamp_scroll();
};

}
