#pragma once
#include "../GUIManager.hpp"
#include "../Elements/SelectableButton.hpp"
#include "../Elements/ColorRectangleDisplay.hpp"

namespace GUIStuff { namespace ElementHelpers {

constexpr static float BIG_BUTTON_SIZE = 30;
constexpr static float SMALL_BUTTON_SIZE = 20;

template <typename OptT> SelectableButton::Data selectable_button_options_to_data(const OptT& options) {
    SelectableButton::Data toRet;
    toRet.onClick = options.onClick;
    toRet.isSelected = options.isSelected;
    toRet.drawType = options.drawType;
    return toRet;
}

struct TextButtonOptions {
    SelectableButton::DrawType drawType = SelectableButton::DrawType::FILLED;
    bool isSelected = false;
    bool wide = false;
    bool centered = true;

    std::function<void()> onClick;
};

struct SVGButtonOptions {
    SelectableButton::DrawType drawType = SelectableButton::DrawType::FILLED;
    bool isSelected = false;
    float size = BIG_BUTTON_SIZE;

    std::function<void()> onClick;
};

struct FixedSizeColorButtonOptions {
    SelectableButton::DrawType drawType = SelectableButton::DrawType::FILLED;
    bool isSelected = false;
    bool hasAlpha = true;
    float size = BIG_BUTTON_SIZE;

    std::function<void()> onClick;
};

void text_button(GUIManager& gui, const char* id, std::string_view text, const TextButtonOptions& options);
void text_button_sized(GUIManager& gui, const char* id, std::string_view text, Clay_SizingAxis x, Clay_SizingAxis y, const TextButtonOptions& options);
void svg_icon_button(GUIManager& gui, const char* id, const std::string& svgPath, const SVGButtonOptions& options);

template <typename T> void color_button(GUIManager& gui, const char* id, T* val, const FixedSizeColorButtonOptions& options) {
    SelectableButton::Data d = selectable_button_options_to_data(options);
    d.innerContent = [&] (const SelectableButton::InnerContentCallbackParameters&) {
        gui.element<ColorRectangleDisplay>([val, hasAlpha = options.hasAlpha] {
            if(hasAlpha)
                return SkColor4f{(*val)[0], (*val)[1], (*val)[2], 1.0f};
            else
                return SkColor4f{(*val)[0], (*val)[1], (*val)[2], (*val)[3]};
        });
    };
    CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(options.size), .height = CLAY_SIZING_FIXED(options.size) } } }) {
        gui.element<SelectableButton>(id, options);
    }
}

}}
