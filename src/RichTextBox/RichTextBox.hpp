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
            HOME,
            END
        };

        struct Cursor {
            uint64_t pos = 0;
            uint64_t selectionBeginPos = 0;
            uint64_t selectionEndPos = 0;
        };

        struct PaintOpts {
            SkColor4f cursorColor = {1, 0, 0, 1};
            std::optional<Cursor> cursor;
        };

        uint64_t move(Movement movement, uint64_t pos);
        void set_width(float newWidth);
        uint64_t insert(uint64_t pos, const std::string& textToInsert);
        uint64_t remove(uint64_t start, uint64_t end);
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

        void process_key_input(Cursor& cur, InputKey in, bool ctrl, bool shift);
        std::string process_copy(Cursor& cur);
        std::string process_cut(Cursor& cur);
        void process_text_input(Cursor& cur, const std::string& in);
    private:
        SkRect get_cursor_rect(uint64_t pos);

        void rebuild();

        bool newlinesAllowed = true;
        float width = 100.0f;
        float fontSize = 0.0f;
        bool needsRebuild = true;
        sk_sp<skia::textlayout::FontCollection> fontCollection;
        std::unique_ptr<skia::textlayout::Paragraph> paragraph;
        std::string text;
};
