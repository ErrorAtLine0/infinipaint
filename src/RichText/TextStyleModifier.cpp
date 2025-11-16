#include "TextStyleModifier.hpp"
#include <Helpers/ConvertVec.hpp>
#include <Helpers/Serializers.hpp>
#include <include/core/SkFontStyle.h>
#include <modules/skparagraph/include/TextStyle.h>
#include "../FontData.hpp"

#define WEIGHT_VALUE_MODIFIER 100

namespace RichText {

TextStyleModifier::ModifierMap TextStyleModifier::defaultMods;

std::shared_ptr<TextStyleModifier> TextStyleModifier::allocate_modifier(ModifierType type) {
    switch(type) {
        case ModifierType::SIZE:
            return std::make_shared<SizeTextStyleModifier>();
        case ModifierType::COLOR:
            return std::make_shared<ColorTextStyleModifier>();
        case ModifierType::WEIGHT:
            return std::make_shared<WeightTextStyleModifier>();
        case ModifierType::SLANT:
            return std::make_shared<SlantTextStyleModifier>();
        case ModifierType::DECORATION:
            return std::make_shared<DecorationTextStyleModifier>();
        case ModifierType::FONT_FAMILIES:
            return std::make_shared<FontFamiliesTextStyleModifier>();
        case ModifierType::COUNT:
            return nullptr;
    }
    return nullptr;
}

std::shared_ptr<TextStyleModifier> TextStyleModifier::get_default_modifier(ModifierType type) {
    auto& defaultMods = get_default_modifiers();
    auto it = defaultMods.find(type);
    if(it == defaultMods.end())
        return nullptr;
    return it->second;
}

const TextStyleModifier::ModifierMap& TextStyleModifier::get_default_modifiers() {
    if(defaultMods.empty()) {
        defaultMods[ModifierType::WEIGHT] = std::make_shared<WeightTextStyleModifier>(SkFontStyle::kNormal_Weight);
        defaultMods[ModifierType::SLANT] = std::make_shared<SlantTextStyleModifier>(SkFontStyle::kUpright_Slant);
        defaultMods[ModifierType::DECORATION] = std::make_shared<DecorationTextStyleModifier>(skia::textlayout::TextDecoration::kNoDecoration);
    }
    return defaultMods;
}

bool TextStyleModifier::equivalent(TextStyleModifier& modifier) const {
    if(modifier.get_type() != get_type())
        return false;
    return equivalent_data(modifier);
}

TextStyleModifier::~TextStyleModifier() {}



WeightTextStyleModifier::WeightTextStyleModifier(SkFontStyle::Weight initWeight): weightValue(initWeight / WEIGHT_VALUE_MODIFIER) {}

TextStyleModifier::ModifierType WeightTextStyleModifier::get_type() const {
    return ModifierType::WEIGHT;
}

SkFontStyle::Weight WeightTextStyleModifier::get_weight() const {
    return static_cast<SkFontStyle::Weight>(static_cast<int>(weightValue) * WEIGHT_VALUE_MODIFIER);
}

void WeightTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(weightValue);
}

void WeightTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(weightValue);
}

void WeightTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setFontStyle(SkFontStyle(static_cast<int>(weightValue) * WEIGHT_VALUE_MODIFIER, style.getFontStyle().width(), style.getFontStyle().slant()));
}

bool WeightTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return weightValue == static_cast<WeightTextStyleModifier&>(modifier).weightValue;
}



SlantTextStyleModifier::SlantTextStyleModifier(uint8_t initSlantValue): slantValue(initSlantValue) {}

TextStyleModifier::ModifierType SlantTextStyleModifier::get_type() const {
    return ModifierType::SLANT;
}

SkFontStyle::Slant SlantTextStyleModifier::get_slant() const {
    return static_cast<SkFontStyle::Slant>(slantValue);
}

void SlantTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(slantValue);
}

void SlantTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(slantValue);
}

void SlantTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setFontStyle(SkFontStyle(style.getFontStyle().weight(), style.getFontStyle().width(), static_cast<SkFontStyle::Slant>(slantValue)));
}

bool SlantTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return slantValue == static_cast<SlantTextStyleModifier&>(modifier).slantValue;
}



ColorTextStyleModifier::ColorTextStyleModifier(const Vector4f& initColor): color(initColor) {}

TextStyleModifier::ModifierType ColorTextStyleModifier::get_type() const {
    return ModifierType::COLOR;
}

const Vector4f& ColorTextStyleModifier::get_color() const {
    return color;
}

void ColorTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(color);
}

void ColorTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(color);
}

void ColorTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setForegroundPaint(SkPaint{convert_vec4<SkColor4f>(color)});
    style.setDecorationColor(convert_vec4<SkColor4f>(color).toSkColor());
}

bool ColorTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return color == static_cast<ColorTextStyleModifier&>(modifier).color;
}



SizeTextStyleModifier::SizeTextStyleModifier(float initSize): size(initSize) {}

TextStyleModifier::ModifierType SizeTextStyleModifier::get_type() const {
    return ModifierType::SIZE;
}

float SizeTextStyleModifier::get_size() const {
    return size;
}

void SizeTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(size);
}

void SizeTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(size);
}

void SizeTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setFontSize(size);
}

bool SizeTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return size == static_cast<SizeTextStyleModifier&>(modifier).size;
}


std::shared_ptr<FontData> FontFamiliesTextStyleModifier::globalFontData = nullptr;

FontFamiliesTextStyleModifier::FontFamiliesTextStyleModifier(const std::vector<SkString>& initFamilies): families(initFamilies) {}

TextStyleModifier::ModifierType FontFamiliesTextStyleModifier::get_type() const {
    return ModifierType::FONT_FAMILIES;
}

const std::vector<SkString>& FontFamiliesTextStyleModifier::get_families() const {
    return families;
}

void FontFamiliesTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(families);
}

void FontFamiliesTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(families);
}

void FontFamiliesTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    if(!globalFontData)
        throw std::runtime_error("[FontFamiliesTextStyleModifier::modify_text_style] global font data not set");

    auto familiesToPush = families;
    globalFontData->push_default_font_families(familiesToPush);
    style.setFontFamilies(familiesToPush);
}

bool FontFamiliesTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return families == static_cast<FontFamiliesTextStyleModifier&>(modifier).families;
}



DecorationTextStyleModifier::DecorationTextStyleModifier(uint8_t initDecorationValue): decorationValue(initDecorationValue) {}

TextStyleModifier::ModifierType DecorationTextStyleModifier::get_type() const {
    return ModifierType::DECORATION;
}

uint8_t DecorationTextStyleModifier::get_decoration_value() const {
    return decorationValue;
}

void DecorationTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(decorationValue);
}

void DecorationTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(decorationValue);
}

void DecorationTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setDecoration(static_cast<skia::textlayout::TextDecoration>(decorationValue));
}

bool DecorationTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return decorationValue == static_cast<DecorationTextStyleModifier&>(modifier).decorationValue;
}

}
