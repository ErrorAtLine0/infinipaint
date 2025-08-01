#pragma once
#include "Element.hpp"
#include <include/core/SkColor.h>
#include <include/core/SkPath.h>
#include <include/effects/SkRuntimeEffect.h>
#include <iostream>
#include <Helpers/SCollision.hpp>
#include <Helpers/HsvRgb.hpp>

namespace GUIStuff {

template <typename T> class ColorPicker : public Element {
    public:
        void update(UpdateInputData& io, T* newData, bool newSelectAlpha, const std::function<void()>& elemUpdate) {
            T* data = newData;
            selectAlpha = newSelectAlpha;

            if(!data)
                return;

            force_update_colorpicker(data);

            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(bb.width()) }
                },
                .custom = { .customData = this }
            }) {
                selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
                if(selection.clicked && data) {
                    float svSelectionAreaSize = get_sv_selection_area_size();
                    modifyingSv = SCollision::collide(SCollision::AABB<float>(bb.min, bb.min + Vector2f{svSelectionAreaSize, svSelectionAreaSize}), io.mouse.pos);
                    Vector2f hueBarPos = get_hue_bar_pos();
                    Vector2f hueBarDim = get_hue_bar_dim();
                    modifyingHue = SCollision::collide(SCollision::AABB<float>(hueBarPos, hueBarPos + hueBarDim), io.mouse.pos);
                    if(selectAlpha) {
                        Vector2f alphaBarPos = get_alpha_bar_pos();
                        Vector2f alphaBarDim = get_alpha_bar_dim();
                        modifyingAlpha = SCollision::collide(SCollision::AABB<float>(alphaBarPos, alphaBarPos + alphaBarDim), io.mouse.pos);
                    }
                }
                if(selection.held && modifyingSv) {
                    float svSelectionAreaSize = bb.width() - (BAR_GAP + BAR_WIDTH);
                    Vector2f newSv = cwise_vec_clamp<Vector2f>((io.mouse.pos - bb.min) / svSelectionAreaSize, Vector2f{0.0f, 0.0f}, Vector2f{1.0f, 1.0f});
                    savedHsv.y() = newSv.x();
                    savedHsv.z() = 1.0f - newSv.y();
                    set_hsv(data, savedHsv);
                }
                if(selection.held && modifyingHue) {
                    Vector2f huePos = get_hue_bar_pos();
                    Vector2f hueDim = get_hue_bar_dim();
                    savedHsv.x() = (1.0f - std::clamp((io.mouse.pos.y() - huePos.y()) / hueDim.y(), 0.0f, 1.0f)) * 360.0f;
                    set_hsv(data, savedHsv);
                }
                if(selection.held && modifyingAlpha) {
                    Vector2f alphaPos = get_alpha_bar_pos();
                    Vector2f alphaDim = get_alpha_bar_dim();
                    (*data)[3] = std::clamp((io.mouse.pos.x() - alphaPos.x()) / alphaDim.x(), 0.0f, 1.0f);
                    oldData = *data;
                }
                if(!selection.held) {
                    modifyingSv = false;
                    modifyingHue = false;
                    modifyingAlpha = false;
                }
                if(elemUpdate)
                    elemUpdate();
            }
        }
        
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
            bb = get_bb(command);
        
            canvas->save();
            canvas->translate(bb.min.x(), bb.min.y());
            float svSelectionAreaSize = get_sv_selection_area_size();
            canvas->scale(svSelectionAreaSize, svSelectionAreaSize);
            canvas->clipRect(SkRect::MakeXYWH(0.0f, 0.0f, 1.0f, 1.0f));

            SkPaint svSelectionAreaPaint;
            svSelectionAreaPaint.setShader(get_sv_selection_shader(savedHsv.x() / 360.0f));
            canvas->drawPaint(svSelectionAreaPaint);

            SkPaint selectionLinePaint({1.0f, 1.0f, 1.0f, 1.0f});
            selectionLinePaint.setStrokeWidth(2.0f / svSelectionAreaSize);
            canvas->drawLine(0.0f, 1.0f - savedHsv.z(), 1.0f, 1.0f - savedHsv.z(), selectionLinePaint);
            canvas->drawLine(savedHsv.y(), 0.0f, savedHsv.y(), 1.0f, selectionLinePaint);

            canvas->restore();

            float normalizedHue = savedHsv.x() / 360.0f;

            Vector2f hueBarPos = get_hue_bar_pos();
            Vector2f hueBarDim = get_hue_bar_dim();
            canvas->save();
            canvas->translate(hueBarPos.x(), hueBarPos.y());
            canvas->scale(hueBarDim.x(), hueBarDim.y());
            canvas->clipRect(SkRect::MakeXYWH(0.0f, 0.0f, 1.0f, 1.0f));

            SkPaint hueBarPaint;
            hueBarPaint.setShader(get_hue_shader());
            canvas->drawPaint(hueBarPaint);
            canvas->drawLine(0.0f, 1.0f - normalizedHue, 1.0f, 1.0f - normalizedHue, selectionLinePaint);

            canvas->restore();

            Vector2f alphaBarPos = get_alpha_bar_pos();
            Vector2f alphaBarDim = get_alpha_bar_dim();
            canvas->save();
            canvas->translate(alphaBarPos.x(), alphaBarPos.y());
            canvas->clipRect(SkRect::MakeXYWH(0.0f, 0.0f, alphaBarDim.x(), alphaBarDim.y()));

            SkPaint alphaBarPaint;
            if(selectAlpha)
                alphaBarPaint.setShader(get_alpha_bar_shader({oldData[0], oldData[1], oldData[2]}, alphaBarDim.x()));
            else
                alphaBarPaint.setColor4f(SkColor4f{oldData[0], oldData[1], oldData[2], 1.0f});
            canvas->drawPaint(alphaBarPaint);

            if(selectAlpha) {
                canvas->scale(alphaBarDim.x(), alphaBarDim.y());
                canvas->drawLine(oldData[3], 0.0f, oldData[3], 1.0f, selectionLinePaint);
            }

            canvas->restore();
        }
    private:
        void force_update_colorpicker(T* data) {
            if(data && (*data == oldData))
                return;

            if(data) {
                savedHsv = rgb_to_hsv<Vector3f>(*data);
                oldData = *data;
            }
            else
                oldData = T{0, 0, 0, 0};
        }

        float get_sv_selection_area_size() {
            return bb.width() - (BAR_GAP + BAR_WIDTH);
        }

        Vector2f get_hue_bar_pos() {
            return {bb.min.x() + get_sv_selection_area_size() + BAR_GAP, bb.min.y()};
        }

        Vector2f get_hue_bar_dim() {
            return {BAR_WIDTH, get_sv_selection_area_size()};
        }

        Vector2f get_alpha_bar_pos() {
            return {bb.min.x(), bb.min.y() + get_sv_selection_area_size() + BAR_GAP};
        }

        Vector2f get_alpha_bar_dim() {
            return {bb.width(), BAR_WIDTH};
        }

        void set_hsv(T* data, const Vector3f& hsv) {
            Vector3f a = hsv_to_rgb<Vector3f>(hsv);
            (*data)[0] = a.x();
            (*data)[1] = a.y();
            (*data)[2] = a.z();
            oldData = *data;
        }

        static constexpr float BAR_WIDTH = 30.0;
        static constexpr float BAR_GAP = 5.0;

        // HSV to RGB func from: https://github.com/hughsk/glsl-hsv2rgb?tab=readme-ov-file
            static constexpr const char* hueBarSkSl =
        R"V(
        vec3 hsv2rgb(vec3 c) {
          vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
          vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
          return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
        }
        
        vec4 main(vec2 fragCoord) {
          vec3 a = hsv2rgb(vec3(1.0 - fragCoord.y, 1.0, 1.0));
          return vec4(a.xyz, 1.0);
        })V";
        
            static constexpr const char* svSelectionAreaSkSl = 
        R"V(uniform float hue;
        vec3 hsv2rgb(vec3 c) {
          vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
          vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
          return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
        }
        
        vec4 main(vec2 fragCoord) {
          vec3 a = hsv2rgb(vec3(hue, fragCoord.x, 1.0 - fragCoord.y));
          return vec4(a.xyz, 1.0);
        })V";

            static constexpr const char* alphaBarSksl = 
        R"V(uniform vec3 mainColor;
uniform float horizontalResolution;

vec4 main(vec2 fragcoord) {
    vec4 alphaBoard1 = vec4(0.2, 0.2, 0.2, 1.0);
    vec4 alphaBoard2 = vec4(0.8, 0.8, 0.8, 1.0);
    
    vec2 flooredCoords = floor(fragcoord / 10);
    bool isDark = mod(flooredCoords.x + flooredCoords.y, 2.0) > 0.5;
  
    return mix(isDark ? alphaBoard2 : alphaBoard1, vec4(mainColor, 1.0), fragcoord.x / horizontalResolution);
}
        )V";

        static sk_sp<SkShader> hueBarShader;
        static sk_sp<SkRuntimeEffect> svSelectionAreaEffect;
        static sk_sp<SkRuntimeEffect> alphaBarEffect;

        static sk_sp<SkShader> get_hue_shader();
        static sk_sp<SkShader> get_sv_selection_shader(float hue);
        static sk_sp<SkShader> get_alpha_bar_shader(const Vector3f& mainColor, float horizontalResolution);

        SCollision::AABB<float> bb;
        SelectionHelper selection;
        T oldData;
        Vector3f savedHsv; // We save the HSV so that conversion doesnt ruin the UI
        bool selectAlpha;
        bool modifyingSv = false;
        bool modifyingHue = false;
        bool modifyingAlpha = false;
};

template <typename T> sk_sp<SkShader> ColorPicker<T>::hueBarShader;
template <typename T> sk_sp<SkRuntimeEffect> ColorPicker<T>::svSelectionAreaEffect;
template <typename T> sk_sp<SkRuntimeEffect> ColorPicker<T>::alphaBarEffect;

template <typename T> sk_sp<SkShader> ColorPicker<T>::get_hue_shader(){
    if(!hueBarShader) {
        auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(hueBarSkSl));
        if(!err.isEmpty()) {
           std::cout << "[ColorPicker::get_hue_shader] Hue shader construction error\n";
           std::cout << err.c_str() << std::endl;
        }
        else
           hueBarShader = effect->makeShader(nullptr, {nullptr, 0});
    }
    return hueBarShader;
}

template <typename T> sk_sp<SkShader> ColorPicker<T>::get_sv_selection_shader(float hue) {
    if(!svSelectionAreaEffect) {
        auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(svSelectionAreaSkSl));
        if(!err.isEmpty()) {
            std::cout << "[ColorPicker::get_sv_selection_shader] SV Selection shader construction error\n";
            std::cout << err.c_str() << std::endl;
            return nullptr;
        }
        else
            svSelectionAreaEffect = effect;
    }
    SkRuntimeShaderBuilder builder(svSelectionAreaEffect);
    builder.uniform("hue") = hue;
    return builder.makeShader();
}

template <typename T> sk_sp<SkShader> ColorPicker<T>::get_alpha_bar_shader(const Vector3f& mainColor, float horizontalResolution) {
    if(!alphaBarEffect) {
        auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(alphaBarSksl));
        if(!err.isEmpty()) {
            std::cout << "[ColorPicker::get_alpha_bar_shader] Alpha bar shader construction error\n";
            std::cout << err.c_str() << std::endl;
            return nullptr;
        }
        else
            alphaBarEffect = effect;
    }
    SkRuntimeShaderBuilder builder(alphaBarEffect);
    builder.uniform("mainColor") = SkV3{mainColor.x(), mainColor.y(), mainColor.z()};
    builder.uniform("horizontalResolution") = horizontalResolution;
    return builder.makeShader();
}

}
