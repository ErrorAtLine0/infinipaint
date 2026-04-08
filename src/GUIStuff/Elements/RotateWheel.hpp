#pragma once
#include "Element.hpp"

namespace GUIStuff {

class RotateWheel : public Element {
    public:
        RotateWheel(GUIManager& gui);
        void layout(const Clay_ElementId& id, double* rotateAngle, const std::function<void()>& onChange);
        virtual void update() override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;

    private:
        float wheel_start();
        float wheel_end();
        void draw_rotate_wheel(SkCanvas* canvas, UpdateInputData& io, bool skiaAA);

        double* rotateAngle = nullptr;
        std::function<void()> onChange;

        struct DisplayData {
            double rotateAngleDisplayed = 0.0;
            bool isHeld = false;
            bool isRotateBarHeld = false;
            bool isRotateBarHovered = false;
            bool operator!=(const DisplayData&) const = default;
            bool operator==(const DisplayData&) const = default;
        };

        DisplayData dd;
        DisplayData oldDD;

        void update_rotate_wheel_mouse_hover(const Vector2f& p);
        void update_rotate_wheel_mouse(const Vector2f& p);
};

}
