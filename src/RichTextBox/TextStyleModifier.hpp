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
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        uint8_t decorationValue;
    private:
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
};

class WeightTextStyleModifier : public TextStyleModifier {
    public:
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        void set_weight(SkFontStyle::Weight newWeight);
        SkFontStyle::Weight get_weight();
    private:
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
        uint8_t weightValue;
};

class SlantTextStyleModifier : public TextStyleModifier {
    public:
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        void set_slant(SkFontStyle::Slant newSlant);
        SkFontStyle::Slant get_slant();
    private:
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
        uint8_t slantValue;
};

class ColorTextStyleModifier : public TextStyleModifier {
    public:
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        Vector4f color;
    private:
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
};

class SizeTextStyleModifier : public TextStyleModifier {
    public:
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        float size;
    private:
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
};

class FontFamiliesTextStyleModifier : public TextStyleModifier {
    public:
        virtual ModifierType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void modify_text_style(skia::textlayout::TextStyle& style) const override;
        std::vector<SkString> families;
    private:
        virtual bool equivalent_data(TextStyleModifier& modifier) const override;
};
