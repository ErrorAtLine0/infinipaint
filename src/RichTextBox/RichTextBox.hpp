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

        TextPosition move(Movement movement, TextPosition pos, std::optional<float>* previousX = nullptr);
        void set_width(float newWidth);
        TextPosition insert(TextPosition pos, const std::string& textToInsert);
        TextPosition remove(TextPosition start, TextPosition end);
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

        void process_text_input(Cursor& cur, const std::string& in);
    private:
        size_t count_grapheme();
        size_t next_grapheme(size_t textBytePos);
        size_t prev_grapheme(size_t textBytePos);

        std::vector<skia::textlayout::TextBox> rects_between_text_positions(TextPosition start, TextPosition end);
        std::vector<SkRect> rects_between_text_positions_2(TextPosition start, TextPosition end);
        void rects_between_text_positions_func(TextPosition start, TextPosition end, std::function<void(const SkRect& r)> f);

        SkRect get_cursor_rect(TextPosition pos);

        std::optional<TextPosition> get_text_pos_closest_to_point(const Vector2f& point);

        void rebuild();

        bool newlinesAllowed = true;
        float width = 0.0f;
        float fontSize = 0.0f;
        bool needsRebuild = true;
        sk_sp<skia::textlayout::FontCollection> fontCollection;
        std::unique_ptr<skia::textlayout::Paragraph> paragraph;
        std::string text;
};
