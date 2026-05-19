/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "Element.hpp"
#include <include/core/SkColor.h>
#include <include/core/SkPath.h>
#include <include/effects/SkRuntimeEffect.h>
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
        virtual void update() override;
        void layout(const Clay_ElementId& id, T* data, bool selectAlpha, const ColorPickerData& conf = {});
    private:
        void set_rgb_from_hsv(const Vector3f& hsv);

        static constexpr float BAR_WIDTH = 30.0;
        static constexpr uint16_t BAR_GAP = 5;

        Vector3f savedHsv = {0.0f, 0.0f, 0.0f};
        T* data;
        std::optional<T> oldData;
        ColorPickerData config;
};

}
