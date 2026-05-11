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

#include "ColorPicker.decl.hpp"
#include <include/effects/SkRuntimeEffect.h>

namespace GUIStuff { namespace ColorPickerShaders {

sk_sp<SkShader> get_hue_shader() {
    static sk_sp<SkShader> hueBarShader;
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

sk_sp<SkShader> get_sv_selection_shader(float hue) {
    static sk_sp<SkRuntimeEffect> svSelectionAreaEffect;
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

sk_sp<SkShader> get_alpha_bar_shader(const Vector3f& mainColor, float horizontalResolution) {
    static sk_sp<SkRuntimeEffect> alphaBarEffect;
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

}}
