#pragma once
#include "Element.hpp"

namespace GUIStuff {

struct ColorPickerButtonData {
    bool hasAlpha = false;
    std::function<void()> onEdit;
};

template <typename T> class ColorPickerButton : public Element {
    public:
        ColorPickerButton(GUIManager& gui);
        void layout(T* val, ColorPickerButtonData& d);
    private:
        ColorPickerButtonData data;
        bool isOpen = false;
};

}
