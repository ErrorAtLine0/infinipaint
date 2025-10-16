#include "RichTextBox.hpp"
#include <modules/skunicode/include/SkUnicode_icu.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/src/ParagraphImpl.h>
#include <Helpers/MathExtras.hpp>

RichTextBox::RichTextBox() {
}

void RichTextBox::process_key_input(Cursor& cur, InputKey in, bool ctrl, bool shift) {
    bool moved = false;

    switch(in) {
        case InputKey::LEFT:
            cur.pos = cur.selectionBeginPos = move(Movement::LEFT, cur.pos);
            moved = true;
            break;
        case InputKey::RIGHT:
            cur.pos = cur.selectionBeginPos = move(Movement::RIGHT, cur.pos);
            moved = true;
            break;
        case InputKey::UP:
            cur.pos = cur.selectionBeginPos = move(Movement::UP, cur.pos);
            moved = true;
            break;
        case InputKey::DOWN:
            cur.pos = cur.selectionBeginPos = move(Movement::DOWN, cur.pos);
            moved = true;
            break;
        case InputKey::HOME:
            cur.pos = cur.selectionBeginPos = move(Movement::HOME, cur.pos);
            moved = true;
            break;
        case InputKey::END:
            cur.pos = cur.selectionBeginPos = move(Movement::END, cur.pos);
            moved = true;
            break;
        case InputKey::BACKSPACE:
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
            else
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.pos, move(Movement::LEFT, cur.pos));
            break;
        case InputKey::DELETE:
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
            else
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.pos, move(Movement::RIGHT, cur.pos));
            break;
        case InputKey::ENTER:
            if(newlinesAllowed)
                process_text_input(cur, "\n");
            break;
        case InputKey::SELECT_ALL:
            cur.selectionBeginPos = 0;
            cur.pos = cur.selectionEndPos = text.size();
            break;
    }

    if(moved && !shift)
        cur.selectionEndPos = cur.selectionBeginPos;
}

std::string RichTextBox::process_copy(Cursor& cur) {
    uint64_t trueBegin = std::min(cur.selectionBeginPos, cur.selectionEndPos);
    uint64_t trueEnd = std::max(cur.selectionBeginPos, cur.selectionEndPos);
    return text.substr(trueBegin, trueEnd - trueBegin);
}

std::string RichTextBox::process_cut(Cursor& cur) {
    std::string toRet = process_copy(cur);
    if(cur.selectionBeginPos != cur.selectionEndPos)
        cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
    return toRet;
}

void RichTextBox::process_text_input(Cursor& cur, const std::string& in) {
    if(!in.empty()) {
        if(cur.selectionBeginPos != cur.selectionEndPos)
            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
        cur.selectionEndPos = cur.selectionBeginPos = cur.pos = insert(cur.pos, in);
    }
}

uint64_t RichTextBox::move(Movement movement, uint64_t pos) {
    rebuild();

    if(pos >= text.size())
        pos = text.size();

    switch (movement) {
        case Movement::LEFT:
            if(pos != 0)
                pos--;
            break;
        case Movement::RIGHT:
            if(pos < text.size())
                pos++;
            break;
        case Movement::UP: {
            SkRect cursorRect = get_cursor_rect(pos);
            Vector2f pointToCheck{cursorRect.x(), cursorRect.top() - 0.1f};
            pos = paragraph->getGlyphPositionAtCoordinate(pointToCheck.x(), pointToCheck.y()).position;
            break;
        }
        case Movement::DOWN: {
            SkRect cursorRect = get_cursor_rect(pos);
            Vector2f pointToCheck{cursorRect.x(), cursorRect.bottom() + 0.1f};
            pos = paragraph->getGlyphPositionAtCoordinate(pointToCheck.x(), pointToCheck.y()).position;
            break;
        }
        case Movement::HOME:
            pos = 0;
            break;
        case Movement::END:
            pos = text.size();
            break;
    }
    return pos;

}

void RichTextBox::set_width(float newWidth) {
    width = newWidth;
    needsRebuild = true;
}

void RichTextBox::set_allow_newlines(bool allow) {
    newlinesAllowed = true;
}

void RichTextBox::set_font_collection(const sk_sp<skia::textlayout::FontCollection>& fC) {
    fontCollection = fC;
    needsRebuild = true;
}

void RichTextBox::rebuild() {
    if(needsRebuild) {
        skia::textlayout::ParagraphStyle pStyle;
        pStyle.setTextAlign(skia::textlayout::TextAlign::kLeft);
        skia::textlayout::TextStyle tStyle;
        tStyle.setFontSize(20.0f);
        tStyle.setFontFamilies({SkString{"Roboto"}});
        pStyle.setTextStyle(tStyle);

        skia::textlayout::ParagraphBuilderImpl a(pStyle, fontCollection, SkUnicodes::ICU::Make());
        a.addText(text.c_str(), text.length());
        paragraph = a.Build();
        paragraph->layout(width);
    }
}

uint64_t RichTextBox::insert(uint64_t pos, const std::string& textToInsert) {
    text.insert(pos, textToInsert);
    needsRebuild = true;
    return pos + textToInsert.size();
}

uint64_t RichTextBox::remove(uint64_t start, uint64_t end) {
    if(start > end)
        std::swap(start, end);

    text.erase(start, end - start);
    needsRebuild = true;
    return start;
}

SkRect RichTextBox::get_cursor_rect(uint64_t pos) {
    SkPoint topLeft;
    float height;

    if(text.empty()) {
        topLeft = {0.0f, 0.0f};
        height = paragraph->getHeight();
    }
    else if(pos == text.size()) {
        std::vector<skia::textlayout::TextBox> posBoxes = paragraph->getRectsForRange(pos - 1, pos, skia::textlayout::RectHeightStyle::kMax, skia::textlayout::RectWidthStyle::kMax);
        if(text.back() == '\n') {
            topLeft = {0.0f, posBoxes.back().rect.bottom()};
            height = std::abs(paragraph->getHeight() - topLeft.y());
        }
        else {
            topLeft = posBoxes.front().rect.TR();
            height = posBoxes.front().rect.height();
        }
    }
    else {
        std::vector<skia::textlayout::TextBox> posBoxes = paragraph->getRectsForRange(pos, pos + 1, skia::textlayout::RectHeightStyle::kMax, skia::textlayout::RectWidthStyle::kMax);
        topLeft = posBoxes.front().rect.TL();
        height = posBoxes.front().rect.height();
    }
    return SkRect::MakeXYWH(topLeft.x(), topLeft.y(), 3.0f, height);
}

void RichTextBox::paint(SkCanvas* canvas, const PaintOpts& paintOpts) {
    rebuild();

    if(paintOpts.cursor.has_value()) {
        auto& cur = paintOpts.cursor.value();
        SkPaint selection{color_mul_alpha(paintOpts.cursorColor, 0.6f)};
        uint64_t trueSelectionStart = std::min(cur.selectionBeginPos, cur.selectionEndPos);
        uint64_t trueSelectionEnd = std::max(cur.selectionBeginPos, cur.selectionEndPos);
        std::vector<skia::textlayout::TextBox> selectionBoxes = paragraph->getRectsForRange(trueSelectionStart, trueSelectionEnd, skia::textlayout::RectHeightStyle::kTight, skia::textlayout::RectWidthStyle::kTight);
        for(auto& selectionBox : selectionBoxes) {
            SkRect rectFixSmallOverlap = SkRect::MakeXYWH(selectionBox.rect.x(), selectionBox.rect.y(), selectionBox.rect.width(), std::max(selectionBox.rect.height() - 0.25f, 0.0f));
            canvas->drawRect(rectFixSmallOverlap, selection);
        }

        if(cur.pos <= text.size()) {
            canvas->drawRect(get_cursor_rect(cur.pos), SkPaint{paintOpts.cursorColor});
        }
    }

    paragraph->paint(canvas, 0.0f, 0.0f);
}
