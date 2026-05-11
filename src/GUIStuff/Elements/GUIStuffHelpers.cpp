/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Element.hpp"
#include "Helpers/MathExtras.hpp"
#include <chrono>

namespace GUIStuff {
    SCollision::AABB<float> clay_bounding_box_to_aabb(const Clay_BoundingBox& bb) {
        return {{bb.x, bb.y}, {bb.x + bb.width, bb.y + bb.height}};
    }

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
        theme->backColor0 = {0.00f, 0.00f, 0.00f, 1.0f};
        theme->backColor1 = {0.156f, 0.156f, 0.18f, 1.0f};
        theme->backColor2 = {0.24f, 0.24f, 0.29f, 1.0f};
        theme->frontColor1 = {0.87f, 0.87f, 0.87f, 1.0f};
        theme->frontColor2 = {0.64f, 0.64f, 0.64f, 1.0f};
        return theme;
    }
    
    void SelectionHelper::update(bool isHovering, bool isLeftClick, bool isLeftHeld, const Vector2f& cursorPos) {
        hovered = isHovering;
    
        clicked = false;
        tapped = false;
        justUnselected = false;
        bool oldSelected = selected;
    
        if(isLeftClick) {
            if(hovered) {
                held = true;
                clicked = true;
                selected = true;
                timeClicked = std::chrono::steady_clock::now();
                clickPos = cursorPos;
            }
            else
                selected = false;
        }
        else if(!isLeftHeld) {
            if(held && hovered && (std::chrono::steady_clock::now() - timeClicked) < std::chrono::milliseconds(250) && vec_distance_sqrd(clickPos, cursorPos) < (25 * 25))
                tapped = true;
            held = false;
        }

        if(!selected && oldSelected)
            justUnselected = true;
    }
}
