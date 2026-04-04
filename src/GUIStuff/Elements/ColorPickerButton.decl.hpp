#pragma once
#include "Element.hpp"

namespace GUIStuff {

struct ColorPickerButtonData {
    bool hasAlpha = true;
    std::function<void()> onEdit;
    std::function<void()> onSelect;
    std::function<void()> onDeselect;
};

template <typename T> class ColorPickerButton : public Element {
    public:
        ColorPickerButton(GUIManager& gui);
        void layout(const Clay_ElementId& id, T* val, const ColorPickerButtonData& d);
    private:
        ColorPickerButtonData data;
        bool isOpen = false;
};

}
