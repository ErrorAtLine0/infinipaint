#pragma once
#include "Element.hpp"

namespace GUIStuff {

class CheckBox : public Element {
    public:
        void update(UpdateInputData& io, bool newIsTicked, const std::function<void()>& elemUpdate);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
        SelectionHelper selection;
    private:
        static constexpr float CHECKBOX_ANIMATION_TIME = 0.3;
        bool isTicked = false;
        float hoverAnimation2 = 0.0;
};

}
