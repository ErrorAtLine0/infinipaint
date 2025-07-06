#include "FontData.hpp"
//#include <include/ports/SkFontMgr_directory.h>
#include <include/ports/SkFontMgr_empty.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>
#include <iostream>

FontData::FontData():
    mgr(SkFontMgr_New_Custom_Empty())
{
    map["Roboto"] = mgr->makeFromFile("data/fonts/Roboto-variable.ttf");
}

FontData::~FontData() {
}

Vector2f get_str_font_bounds(const SkFont& font, const std::string& str) {
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    float nextText = font.measureText(str.data(), str.length(), SkTextEncoding::kUTF8, nullptr);
    return Vector2f{nextText, - metrics.fAscent + metrics.fDescent};
}
