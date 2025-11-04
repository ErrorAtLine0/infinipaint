#include "TextStyleModifier.hpp"
#include <Helpers/ConvertVec.hpp>
#include <Helpers/Serializers.hpp>

#define WEIGHT_VALUE_MODIFIER 100

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
        case ModifierType::FONT_FAMILIES:
            return std::make_shared<FontFamiliesTextStyleModifier>();
    }
    return nullptr;
}

bool TextStyleModifier::equivalent(TextStyleModifier& modifier) const {
    if(modifier.get_type() != get_type())
        return false;
    return equivalent_data(modifier);
}

TextStyleModifier::~TextStyleModifier() {}



TextStyleModifier::ModifierType WeightTextStyleModifier::get_type() const {
    return ModifierType::WEIGHT;
}

void WeightTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(weightValue);
}

void WeightTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(weightValue);
}

void WeightTextStyleModifier::set_weight(SkFontStyle::Weight newWeight) {
    weightValue = newWeight / WEIGHT_VALUE_MODIFIER;
}

SkFontStyle::Weight WeightTextStyleModifier::get_weight() {
    return static_cast<SkFontStyle::Weight>(static_cast<int>(weightValue) * WEIGHT_VALUE_MODIFIER);
}

void WeightTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setFontStyle(SkFontStyle(static_cast<int>(weightValue) * WEIGHT_VALUE_MODIFIER, style.getFontStyle().width(), style.getFontStyle().slant()));
}

bool WeightTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return weightValue == static_cast<WeightTextStyleModifier&>(modifier).weightValue;
}



TextStyleModifier::ModifierType SlantTextStyleModifier::get_type() const {
    return ModifierType::SLANT;
}

void SlantTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(slantValue);
}

void SlantTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(slantValue);
}

void SlantTextStyleModifier::set_slant(SkFontStyle::Slant newSlant) {
    slantValue = newSlant;
}

SkFontStyle::Slant SlantTextStyleModifier::get_slant() {
    return static_cast<SkFontStyle::Slant>(slantValue);
}

void SlantTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setFontStyle(SkFontStyle(style.getFontStyle().weight(), style.getFontStyle().width(), static_cast<SkFontStyle::Slant>(slantValue)));
}

bool SlantTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return slantValue == static_cast<SlantTextStyleModifier&>(modifier).slantValue;
}



TextStyleModifier::ModifierType ColorTextStyleModifier::get_type() const {
    return ModifierType::COLOR;
}

void ColorTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(color);
}

void ColorTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(color);
}

void ColorTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setForegroundPaint(SkPaint{convert_vec4<SkColor4f>(color)});
}

bool ColorTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return color == static_cast<ColorTextStyleModifier&>(modifier).color;
}



TextStyleModifier::ModifierType SizeTextStyleModifier::get_type() const {
    return ModifierType::SIZE;
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



TextStyleModifier::ModifierType FontFamiliesTextStyleModifier::get_type() const {
    return ModifierType::FONT_FAMILIES;
}

void FontFamiliesTextStyleModifier::save(cereal::PortableBinaryOutputArchive& a) const {
    a(families);
}

void FontFamiliesTextStyleModifier::load(cereal::PortableBinaryInputArchive& a) {
    a(families);
}

void FontFamiliesTextStyleModifier::modify_text_style(skia::textlayout::TextStyle& style) const {
    style.setFontFamilies(families);
}

bool FontFamiliesTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return families == static_cast<FontFamiliesTextStyleModifier&>(modifier).families;
}
