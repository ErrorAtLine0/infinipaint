#pragma once
#include "Element.hpp"

namespace GUIStuff {

    class GUIManager;

class PaintCircleMenu : public Element {
    public:
        struct Data {
            double currentRotationAngle = 0.0;
            double* newRotationAngle = nullptr;
            Vector4f* selectedColor = nullptr;
            std::vector<Vector3f> palette;
        };
        void update(GUIManager& gui, const Data& data, const std::function<void()>& elemUpdate);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
    private:
        void draw_rotate_bar(SkCanvas* canvas, UpdateInputData& io);
        void draw_palette_bar(SkCanvas* canvas, UpdateInputData& io);
        Data d;
        SCollision::AABB<float> bb;
        SelectionHelper rotateBarSelect;
        SelectionHelper colorBarSelect;
        unsigned colorSelectionIndex = 0;
};

}
