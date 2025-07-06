// This file has been copied from the Skia library, and may be slightly modified. Please check the NOTICE file for more details

#pragma once
#include <include/core/SkFont.h>
#include <include/core/SkTextBlob.h>

#include <cstddef>
#include <vector>

namespace CollabTextBox {
    struct ShapeResult {
        sk_sp<SkTextBlob> blob;
        std::vector<size_t> lineBreakOffsets;
        std::vector<SkRect> glyphBounds;
        std::vector<bool> wordBreaks;
        int verticalAdvance;
    };
    
    ShapeResult shape(const char* ut8text, size_t textByteLen, const SkFont& font, sk_sp<SkFontMgr> fontMgr, const char* locale, float width);
    std::vector<bool> get_utf8_word_boundaries(const char* begin, size_t byteCount, const char* locale);
};
