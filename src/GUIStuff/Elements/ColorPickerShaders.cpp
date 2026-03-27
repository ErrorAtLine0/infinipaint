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
