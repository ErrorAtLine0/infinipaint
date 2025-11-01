#include "Element.hpp"

namespace GUIStuff {
    SkFont get_setup_skfont() {
        SkFont font;
        font.setLinearMetrics(true);
        font.setHinting(SkFontHinting::kNormal);
        //font.setForceAutoHinting(true);
        font.setSubpixel(true);
        font.setBaselineSnap(true);
        font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        //paint.setAntiAlias(true);
        return font;
    }

    SkFont UpdateInputData::get_font(float fSize) const {
        SkFont f = get_setup_skfont();
        f.setTypeface(textTypeface);
        f.setSize(fSize);
        return f;
    }

    std::shared_ptr<Theme> get_default_dark_mode() {
        std::shared_ptr<Theme> theme(std::make_shared<Theme>());
        theme->fillColor1 = {0.65f, 0.64f, 1.0f, 1.0f};
        theme->fillColor2 = {0.6f, 0.6f, 0.785f, 1.0f};
        theme->backColor1 = {0.156f, 0.156f, 0.18f, 1.0f};
        theme->backColor2 = {0.24f, 0.24f, 0.29f, 1.0f};
        theme->frontColor1 = {0.87f, 0.87f, 0.87f, 1.0f};
        theme->frontColor2 = {0.64f, 0.64f, 0.64f, 1.0f};
        return theme;
    }

    SCollision::AABB<float> Element::get_bb(Clay_RenderCommand* command) {
        return {{command->boundingBox.x, command->boundingBox.y}, {command->boundingBox.x + command->boundingBox.width, command->boundingBox.y + command->boundingBox.height}};
    }
    
    void SelectionHelper::update(bool isHovering, bool isLeftClick, bool isLeftHeld) {
        hovered = isHovering;
    
        clicked = false;
        justUnselected = false;
        bool oldSelected = selected;
    
        if(isLeftClick) {
            if(hovered) {
                selected = true;
                clicked = true;
                held = true;
            }
            else
                selected = false;
        }
        else if(!isLeftHeld)
            held = false;

        if(!selected && oldSelected)
            justUnselected = true;
    }
}
