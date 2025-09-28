#pragma once
#include "Element.hpp"

namespace GUIStuff {

class RotateWheel : public Element {
    public:
        void update(UpdateInputData& io, double* newRotationAnglePtr, const std::function<void()>& elemUpdate);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
        SelectionHelper selection;
    private:
        float wheel_start();
        float wheel_end();
        void draw_rotate_wheel(SkCanvas* canvas, UpdateInputData& io);

        double rotateAngle = 0.0f;
        SCollision::AABB<float> bb;
        bool wheelHovered;
};

}
