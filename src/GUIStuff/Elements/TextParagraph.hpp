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

#pragma once
#include "Element.hpp"
#include "../../RichText/TextBox.hpp"

namespace GUIStuff {

class TextParagraph : public Element {
    public:
        struct Data {
            RichText::TextData text;
            float maxGrowX = 10000.0f;
            float maxGrowY = 0.0f;
            bool ellipsis = true;
            bool allowNewlines = true;
            bool operator==(const Data& d) const = default;
            bool operator!=(const Data& d) const = default;
        };

        TextParagraph(GUIManager& gui);
        void layout(const Clay_ElementId& id, const Data& newData);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
    private:
        Data d;
        float fontSize;
        std::unique_ptr<RichText::TextBox> textbox;
};

}
