#pragma once
#include <cstdint>
#include <include/core/SkCanvas.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/Paragraph.h>
#include <modules/skparagraph/include/TextStyle.h>
#include "../SharedTypes.hpp"
#include "TextStyleModifier.hpp"
#include "cereal/archives/portable_binary.hpp"

class RichTextBox {
    public:
        RichTextBox();

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

        struct TextPosition {
            // Set to uint32_t values because theyll be saved and sent over networks. Size has to be consistent
            size_t fParagraphIndex = std::numeric_limits<uint32_t>::max();
            size_t fTextByteIndex = std::numeric_limits<uint32_t>::max();
            template <typename Archive> void serialize(Archive& a) {
                a((uint32_t)fParagraphIndex, (uint32_t)fTextByteIndex);
            }
            bool operator==(const TextPosition& o) const = default;
            bool operator<(const TextPosition& o) const;
            bool operator>(const TextPosition& o) const;
        };

        struct TextStyleRangeModifier {
            TextPosition start;
            TextPosition end;
            std::shared_ptr<TextStyleModifier> modifier;
            void apply_to_start_and_end(const std::function<void(TextPosition&)>& f);
            void save(cereal::PortableBinaryOutputArchive& a) const;
            void load(cereal::PortableBinaryInputArchive& a);
        };

        struct Cursor {
            TextPosition pos = {0, 0};
            TextPosition selectionBeginPos = {0, 0};
            TextPosition selectionEndPos = {0, 0};
            template <typename Archive> void serialize(Archive& a) {
                a(pos, selectionBeginPos, selectionEndPos);
            }
            std::optional<float> previousX;
        };

        struct PaintOpts {
            Vector3f cursorColor = {1, 0, 0};
            std::optional<Cursor> cursor;
        };

        struct RichTextData {
            struct Paragraph {
                std::string text;
                template<typename Archive> void serialize(Archive& a) {
                    a(text);
                }
            };
            template<typename Archive> void serialize(Archive& a) {
                a(paragraphs, tStyleModifiers);
            }
            std::vector<Paragraph> paragraphs;
            std::vector<TextStyleRangeModifier> tStyleModifiers;
        };

        TextPosition move(Movement movement, TextPosition pos, std::optional<float>* previousX = nullptr, bool flipDependingOnTextDirection = false);
        void set_width(float newWidth);
        TextPosition insert(TextPosition pos, std::string_view textToInsert);
        TextPosition remove(TextPosition p1, TextPosition p2);
        void set_font_collection(const sk_sp<skia::textlayout::FontCollection>& fC);
        void paint(SkCanvas* canvas, const PaintOpts& paintOpts);
        void set_allow_newlines(bool allow);

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
        void set_text_style_modifier_between(TextPosition p1, TextPosition p2, const std::shared_ptr<TextStyleModifier>& modifier);

        RichTextData get_rich_text_data();
        void set_rich_text_data(const RichTextData& richText);
        void clear_text();
        void set_string(const std::string& str);

        void process_mouse_left_button(Cursor& cur, const Vector2f& pos, bool clicked, bool held, bool shift);
        void process_key_input(Cursor& cur, InputKey in, bool ctrl, bool shift);
        std::string process_copy(Cursor& cur);
        std::string process_cut(Cursor& cur);
        std::string get_text_between(TextPosition p1, TextPosition p2);
        std::string get_string();
        void set_tab_space_width(unsigned newTabWidth);
        void process_text_input(Cursor& cur, const std::string& in);

    private:
        static size_t count_grapheme(const std::string& text);
        static size_t next_grapheme(const std::string& text, size_t textBytePos);
        static size_t prev_grapheme(const std::string& text, size_t textBytePos);
        static TextPosition get_text_pos_from_byte_pos(const std::string& text, size_t textBytePos);

        size_t get_byte_pos_from_text_pos(TextPosition textPos);

        void rects_between_text_positions_func(TextPosition p1, TextPosition p2, std::function<void(const SkRect& r)> f);

        void fit_text_style_ranges_in_text();

        SkRect get_cursor_rect(TextPosition pos);

        TextPosition get_text_pos_closest_to_point(Vector2f point);

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
            skia::textlayout::ParagraphStyle pStyle;
        };

        skia::textlayout::TextStyle initialTStyle;
        std::vector<ParagraphData> paragraphs;
        std::vector<TextStyleRangeModifier> tStyleModifiers;
};
