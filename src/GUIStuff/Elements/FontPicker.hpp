#pragma once
#include "Element.hpp"
#include <limits>

namespace GUIStuff {

struct FontPickerData {
    std::function<void()> onFontChange;
};

class FontPicker : public Element {
    public:
        FontPicker(GUIManager& gui);
        void layout(const Clay_ElementId& id, std::string* newFontName, const FontPickerData& newData = {});
    private:
        std::string* fontName;
        FontPickerData data;

        size_t hoveringOver = std::numeric_limits<size_t>::max();

        bool jumpToFontName = false;

        std::optional<std::string> get_valid_font_name();

        bool dropdownOpen = false;
};

}
