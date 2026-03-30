#pragma once
#include "Element.hpp"

namespace GUIStuff {

class PaintCircleMenu : public Element {
    public:
        PaintCircleMenu(GUIManager& gui);
        struct Data {
            double* rotationAngle = nullptr;
            Vector4f* selectedColor = nullptr;
            std::vector<Vector3f> palette;
            std::function<void()> onRotate;
            std::function<void()> onPaletteClick;
        };
        void layout(const Data& data);
        virtual bool collides_with_point(const Vector2f& p) const override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual bool input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) override;
        virtual bool input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) override;
    private:
        void draw_rotate_bar(SkCanvas* canvas, UpdateInputData& io, bool skiaAA);
        void draw_palette_bar(SkCanvas* canvas, UpdateInputData& io, bool skiaAA);
        Data d;
        unsigned colorSelectionIndex = 0;
        bool isHovering = false;
        bool isRotateBarHeld = false;
        bool isRotateBarHovered = false;
        bool isColorBarHovered = false;
        bool isHeld = false;

        void update_paint_circle_menu_mouse(const Vector2f& p, bool leftClicked);
};

}
