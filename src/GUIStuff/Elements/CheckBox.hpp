#pragma once
#include "Element.hpp"

namespace GUIStuff {

class CheckBox : public Element {
    public:
        CheckBox(GUIManager& gui);

        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;

        void layout(const Clay_ElementId& id, const std::function<bool()>& isTicked, const std::function<void()>& onClick);
    private:
        static constexpr float CHECKBOX_ANIMATION_TIME = 0.3;
        float hoverAnimation2 = 0.0;
        std::function<bool()> isTicked;
        std::function<void()> onClick;
};

}
