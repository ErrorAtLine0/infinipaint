#pragma once
#include "Element.hpp"
#include <modules/skparagraph/include/Paragraph.h>

namespace GUIStuff {

class TextParagraph : public Element {
    public:
        TextParagraph(GUIManager& gui);
        void layout(const Clay_ElementId& id, const std::shared_ptr<skia::textlayout::Paragraph>& paragraph, float width);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
    private:
        std::shared_ptr<skia::textlayout::Paragraph> data;
};

}
