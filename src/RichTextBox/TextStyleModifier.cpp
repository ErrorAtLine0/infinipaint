#include "TextStyleModifier.hpp"
#include <Helpers/ConvertVec.hpp>
#include <Helpers/Serializers.hpp>
#include <include/core/SkFontStyle.h>
#include <modules/skparagraph/include/TextStyle.h>

#define WEIGHT_VALUE_MODIFIER 100

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
        auto weightMod = std::make_shared<WeightTextStyleModifier>();
        weightMod->set_weight(SkFontStyle::kNormal_Weight);
        defaultMods[ModifierType::WEIGHT] = weightMod;

        auto slantMod = std::make_shared<SlantTextStyleModifier>();
        slantMod->set_slant(SkFontStyle::kUpright_Slant);
        defaultMods[ModifierType::SLANT] = slantMod;

        auto decorationMod = std::make_shared<DecorationTextStyleModifier>();
        decorationMod->decorationValue = skia::textlayout::TextDecoration::kNoDecoration;
        defaultMods[ModifierType::DECORATION] = decorationMod;
    }
    return defaultMods;
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
    style.setDecorationColor(convert_vec4<SkColor4f>(color).toSkColor());
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
    auto familiesToPush = families;
    familiesToPush.emplace_back("Roboto");
    style.setFontFamilies(familiesToPush);
}

bool FontFamiliesTextStyleModifier::equivalent_data(TextStyleModifier& modifier) const {
    return families == static_cast<FontFamiliesTextStyleModifier&>(modifier).families;
}



TextStyleModifier::ModifierType DecorationTextStyleModifier::get_type() const {
    return ModifierType::DECORATION;
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
