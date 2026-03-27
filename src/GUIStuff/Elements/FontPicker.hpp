#pragma once
#include "Element.hpp"

namespace GUIStuff {

class FontPicker : public Element {
    public:
        struct Data {
            std::string* fontName = nullptr;
            std::function<void()> onFontChange;
        };
        FontPicker(GUIManager& gui);
        void layout(const Data& data);
    private:
        Data d;

        std::vector<std::string> sortedFontList;
        std::vector<std::string> sortedFontListLowercase;
        bool dropdownOpen = false;
};

}
