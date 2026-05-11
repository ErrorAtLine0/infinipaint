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
#include <modules/skparagraph/include/ParagraphStyle.h>

namespace RichText {

struct ParagraphStyleData {
    skia::textlayout::TextAlign textAlignment = skia::textlayout::TextAlign::kLeft;
    skia::textlayout::TextDirection textDirection = skia::textlayout::TextDirection::kLtr;
    template <typename Archive> void save(Archive& a) const {
        a(static_cast<uint8_t>(textAlignment), static_cast<uint8_t>(textDirection));
    }
    template <typename Archive> void load(Archive& a) {
        uint8_t alignRead, directionRead;
        a(alignRead, directionRead);
        textAlignment = static_cast<skia::textlayout::TextAlign>(alignRead);
        textDirection = static_cast<skia::textlayout::TextDirection>(directionRead);
    }
    bool operator==(const ParagraphStyleData& o) const = default;
    bool operator!=(const ParagraphStyleData& o) const = default;
    skia::textlayout::ParagraphStyle get_paragraph_style() const;
};

}
