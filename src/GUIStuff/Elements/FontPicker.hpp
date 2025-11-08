#pragma once
#include "Element.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

class FontPicker : public Element {
    public:
        bool update(UpdateInputData& io, std::string* fontName, GUIManager* gui);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
        SelectionHelper selection;
    private:
        std::vector<std::string> sortedFontList;
        std::vector<std::string> sortedFontListLowercase;
        bool dropdownOpen = false;
};

}
