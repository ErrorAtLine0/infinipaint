#pragma once
#include "Element.hpp"

namespace GUIStuff {

class SelectableButton : public Element {
    public:
        enum class DrawType {
            TRANSPARENT_ALL,
            FILLED,
            FILLED_INVERSE,
            TRANSPARENT_BORDER
        };

        struct InnerContentCallbackParameters {
            bool isSelected;
            bool isHovering;
            bool isHeld;
        };

        struct Data {
            DrawType drawType = DrawType::TRANSPARENT_ALL;
            bool isSelected = false;
            std::function<void()> onClick;
            std::function<void(const InnerContentCallbackParameters&)> innerContent;
        };

        SelectableButton(GUIManager& gui);
        void layout(const Clay_ElementId& id, const Data& d);
        virtual bool input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) override;
        virtual bool input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) override;

    private:
        bool isHovering = false;
        bool isHeld = false;
        std::function<void()> onClick;
};

}
