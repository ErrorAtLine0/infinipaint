#include "FontData.hpp"
#include <include/ports/SkFontMgr_empty.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
    #include <include/ports/SkFontMgr_directory.h>
#elif _WIN32
    #include <include/ports/SkTypeface_win.h>
#else
    #include <include/ports/SkFontMgr_fontconfig.h>
    #include <include/ports/SkFontScanner_FreeType.h>
    #include <include/ports/SkFontMgr_directory.h>
#endif

#include <src/base/SkUTF.h>

FontData::FontData()
{
#ifdef __EMSCRIPTEN__
    localFontMgr = SkFontMgr_New_Custom_Empty();
    defaultFontMgr = SkFontMgr_New_Custom_Directory("data/fonts");
#elif _WIN32
    localFontMgr = SkFontMgr_New_DirectWrite();
    defaultFontMgr = SkFontMgr_New_DirectWrite();
#else
    localFontMgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
    defaultFontMgr = SkFontMgr_New_Custom_Directory("data/fonts");
#endif


    map["Roboto"] = defaultFontMgr->makeFromFile("data/fonts/Roboto-variable.ttf");

    collection = sk_make_sp<skia::textlayout::FontCollection>();
    collection->setDefaultFontManager(defaultFontMgr, std::vector<SkString>{SkString{"Roboto"}, SkString{"Noto Emoji"}, SkString{"Noto Kufi Arabic"}});
    collection->setDynamicFontManager(localFontMgr);
    std::string s = "🙂";
    const char* sPtr = s.data();
    SkFontStyle defaultStyle;
    SkString defaultLocale("en");
    auto defaultEmojiFallback = collection->defaultEmojiFallback(SkUTF::NextUTF8(&sPtr, s.c_str() + s.length()), defaultStyle, defaultLocale);
    fallbackOnEmoji = !defaultEmojiFallback;
}

const std::vector<SkString>& FontData::get_default_font_families() const {
    static std::vector<SkString> defaultFontFamilies;
    if(defaultFontFamilies.empty()) {
        defaultFontFamilies.emplace_back("Roboto");
        if(fallbackOnEmoji)
            defaultFontFamilies.emplace_back("Noto Emoji");
        defaultFontFamilies.emplace_back("Noto Kufi Arabic");
    }
    return defaultFontFamilies;
}

void FontData::push_default_font_families(std::vector<SkString>& fontFamilies) const {
    fontFamilies.emplace_back("Roboto");
    if(fallbackOnEmoji)
        fontFamilies.emplace_back("Noto Emoji");
    fontFamilies.emplace_back("Noto Kufi Arabic");
}

FontData::~FontData() {
}

Vector2f get_str_font_bounds(const SkFont& font, const std::string& str) {
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    float nextText = font.measureText(str.data(), str.length(), SkTextEncoding::kUTF8, nullptr);
    return Vector2f{nextText, - metrics.fAscent + metrics.fDescent};
}
