#pragma once
#include "Element.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

template <typename T> class NumberSlider : public Element {
    public:
        NumberSlider(GUIManager& gui):
            Element(gui) {}

        void layout(const Clay_ElementId& id, T* data, T minData, T maxData, const std::function<void()>& onChange) {
            this->data = data;
            this->minData = minData;
            this->maxData = maxData;
            this->onChange = onChange;

            CLAY(id, {
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(100), .height = CLAY_SIZING_FIXED(10)}
                },
                .custom = { .customData = this }
            }) {
            }
        }

        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override {
            auto& bb = boundingBox.value();

            canvas->save();
            canvas->translate(bb.min.x(), bb.min.y());

            float lerpTimeHover = smooth_two_way_time(hoverAnimation, io.deltaTime, isHovering, io.theme->hoverExpandTime);

            static BezierEasing easeHeight(0.68, -1.55, 0.265, 2.55);

            float lerpTimeHeld = easeHeight(smooth_two_way_time(holdAnimation, io.deltaTime, isHeld, 0.3));

            float holderRadius = lerp_vec(4.0, 5.0, lerpTimeHover);
            float holderHeight = lerp_vec(4.0, 10.0, lerpTimeHeld);

            const float yChange = bb.height() * 0.5f - holderRadius * 0.5f;

            float holderPos = lerp_time<float>(*data, maxData, minData) * bb.width();

            SkRect barFull = SkRect::MakeXYWH(0.0f, yChange, holderPos, holderRadius);
            SkRect barEmpty = SkRect::MakeXYWH(holderPos, yChange, bb.width() - holderPos, holderRadius);

            SkPaint barFullP;
            barFullP.setAntiAlias(skiaAA);
            barFullP.setColor(convert_vec4<SkColor4f>(io.theme->fillColor1));
            canvas->drawRoundRect(barFull, 5.0f, 5.0f, barFullP);

            SkPaint barEmptyP;
            barEmptyP.setAntiAlias(skiaAA);
            barEmptyP.setColor(convert_vec4<SkColor4f>(io.theme->backColor2));
            canvas->drawRoundRect(barEmpty, 5.0f, 5.0f, barEmptyP);

            canvas->translate(holderPos, bb.height() * 0.5f);

            SkRect holderRect = SkRect::MakeLTRB(-holderRadius, -holderHeight, holderRadius, holderHeight);
            SkPaint holderBorderP;
            holderBorderP.setAntiAlias(skiaAA);
            holderBorderP.setStyle(SkPaint::kStroke_Style);
            holderBorderP.setStrokeWidth(3.0f);
            holderBorderP.setColor(convert_vec4<SkColor4f>(io.theme->fillColor1));
            canvas->drawRoundRect(holderRect, holderRadius, holderRadius, holderBorderP);

            SkPaint holderP;
            holderP.setAntiAlias(skiaAA);
            holderP.setColor(convert_vec4<SkColor4f>(io.theme->backColor2));
            canvas->drawRoundRect(holderRect, holderRadius, holderRadius, holderP);
            
            canvas->restore();
        }
        virtual bool input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) override {
            isHovering = mouseHovering;
            isHeld = isHovering && button.button == InputManager::MouseButton::LEFT && button.down;
            if(isHeld && boundingBox.has_value())
                update_slider_pos(button.pos);
            return Element::input_mouse_button_callback(button, mouseHovering);
        }

        virtual bool input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) override {
            isHovering = mouseHovering;
            if(isHeld && boundingBox.has_value())
                update_slider_pos(motion.pos);
            return Element::input_mouse_motion_callback(motion, mouseHovering);
        }

    private:
        void update_slider_pos(const Vector2f& p) {
            gui.set_post_callback_func([&, p] {
                float fracPosOnSlider = (p.x() - boundingBox.value().min.x()) / boundingBox.value().width();
                *data = static_cast<T>(std::clamp<double>(std::lerp<double>(minData, maxData, fracPosOnSlider), minData, maxData)); // Clamp as double then cast so that unsigned types dont wrap on clamp
                if(onChange) onChange();
            });
        }

        bool isHeld = false;
        bool isHovering = false;
        T* data = nullptr;
        T minData = 0.0;
        T maxData = 1.0;
        std::function<void()> onChange;

        float hoverAnimation = 0.0;
        float holdAnimation = 0.0;
};

}
