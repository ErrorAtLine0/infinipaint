#pragma once
#include <include/core/SkFontMgr.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkFont.h>
#include <unordered_map>
#include <Eigen/Dense>
#include <modules/skparagraph/include/FontCollection.h>

using namespace Eigen;

struct FontData {
    FontData();
    sk_sp<SkFontMgr> defaultFontMgr;
    sk_sp<SkFontMgr> localFontMgr;
    sk_sp<skia::textlayout::FontCollection> collection;
    std::unordered_map<std::string, sk_sp<SkTypeface>> map;
    bool fallbackOnEmoji = false;
    const std::vector<SkString>& get_default_font_families() const;
    void push_default_font_families(std::vector<SkString>& fontFamilies) const;
    ~FontData();
};

Vector2f get_str_font_bounds(const SkFont& font, const std::string& str);
