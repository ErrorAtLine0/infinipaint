#pragma once
#include "Element.hpp"

namespace GUIStuff {

template <typename T> class NumberSlider : public Element {
    public:
        void update(UpdateInputData& io, T* dataPtr, T minNew, T maxNew, const std::function<void()>& elemUpdate) {
            data = dataPtr;
            min = minNew;
            max = maxNew;

            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(100), .height = CLAY_SIZING_FIXED(15)}
                },
                .custom = { .customData = this }
            }) {
                selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
                if(selection.held && data) {
                    float fracPosOnSlider = (io.mouse.pos.x() - bb.min.x()) / bb.width();
                    *data = std::clamp<T>(std::lerp<double>(min, max, fracPosOnSlider), min, max);
                }
                if(elemUpdate)
                    elemUpdate();
            }
        }

        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override {
            if(!data)
                return;

            bb = get_bb(command);

            canvas->save();
            canvas->translate(bb.min.x(), bb.min.y());

            float lerpTimeHover = smooth_two_way_time(hoverAnimation, io.deltaTime, selection.hovered, io.theme->hoverExpandTime);

            //static BezierEasing easeHeight(0.75, 0.25, 0.25, 0.75);
            static BezierEasing easeHeight(0.68, -1.55, 0.265, 2.55);

            float lerpTimeHeld = easeHeight(smooth_two_way_time(holdAnimation, io.deltaTime, selection.held, 0.3));

            float holderRadius = lerp_vec(4.0, 5.0, lerpTimeHover);
            float holderHeight = lerp_vec(4.0, 10.0, lerpTimeHeld);

            const float yChange = bb.height() * 0.5f - holderRadius * 0.5f;

            float holderPos = lerp_time<float>(*data, max, min) * bb.width();

            SkRect barFull = SkRect::MakeXYWH(0.0f, yChange, holderPos, holderRadius);
            SkRect barEmpty = SkRect::MakeXYWH(holderPos, yChange, bb.width() - holderPos, holderRadius);

            SkPaint barFullP;
            barFullP.setColor(convert_vec4<SkColor4f>(io.theme->fillColor1));
            canvas->drawRoundRect(barFull, 5.0f, 5.0f, barFullP);

            SkPaint barEmptyP;
            barEmptyP.setColor(convert_vec4<SkColor4f>(io.theme->backColor2));
            canvas->drawRoundRect(barEmpty, 5.0f, 5.0f, barEmptyP);

            canvas->translate(holderPos, bb.height() * 0.5f);

            SkRect holderRect = SkRect::MakeLTRB(-holderRadius, -holderHeight, holderRadius, holderHeight);
            SkPaint holderBorderP;
            holderBorderP.setStyle(SkPaint::kStroke_Style);
            holderBorderP.setStrokeWidth(3.0f);
            holderBorderP.setColor(convert_vec4<SkColor4f>(io.theme->fillColor1));
            canvas->drawRoundRect(holderRect, holderRadius, holderRadius, holderBorderP);

            SkPaint holderP;
            holderP.setColor(convert_vec4<SkColor4f>(io.theme->backColor2));
            canvas->drawRoundRect(holderRect, holderRadius, holderRadius, holderP);
            
            canvas->restore();
        }

    private:
        SCollision::AABB<float> bb;
        SelectionHelper selection;

        T* data = nullptr;
        T min = 0.0;
        T max = 1.0;

        float hoverAnimation = 0.0;
        float holdAnimation = 0.0;
};

}
