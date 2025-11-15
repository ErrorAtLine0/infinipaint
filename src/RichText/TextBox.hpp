#pragma once
#include <cstdint>
#include <cwchar>
#include <include/core/SkCanvas.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/Paragraph.h>
#include <modules/skparagraph/include/TextStyle.h>
#include "../SharedTypes.hpp"
#include "TextStyleModifier.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "ParagraphStyleData.hpp"

namespace RichText {

struct TextPosition {
    // Set to uint32_t values because theyll be saved and sent over networks. Size has to be consistent
    size_t fParagraphIndex = std::numeric_limits<uint32_t>::max();
    size_t fTextByteIndex = std::numeric_limits<uint32_t>::max();
    template <typename Archive> void save(Archive& a) const {
        a(static_cast<uint32_t>(fParagraphIndex), static_cast<uint32_t>(fTextByteIndex));
    }
    template <typename Archive> void load(Archive& a) {
        uint32_t p, t;
        a(p, t);
        fParagraphIndex = p;
        fTextByteIndex = t;
    }
    bool operator==(const TextPosition& o) const = default;
    bool operator<(const TextPosition& o) const;
    bool operator>(const TextPosition& o) const;
    bool operator>=(const TextPosition& o) const;
    bool operator<=(const TextPosition& o) const;
};

struct PositionedTextStyleMod {
    TextPosition pos;
    TextStyleModifier::ModifierMap mods;
};

typedef std::vector<PositionedTextStyleMod> TextStyleModContainer;

class TextData {
    public:
        struct Paragraph {
            std::string text;
            ParagraphStyleData pStyleData;
            template<typename Archive> void serialize(Archive& a) {
                a(text, pStyleData);
            }
        };
        std::string get_serialized() const;
        static TextData deserialize_string(std::string& s);
        std::string get_plain_text() const;
        void save(cereal::PortableBinaryOutputArchive& a) const;
        void load(cereal::PortableBinaryInputArchive& a);
        TextStyleModContainer tStyleMods;
        std::vector<Paragraph> paragraphs;
};

class TextBox {
    public:
        TextBox();

        bool inputChangedTextBox = false;

        enum class Movement {
            NOWHERE,
            LEFT,
            RIGHT,
            UP,
            DOWN,
            LEFT_WORD,
            RIGHT_WORD,
            HOME,
            END
        };

        struct Cursor {
            TextPosition pos = {0, 0};
            TextPosition selectionBeginPos = {0, 0};
            TextPosition selectionEndPos = {0, 0};
            template <typename Archive> void serialize(Archive& a) {
                a(pos, selectionBeginPos, selectionEndPos);
            }
            std::optional<float> previousX;
            bool operator==(const Cursor& o) const;
            bool operator!=(const Cursor& o) const;
        };

        struct PaintOpts {
            Vector3f cursorColor = {1, 0, 0};
            std::optional<Cursor> cursor;
        };

        TextPosition move(Movement movement, TextPosition pos, std::optional<float>* previousX = nullptr, bool flipDependingOnTextDirection = false);
        void set_width(float newWidth);
        TextPosition insert(TextPosition pos, std::string_view textToInsert, const std::optional<TextStyleModifier::ModifierMap>& inputModMap = std::nullopt);
        TextPosition remove(TextPosition p1, TextPosition p2);
        void set_font_collection(const sk_sp<skia::textlayout::FontCollection>& fC);
        void paint(SkCanvas* canvas, const PaintOpts& paintOpts);
        void set_allow_newlines(bool allow);
        float get_height();

        enum class InputKey {
            LEFT,
            RIGHT,
            UP,
            DOWN,
            HOME,
            END,
            BACKSPACE,
            DELETE,
            ENTER,
            TAB,
            SELECT_ALL
        };

        void set_initial_text_style(const skia::textlayout::TextStyle& tStyle);
        void set_initial_text_style_modifier(const std::shared_ptr<TextStyleModifier>& modifier);
        void set_text_style_modifier_between(TextPosition p1, TextPosition p2, const std::shared_ptr<TextStyleModifier>& modifier);

        void set_text_alignment_between(size_t paragraphIndex1, size_t paragraphIndex2, skia::textlayout::TextAlign newAlignment);
        void set_text_direction_between(size_t paragraphIndex1, size_t paragraphIndex2, skia::textlayout::TextDirection newDirection);
        ParagraphStyleData get_paragraph_style_data_at(size_t paragraphIndex);

        TextStyleModifier::ModifierMap get_mods_used_at_pos(TextPosition p);

        TextData get_rich_text_data();
        void set_rich_text_data(const TextData& richText);

        TextData get_rich_text_data_between(TextPosition p1, TextPosition p2);
        TextPosition insert_rich_text(TextPosition p, const TextData& richText);

        void clear_text();
        void set_string(const std::string& str);

        void process_mouse_left_button(Cursor& cur, const Vector2f& pos, bool clicked, bool held, bool shift);
        void process_key_input(Cursor& cur, InputKey in, bool ctrl, bool shift, const std::optional<TextStyleModifier::ModifierMap>& inputModMap = std::nullopt);
        std::pair<std::string, TextData> process_copy(Cursor& cur);
        std::pair<std::string, TextData> process_cut(Cursor& cur);
        std::string get_text_between(TextPosition p1, TextPosition p2);
        std::string get_string();
        void set_tab_space_width(unsigned newTabWidth);
        void process_text_input(Cursor& cur, const std::string& in, const std::optional<TextStyleModifier::ModifierMap>& inputModMap = std::nullopt);
        void process_rich_text_input(Cursor& cur, const TextData& richText);

    private:
        std::pair<TextPosition, TextPosition> get_start_end_text_pos(TextPosition p1, TextPosition p2);
        std::pair<size_t, size_t> get_start_end_paragraph_pos(size_t p1, size_t p2);

        static size_t count_grapheme(const std::string& text);
        static size_t next_grapheme(const std::string& text, size_t textBytePos);
        static size_t prev_grapheme(const std::string& text, size_t textBytePos);
        static TextPosition get_text_pos_from_byte_pos(const std::string& text, size_t textBytePos);

        size_t get_byte_pos_from_text_pos(TextPosition textPos);

        void rects_between_text_positions_func(TextPosition p1, TextPosition p2, std::function<void(const SkRect& r)> f);

        SkRect get_cursor_rect(TextPosition pos);

        TextPosition get_text_pos_closest_to_point(Vector2f point);
        std::shared_ptr<TextStyleModifier> get_last_text_style_mod_before_pos(TextPosition pos, TextStyleModifier::ModifierType modType);
        void erase_if_over_all_styles_until_pos(TextPosition pos, const std::function<bool(TextPosition, const std::shared_ptr<TextStyleModifier>&)>& func);
        void insert_style_at_pos(TextPosition pos, const std::shared_ptr<TextStyleModifier>& modifier);

        void remove_duplicate_text_style_mods();

        void rebuild();

        bool newlinesAllowed = true;
        float width = 0.0f;
        bool needsRebuild = true;
        unsigned tabWidth = 8;
        sk_sp<skia::textlayout::FontCollection> fontCollection;

        struct ParagraphData {
            std::string text;
            std::unique_ptr<skia::textlayout::Paragraph> p;
            float heightOffset;
            ParagraphStyleData pStyleData;
        };

        skia::textlayout::TextStyle initialTStyle;
        TextStyleModContainer tStyleMods;
        std::vector<ParagraphData> paragraphs;
};

}
