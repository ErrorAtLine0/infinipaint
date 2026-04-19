#pragma once

#include "Element.hpp"

namespace GUIStuff {

class ImageDisplay : public Element {
    public:
        ImageDisplay(GUIManager& gui);
        void layout(const Clay_ElementId& id, const std::filesystem::path& imgPath);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;

    private:
        std::filesystem::path oldImgPath;
        sk_sp<SkImage> img;
};

}
