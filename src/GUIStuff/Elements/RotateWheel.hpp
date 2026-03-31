#pragma once
#include "Element.hpp"

namespace GUIStuff {

class RotateWheel : public Element {
    public:
        RotateWheel(GUIManager& gui);
        void layout(const Clay_ElementId& id, double* rotateAngle, const std::function<void()>& onChange);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual bool input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) override;
        virtual bool input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) override;

    private:
        float wheel_start();
        float wheel_end();
        void draw_rotate_wheel(SkCanvas* canvas, UpdateInputData& io, bool skiaAA);

        double* rotateAngle = nullptr;
        bool isHeld = false;
        bool isHovering = false;
        bool isRotateBarHeld = false;
        bool isRotateBarHovered = false;
        std::function<void()> onChange;

        void update_paint_circle_menu_mouse(const Vector2f& p);
};

}
