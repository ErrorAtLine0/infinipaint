#pragma once
#include <cstdint>
#include <include/core/SkCanvas.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/Paragraph.h>
#include "../SharedTypes.hpp"

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
            size_t fTextByteIndex = std::numeric_limits<uint32_t>::max();
            size_t fParagraphIndex = std::numeric_limits<uint32_t>::max();
            template <typename Archive> void serialize(Archive& a) {
                a((uint32_t)fTextByteIndex, (uint32_t)fParagraphIndex);
            }
            bool operator==(const TextPosition& o) const = default;
            bool operator<(const TextPosition& o) const;
        };

        struct Cursor {
            TextPosition pos = {0, 0};
            TextPosition selectionBeginPos = {0, 0};
            TextPosition selectionEndPos = {0, 0};
            std::optional<float> previousX;
        };

        struct PaintOpts {
            Vector3f cursorColor = {1, 0, 0};
            std::optional<Cursor> cursor;
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
            SELECT_ALL
        };

        void process_mouse_left_button(Cursor& cur, const Vector2f& pos, bool clicked, bool held, bool shift);
        void process_key_input(Cursor& cur, InputKey in, bool ctrl, bool shift);
        std::string process_copy(Cursor& cur);
        std::string process_cut(Cursor& cur);
        std::string get_text_between(TextPosition p1, TextPosition p2);

        void process_text_input(Cursor& cur, const std::string& in);
    private:
        static size_t count_grapheme(const std::string& text);
        static size_t next_grapheme(const std::string& text, size_t textBytePos);
        static size_t prev_grapheme(const std::string& text, size_t textBytePos);
        static TextPosition get_text_pos_from_byte_pos(const std::string& text, size_t textBytePos);
        size_t get_byte_pos_from_text_pos(TextPosition textPos);

        void rects_between_text_positions_func(TextPosition p1, TextPosition p2, std::function<void(const SkRect& r)> f);

        SkRect get_cursor_rect(TextPosition pos);

        TextPosition get_text_pos_closest_to_point(Vector2f point);

        void rebuild();

        bool newlinesAllowed = true;
        float width = 0.0f;
        float fontSize = 0.0f;
        bool needsRebuild = true;
        sk_sp<skia::textlayout::FontCollection> fontCollection;

        struct ParagraphData {
            std::string text;
            std::unique_ptr<skia::textlayout::Paragraph> p;
            float heightOffset;
            skia::textlayout::ParagraphStyle pStyle;
        };

        std::vector<ParagraphData> paragraphs;
};
