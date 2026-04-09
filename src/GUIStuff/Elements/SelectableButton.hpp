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
            bool instantResponse = false;
            std::function<void()> onClick;
            std::function<void(SelectableButton*)> onClickButton;
            std::function<void(const InnerContentCallbackParameters&)> innerContent;
        };

        SelectableButton(GUIManager& gui);
        void layout(const Clay_ElementId& id, const Data& d);
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;

    private:
        bool instantResponse = false;
        bool isHeld = false;
        bool oldIsHovering = false;
        std::function<void()> onClick;
};

}
