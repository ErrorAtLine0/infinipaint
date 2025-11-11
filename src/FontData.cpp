#include "FontData.hpp"
#include <include/ports/SkFontMgr_directory.h>
#include <include/ports/SkFontMgr_empty.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
#elif _WIN32
    #include <include/ports/SkTypeface_win.h>
#elif __APPLE__
    #include <include/ports/SkFontMgr_mac_ct.h>
#elif __linux__
    #include <include/ports/SkFontMgr_fontconfig.h>
    #include <include/ports/SkFontScanner_FreeType.h>
#endif

FontData::FontData()
{
#ifdef __EMSCRIPTEN__
    localFontMgr = SkFontMgr_New_Custom_Empty();
#elif _WIN32
    localFontMgr = SkFontMgr_New_GDI();
#elif __APPLE__
    localFontMgr = SkFontMgr_New_Custom_Empty();
#elif __linux__
    localFontMgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#else
    localFontMgr = SkFontMgr_New_Custom_Empty();
#endif

    defaultFontMgr = SkFontMgr_New_Custom_Directory("data/fonts");

    map["Roboto"] = defaultFontMgr->makeFromFile("data/fonts/Roboto-variable.ttf");

    collection = sk_make_sp<skia::textlayout::FontCollection>();
    collection->setDefaultFontManager(defaultFontMgr, {SkString{"Roboto"}});
    collection->setDynamicFontManager(localFontMgr);
}

FontData::~FontData() {
}

Vector2f get_str_font_bounds(const SkFont& font, const std::string& str) {
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    float nextText = font.measureText(str.data(), str.length(), SkTextEncoding::kUTF8, nullptr);
    return Vector2f{nextText, - metrics.fAscent + metrics.fDescent};
}
