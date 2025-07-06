#pragma once
#include <include/core/SkFontMgr.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkFont.h>
#include <unordered_map>
#include <Eigen/Dense>

using namespace Eigen;

struct FontData {
    FontData();
    sk_sp<SkFontMgr> mgr;
    std::unordered_map<std::string, sk_sp<SkTypeface>> map;
    ~FontData();
};

Vector2f get_str_font_bounds(const SkFont& font, const std::string& str);
