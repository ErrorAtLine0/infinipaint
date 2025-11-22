#include "FontData.hpp"
#include <include/ports/SkFontMgr_empty.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
    #include <include/ports/SkFontMgr_directory.h>
#elif __APPLE__
    #include <include/ports/SkFontMgr_mac_ct.h>
    #include <ApplicationServices/ApplicationServices.h>
#elif _WIN32
    #include <include/ports/SkTypeface_win.h>
    #include "WindowsFontData/CustomFontSetManager.h"
    DWriteCustomFontSets::CustomFontSetManager fontSetManagerWindows;
#else
    #include <include/ports/SkFontMgr_fontconfig.h>
    #include <include/ports/SkFontScanner_FreeType.h>
    #include <include/ports/SkFontMgr_directory.h>
#endif

#include <src/base/SkUTF.h>
#include <filesystem>

#include <Helpers/Logger.hpp>

FontData::FontData()
{

#ifdef __EMSCRIPTEN__
    defaultFontMgr = SkFontMgr_New_Custom_Directory("data/fonts");
#elif __APPLE__
    for(const auto& dirEntry : std::filesystem::recursive_directory_iterator("data/fonts")) {
        if(dirEntry.is_regular_file()) {
            std::string urlStr = std::string(std::filesystem::canonical(dirEntry.path()).string());
            CFURLRef fontURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(urlStr.c_str()), urlStr.size(), false);
            CTFontManagerRegisterFontsForURL(fontURL, CTFontManagerScope::kCTFontManagerScopeProcess, nullptr);
            CFRelease(fontURL);
        }
    }
    defaultFontMgr = SkFontMgr_New_CoreText(nullptr);
#elif _WIN32
    localFontMgr = SkFontMgr_New_DirectWrite();
    std::vector<std::wstring> fontPaths;
    for(const auto& dirEntry : std::filesystem::recursive_directory_iterator(std::filesystem::path(L"data\\fonts"))) {
        if(dirEntry.is_regular_file())
            fontPaths.emplace_back(dirEntry.path().wstring());
    }
    fontSetManagerWindows.CreateFontSetUsingLocalFontFiles(fontPaths);
    fontSetManagerWindows.CreateFontCollectionFromFontSet();

    IDWriteFactory* fac = fontSetManagerWindows.IDWriteFactory5_IsAvailable() ? fontSetManagerWindows.m_dwriteFactory5.Get() : fontSetManagerWindows.m_dwriteFactory3.Get();
    defaultFontMgr = SkFontMgr_New_DirectWrite(fac, fontSetManagerWindows.m_customFontCollection.Get());
#else
    localFontMgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
    defaultFontMgr = SkFontMgr_New_Custom_Directory("data/fonts");
#endif

    map["Roboto"] = defaultFontMgr->makeFromFile("data/fonts/Roboto-variable.ttf");

    collection = sk_make_sp<skia::textlayout::FontCollection>();
    collection->setDefaultFontManager(defaultFontMgr, std::vector<SkString>{SkString{"Roboto"}, SkString{"Noto Emoji"}, SkString{"Noto Kufi Arabic"}});
    if(localFontMgr)
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
