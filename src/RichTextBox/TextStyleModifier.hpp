#pragma once
#include <cstdint>
#include <modules/skparagraph/include/Paragraph.h>
#include <modules/skparagraph/include/TextStyle.h>
#include "cereal/archives/portable_binary.hpp"
#include "../SharedTypes.hpp"

class TextStyleModifier {
    public:
        enum class ModifierType : uint16_t {
            SIZE = 0,
            COLOR,
            WEIGHT,
            SLANT,
            DECORATION,
            FONT_FAMILIES,

            COUNT
        };

        typedef std::unordered_map<ModifierType, std::shared_ptr<TextStyleModifier>> ModifierMap;

        static std::shared_ptr<TextStyleModifier> allocate_modifier(ModifierType type);
        static std::shared_ptr<TextStyleModifier> get_default_modifier(ModifierType type);
        static const ModifierMap& get_default_modifiers();
        virtual ModifierType get_type() const = 0;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load(cereal::PortableBinaryInputArchive& a) = 0;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const = 0;
        bool equivalent(TextStyleModifier& modifier) const;
        virtual ~TextStyleModifier();
    private:
        static ModifierMap defaultMods;
        virtual bool equivalent_data(TextStyleModifier& modifier) const = 0;
};

class DecorationTextStyleModifier : public TextStyleModifier {
    public:
        DecorationTextStyleModifier() = default;
        DecorationTextStyleModifier(uint8_t initDecorationValue);
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        uint8_t get_decoration_value() const;
    private:
        uint8_t decorationValue;
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
};

class WeightTextStyleModifier : public TextStyleModifier {
    public:
        WeightTextStyleModifier() = default;
        WeightTextStyleModifier(SkFontStyle::Weight initWeight);
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        SkFontStyle::Weight get_weight() const;
    private:
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
        uint8_t weightValue;
};

class SlantTextStyleModifier : public TextStyleModifier {
    public:
        SlantTextStyleModifier() = default;
        SlantTextStyleModifier(uint8_t initSlantValue);
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        SkFontStyle::Slant get_slant() const;
    private:
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
        uint8_t slantValue;
};

class ColorTextStyleModifier : public TextStyleModifier {
    public:
        ColorTextStyleModifier() = default;
        ColorTextStyleModifier(const Vector4f& initColor);
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        const Vector4f& get_color() const;
    private:
        Vector4f color;
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
};

class SizeTextStyleModifier : public TextStyleModifier {
    public:
        SizeTextStyleModifier() = default;
        SizeTextStyleModifier(float initSize);
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        float get_size() const;
    private:
        float size;
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
};

class FontFamiliesTextStyleModifier : public TextStyleModifier {
    public:
        FontFamiliesTextStyleModifier() = default;
        FontFamiliesTextStyleModifier(const std::vector<SkString>& initFamilies);
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        const std::vector<SkString>& get_families() const;
    private:
        std::vector<SkString> families;
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
};
