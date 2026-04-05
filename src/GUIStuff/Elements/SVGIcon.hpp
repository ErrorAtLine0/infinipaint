#pragma once
#include "Element.hpp"

namespace GUIStuff {

class SVGIcon : public Element {
    public:
        SVGIcon(GUIManager& gui);
        void layout(const Clay_ElementId& id, const std::string& newSvgPath, bool newIsHighlighted = false);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
    private:
        bool highlighted;
        sk_sp<SkSVGDOM> svgDom;
};

}
