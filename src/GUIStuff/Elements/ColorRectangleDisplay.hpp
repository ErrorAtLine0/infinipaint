#pragma once
#include "Element.hpp"
#include <include/effects/SkRuntimeEffect.h>

namespace GUIStuff {

class ColorRectangleDisplay : public Element {
    public:
        ColorRectangleDisplay(GUIManager& gui);
        void layout(const Clay_ElementId& id, const std::function<SkColor4f()>& colorFunc);
        virtual void update() override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
    private:
        static constexpr const char* alphaBackgroundSksl = 
        R"V(
vec4 main(vec2 fragcoord) {
    vec4 alphaBoard1 = vec4(0.2, 0.2, 0.2, 1.0);
    vec4 alphaBoard2 = vec4(0.8, 0.8, 0.8, 1.0);
    
    vec2 flooredCoords = floor(fragcoord / 5);
    bool isDark = mod(flooredCoords.x + flooredCoords.y, 2.0) > 0.5;
  
    return isDark ? alphaBoard2 : alphaBoard1;
}
        )V";
        std::function<SkColor4f()> colorFunc;
        SkColor4f drawVal;

        static sk_sp<SkRuntimeEffect> alphaBackgroundEffect;
        static sk_sp<SkShader> get_alpha_background_shader();
};

}
