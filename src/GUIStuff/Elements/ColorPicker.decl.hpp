#pragma once
#include "Element.hpp"
#include <include/core/SkColor.h>
#include <include/core/SkPath.h>
#include <include/effects/SkRuntimeEffect.h>
#include <iostream>
#include <Helpers/SCollision.hpp>
#include <Helpers/HsvRgb.hpp>

namespace GUIStuff {

namespace ColorPickerShaders {
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
        R"V(
        uniform float hue;
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
        R"V(
        uniform vec3 mainColor;
        uniform float horizontalResolution;
        
        vec4 main(vec2 fragcoord) {
            vec4 alphaBoard1 = vec4(0.2, 0.2, 0.2, 1.0);
            vec4 alphaBoard2 = vec4(0.8, 0.8, 0.8, 1.0);
            
            vec2 flooredCoords = floor(fragcoord / 10);
            bool isDark = mod(flooredCoords.x + flooredCoords.y, 2.0) > 0.5;
      
            return mix(isDark ? alphaBoard2 : alphaBoard1, vec4(mainColor, 1.0), fragcoord.x / horizontalResolution);
        })V";

    sk_sp<SkShader> get_hue_shader();
    sk_sp<SkShader> get_sv_selection_shader(float hue);
    sk_sp<SkShader> get_alpha_bar_shader(const Vector3f& mainColor, float horizontalResolution);
}

struct ColorPickerData {
    std::function<void()> onChange;
    std::function<void()> onHold;
    std::function<void()> onRelease;
};

template <typename T> class ColorPicker : public Element {
    public:
        ColorPicker(GUIManager& gui);
        void layout(const Clay_ElementId& id, T* data, bool selectAlpha, const ColorPickerData& config);
        virtual void update() override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA);
        void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
    private:
        void update_color_picker_pos(const Vector2f& p, bool justHeld);
        float get_sv_selection_area_size();
        Vector2f get_hue_bar_pos();
        Vector2f get_hue_bar_dim();
        Vector2f get_alpha_bar_pos();
        Vector2f get_alpha_bar_dim();
        void set_hsv(const Vector3f& hsv);

        static constexpr float BAR_WIDTH = 30.0;
        static constexpr float BAR_GAP = 5.0;

        struct DisplayData {
            Vector3f savedHsv = {0.0f, 0.0f, 0.0f};
            float savedAlpha = 0.0f;
            bool operator!=(const DisplayData&) const = default;
            bool operator==(const DisplayData&) const = default;
        };

        DisplayData dd;
        DisplayData oldDD;

        T* data;
        std::optional<T> oldData;
        ColorPickerData config;
        bool selectAlpha = false;
        enum class HeldBar {
            NONE,
            SV_HELD,
            HUE_HELD,
            ALPHA_HELD
        };
        HeldBar held = HeldBar::NONE;
};

}
