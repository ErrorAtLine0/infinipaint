#pragma once
#include "Element.hpp"

namespace GUIStuff {

class RadioButton : public Element {
    public:
        RadioButton(GUIManager& gui);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) override;

        void layout(const Clay_ElementId& id, const std::function<bool()>& isTicked, const std::function<void()>& onClick);
    private:
        static constexpr float RADIOBUTTON_ANIMATION_TIME = 0.3;
        bool isHovering = false;
        float hoverAnimation2 = 0.0;
        std::function<bool()> isTicked;
        std::function<void()> onClick;
};

}
