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
    skia::textlayout::ParagraphStyle get_paragraph_style() const;
};

}
