#pragma once
#include "Element.hpp"
#include "../../RichText/TextBox.hpp"

namespace GUIStuff {

class TextParagraph : public Element {
    public:
        struct Data {
            RichText::TextData text;
            float width;
            bool operator==(const Data& d) const = default;
            bool operator!=(const Data& d) const = default;
        };

        TextParagraph(GUIManager& gui);
        void layout(const Clay_ElementId& id, const Data& newData);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
    private:
        Data d;
        std::unique_ptr<RichText::TextBox> textbox;
};

}
