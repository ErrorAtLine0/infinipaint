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
            std::function<void(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering)> mouseButton;
        };
        void layout(const Clay_ElementId& id, const Data& data);
        virtual void update() override;
        virtual bool collides_with_point(const Vector2f& p) const override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
    private:
        void draw_rotate_bar(SkCanvas* canvas, UpdateInputData& io, bool skiaAA);
        void draw_palette_bar(SkCanvas* canvas, UpdateInputData& io, bool skiaAA);
        Data d;

        struct DisplayData {
            Vector4f selectedColorDisplayed = {0.0f, 0.0f, 0.0f, 0.0f};
            double rotationAngleDisplayed = 0.0;
            unsigned colorSelectionIndex = 0;
            bool isRotateBarHeld = false;
            bool isRotateBarHovered = false;
            bool isColorBarHovered = false;
            bool isHeld = false;
            bool operator!=(const DisplayData&) const = default;
            bool operator==(const DisplayData&) const = default;
        };

        DisplayData dd;
        DisplayData oldDD;

        void update_paint_circle_menu_mouse_hover(const Vector2f& p);
        void update_paint_circle_menu_mouse(const Vector2f& p, bool leftClicked);
};

}
