#pragma once
#include "Element.hpp"
#include <modules/skparagraph/include/Paragraph.h>

namespace GUIStuff {

class TextParagraph : public Element {
    public:
        void update(UpdateInputData& io, std::unique_ptr<skia::textlayout::Paragraph> paragraph, float width, const std::function<void()>& elemUpdate);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
    private:
        SCollision::AABB<float> bb = SCollision::AABB<float>({0.0f, 0.0f}, {0.0f, 0.0f});
        std::unique_ptr<skia::textlayout::Paragraph> data;
};

}
