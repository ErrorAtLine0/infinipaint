#pragma once
#include "Element.hpp"

namespace GUIStuff {

class SVGIcon : public Element {
    public:
        void update(UpdateInputData& io, const std::string& newSvgPath, bool newIsHighlighted, const std::function<void()>& elemUpdate);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
    private:
        bool highlighted;
        sk_sp<SkSVGDOM> svgDom;
};

}
