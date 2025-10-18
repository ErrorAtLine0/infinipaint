#include "RichTextBox.hpp"
#include <limits>
#include <modules/skparagraph/include/Metrics.h>
#include <modules/skparagraph/src/TextLine.h>
#include <modules/skunicode/include/SkUnicode.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/src/ParagraphImpl.h>
#include <Helpers/MathExtras.hpp>
#include <src/base/SkUTF.h>

bool RichTextBox::TextPosition::operator<(const RichTextBox::TextPosition& o) const {
    return (this->fParagraphIndex < o.fParagraphIndex) || (this->fParagraphIndex == o.fParagraphIndex && this->fTextByteIndex < o.fTextByteIndex);
}

RichTextBox::RichTextBox() {
}

void RichTextBox::process_key_input(Cursor& cur, InputKey in, bool ctrl, bool shift) {
    bool moved = false;

    switch(in) {
        case InputKey::LEFT:
            cur.pos = cur.selectionBeginPos = move(ctrl ? Movement::LEFT_WORD : Movement::LEFT, cur.pos);
            moved = true;
            break;
        case InputKey::RIGHT:
            cur.pos = cur.selectionBeginPos = move(ctrl ? Movement::RIGHT_WORD : Movement::RIGHT, cur.pos);
            moved = true;
            break;
        case InputKey::UP:
            cur.pos = cur.selectionBeginPos = move(Movement::UP, cur.pos, &cur.previousX);
            moved = true;
            break;
        case InputKey::DOWN:
            cur.pos = cur.selectionBeginPos = move(Movement::DOWN, cur.pos, &cur.previousX);
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
            cur.selectionBeginPos = {0, 0};
            cur.pos = cur.selectionEndPos = {text.size(), 0};
            break;
    }

    if(moved && !shift)
        cur.selectionEndPos = cur.selectionBeginPos;

    if(in != InputKey::UP && in != InputKey::DOWN)
        cur.previousX = std::nullopt;
}

void RichTextBox::process_mouse_left_button(Cursor& cur, const Vector2f& pos, bool clicked, bool held, bool shift) {
    if(clicked || held) {
        rebuild();

        std::optional<TextPosition> newPos = get_text_pos_closest_to_point(pos);
        if(newPos.has_value()) {
            cur.pos = newPos.value();
            cur.selectionBeginPos = cur.pos;
            if(clicked && !shift)
                cur.selectionEndPos = cur.selectionBeginPos;
            cur.previousX = std::nullopt;
        }
    }
}

std::string RichTextBox::process_copy(Cursor& cur) {
    TextPosition trueBegin = std::min(cur.selectionBeginPos, cur.selectionEndPos);
    TextPosition trueEnd = std::max(cur.selectionBeginPos, cur.selectionEndPos);
    return text.substr(trueBegin.fTextByteIndex, trueEnd.fTextByteIndex - trueBegin.fTextByteIndex);
}

std::string RichTextBox::process_cut(Cursor& cur) {
    std::string toRet = process_copy(cur);
    if(cur.selectionBeginPos != cur.selectionEndPos) {
        cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
        cur.previousX = std::nullopt;
    }
    return toRet;
}

void RichTextBox::process_text_input(Cursor& cur, const std::string& in) {
    if(!in.empty()) {
        if(cur.selectionBeginPos != cur.selectionEndPos)
            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
        cur.selectionEndPos = cur.selectionBeginPos = cur.pos = insert(cur.pos, in);
        cur.previousX = std::nullopt;
    }
}

size_t RichTextBox::count_grapheme() {
    rebuild();
    auto u = SkUnicodes::ICU::Make();
    auto breakIterator = u->makeBreakIterator(SkUnicode::BreakType::kGraphemes);
    breakIterator->setText(text.c_str(), text.size());
    size_t count = 0;
    while(!breakIterator->isDone()) {
        breakIterator->next();
        count++;
    }
    return count - 1;
}

size_t RichTextBox::next_grapheme(size_t textBytePos) {
    rebuild();
    if(textBytePos == text.size())
        return text.size();
    auto u = SkUnicodes::ICU::Make();
    auto breakIterator = u->makeBreakIterator(SkUnicode::BreakType::kGraphemes);
    breakIterator->setText(text.c_str() + textBytePos, text.size() - textBytePos);
    size_t count = breakIterator->next();
    return count + textBytePos;
}

size_t RichTextBox::prev_grapheme(size_t textBytePos) {
    rebuild();
    if(textBytePos == 0)
        return 0;
    skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
    bool glyphClusterFound = paragraph->getGlyphClusterAt(textBytePos - 1, &glyphInfo);
    if(glyphClusterFound)
        return glyphInfo.fClusterTextRange.start;
    return std::numeric_limits<size_t>::max();
}

std::optional<RichTextBox::TextPosition> RichTextBox::get_text_pos_closest_to_point(const Vector2f& point) {
    skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
    bool foundGlyph = paragraph->getClosestGlyphClusterAt(point.x(), point.y(), &glyphInfo);
    if(foundGlyph) {
        TextPosition toRet = {0, 0};
        toRet.fTextByteIndex = glyphInfo.fClusterTextRange.start;
        if(std::abs(glyphInfo.fBounds.right() - point.x()) < std::abs(glyphInfo.fBounds.left() - point.x()))
            toRet.fTextByteIndex = next_grapheme(toRet.fTextByteIndex);
        return toRet;
    }
    return std::nullopt;
}

RichTextBox::TextPosition RichTextBox::move(Movement movement, TextPosition pos, std::optional<float>* previousX) {
    rebuild();

    if(pos.fTextByteIndex >= text.size())
        pos.fTextByteIndex = text.size();

    switch (movement) {
        case Movement::LEFT: {
            pos.fTextByteIndex = prev_grapheme(pos.fTextByteIndex);
            break;
        }
        case Movement::RIGHT: {
            pos.fTextByteIndex = next_grapheme(pos.fTextByteIndex);
            break;
        }
        case Movement::UP: {
            assert(previousX != nullptr);
            if(text.size() == 0)
                break;
            if(pos.fTextByteIndex == 0)
                break;
            int lineNumber = paragraph->getLineNumberAt(prev_grapheme(pos.fTextByteIndex));
            if(lineNumber == -1)
                break;
            if(lineNumber == 0) {
                pos.fTextByteIndex = 0;
                if(previousX != nullptr)
                    *previousX = get_cursor_rect(pos).centerX();
            }
            else {
                skia::textlayout::LineMetrics prevLineMetrics;
                paragraph->getLineMetricsAt(lineNumber - 1, &prevLineMetrics);
                SkRect cursorRect = get_cursor_rect(pos);
                Vector2f pointToCheck = {(previousX != nullptr && previousX->has_value()) ? previousX->value() : cursorRect.centerX(), prevLineMetrics.fBaseline};

                std::optional<TextPosition> newPos = get_text_pos_closest_to_point(pointToCheck);
                if(newPos.has_value())
                    pos = newPos.value();
                if(previousX != nullptr && !previousX->has_value()) {
                    *previousX = pointToCheck.x();
                }
            }
            break;
        }
        case Movement::DOWN: {
            assert(previousX != nullptr);
            if(text.size() == 0)
                break;
            if(pos.fTextByteIndex == text.size())
                break;
            int lineNumber = paragraph->getLineNumberAt(pos.fTextByteIndex);
            if(lineNumber == -1)
                break;
            if(lineNumber == static_cast<int>(paragraph->lineNumber() - 1)) {
                pos.fTextByteIndex = text.size();
                if(previousX != nullptr)
                    *previousX = get_cursor_rect(pos).centerX();
            }
            else {
                skia::textlayout::LineMetrics nextLineMetrics;
                paragraph->getLineMetricsAt(lineNumber + 1, &nextLineMetrics);
                SkRect cursorRect = get_cursor_rect(pos);
                Vector2f pointToCheck{(previousX != nullptr && previousX->has_value()) ? previousX->value() : cursorRect.centerX(), nextLineMetrics.fBaseline};

                std::optional<TextPosition> newPos = get_text_pos_closest_to_point(pointToCheck);
                if(newPos.has_value())
                    pos = newPos.value();
                if(previousX != nullptr && !previousX->has_value()) {
                    *previousX = pointToCheck.x();
                }
            }
            break;
        }
        case Movement::LEFT_WORD: {
            if(pos.fTextByteIndex == 0)
                break;
            auto u = SkUnicodes::ICU::Make();

            size_t byteIndexToRet = 0;

            auto breakIterator = u->makeBreakIterator(SkUnicode::BreakType::kWords);
            breakIterator->setText(text.c_str(), text.size());

            while(!breakIterator->isDone()) {
                size_t nextByteIndex = breakIterator->next();
                if(nextByteIndex >= pos.fTextByteIndex)
                    return {byteIndexToRet, 0};

                const char* tPtr = text.c_str() + nextByteIndex;
                SkUnichar u = SkUTF::NextUTF8(&tPtr, text.c_str() + text.size());
                if(!SkUnicodes::ICU::Make()->isWhitespace(u))
                    byteIndexToRet = nextByteIndex;
            }
            break;
        }
        case Movement::RIGHT_WORD: {
            if(pos.fTextByteIndex == text.size())
                return pos;
            auto u = SkUnicodes::ICU::Make();
            auto breakIterator = u->makeBreakIterator(SkUnicode::BreakType::kWords);
            breakIterator->setText(text.c_str() + pos.fTextByteIndex, text.size() - pos.fTextByteIndex);
            while(!breakIterator->isDone()) {
                TextPosition p = {breakIterator->next() + pos.fTextByteIndex, 0};
                if(p.fTextByteIndex == text.size())
                    return p;
                const char* tPtr = text.c_str() + p.fTextByteIndex;
                SkUnichar u = SkUTF::NextUTF8(&tPtr, text.c_str() + text.size());
                if(!SkUnicodes::ICU::Make()->isWhitespace(u))
                    return p;
            }
            break;
        }
        case Movement::HOME:
            pos.fTextByteIndex = 0;
            break;
        case Movement::END:
            pos.fTextByteIndex = text.size();
            break;
    }
    return pos;

}

void RichTextBox::set_width(float newWidth) {
    if(width != newWidth) {
        width = newWidth;
        needsRebuild = true;
    }
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
        pStyle.setTextDirection(skia::textlayout::TextDirection::kLtr);
        skia::textlayout::TextStyle tStyle;
        tStyle.setFontSize(20.0f);
        tStyle.setFontFamilies({SkString{"Roboto"}});
        pStyle.setTextStyle(tStyle);

        skia::textlayout::ParagraphBuilderImpl a(pStyle, fontCollection, SkUnicodes::ICU::Make());
        a.addText(text.c_str(), text.length());
        paragraph = a.Build();
        paragraph->layout(width);
        needsRebuild = false;
    }
}

RichTextBox::TextPosition RichTextBox::insert(TextPosition pos, const std::string& textToInsert) {
    text.insert(pos.fTextByteIndex, textToInsert);
    needsRebuild = true;
    return TextPosition{pos.fTextByteIndex + textToInsert.size()};
}

RichTextBox::TextPosition RichTextBox::remove(TextPosition start, TextPosition end) {
    TextPosition trueBegin = std::min(start, end);
    TextPosition trueEnd = std::max(start, end);

    text.erase(trueBegin.fTextByteIndex, trueEnd.fTextByteIndex - trueBegin.fTextByteIndex);
    needsRebuild = true;
    return trueBegin;
}

SkRect RichTextBox::get_cursor_rect(TextPosition pos) {
    SkPoint topLeft;
    float height;

    if(text.empty()) {
        topLeft = {0.0f, 0.0f};
        height = paragraph->getHeight();
    }
    else {
        if(pos.fTextByteIndex == text.size()) {
            skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
            bool exists = paragraph->getGlyphClusterAt(prev_grapheme(pos.fTextByteIndex), &glyphInfo);
            if(exists) {
                topLeft = glyphInfo.fBounds.TR();
                height = glyphInfo.fBounds.height();
            }
            else {
                topLeft = {0.0f, 0.0f};
                height = 0.0f;
            }
        }
        else {
            skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
            bool exists = paragraph->getGlyphClusterAt(pos.fTextByteIndex, &glyphInfo);
            if(exists) {
                topLeft = glyphInfo.fBounds.TL();
                height = glyphInfo.fBounds.height();
            }
            else {
                topLeft = {0.0f, 0.0f};
                height = 0.0f;
            }
        }
    }
    return SkRect::MakeXYWH(topLeft.x(), topLeft.y(), 3.0f, height);
}

std::vector<skia::textlayout::TextBox> RichTextBox::rects_between_text_positions(TextPosition start, TextPosition end) {
    TextPosition trueStart = std::min(start, end);
    TextPosition trueEnd = std::max(start, end);

    unsigned glyphID1 = 0, glyphID2 = 0;

    // Commenting this so that illegal instructions become more apparent (they're there even if its uncommented, but can't be caught as easily)
    //if(trueStart == trueEnd)
    //    return {};

    if(text.empty())
        return {};
    else {
        skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;

        if(trueStart.fTextByteIndex == text.size()) {
            if(paragraph->getGlyphClusterAt(prev_grapheme(trueStart.fTextByteIndex), &glyphInfo)) {
                glyphID1 = paragraph->getGlyphPositionAtCoordinate(glyphInfo.fBounds.x(), glyphInfo.fBounds.centerY()).position + 1;
            }
        }
        else {
            if(paragraph->getGlyphClusterAt(trueStart.fTextByteIndex, &glyphInfo))
                glyphID1 = paragraph->getGlyphPositionAtCoordinate(glyphInfo.fBounds.x(), glyphInfo.fBounds.centerY()).position;
        }

        if(trueEnd.fTextByteIndex == text.size()) {
            if(paragraph->getGlyphClusterAt(prev_grapheme(trueEnd.fTextByteIndex), &glyphInfo))
                glyphID2 = paragraph->getGlyphPositionAtCoordinate(glyphInfo.fBounds.x(), glyphInfo.fBounds.centerY()).position + 1;
        }
        else {
            if(paragraph->getGlyphClusterAt(trueEnd.fTextByteIndex, &glyphInfo))
                glyphID2 = paragraph->getGlyphPositionAtCoordinate(glyphInfo.fBounds.x(), glyphInfo.fBounds.centerY()).position;
        }
    }

    if(glyphID1 == glyphID2)
        return {};

    return paragraph->getRectsForRange(glyphID1, glyphID2, skia::textlayout::RectHeightStyle::kMax, skia::textlayout::RectWidthStyle::kMax);
}

constexpr float SELECTION_RECT_EXTRA_AREA = 2.0f;

std::vector<SkRect> RichTextBox::rects_between_text_positions_2(TextPosition start, TextPosition end) {
    TextPosition trueStart = std::min(start, end);
    TextPosition trueEnd = std::max(start, end);

    std::vector<std::pair<int, SkRect>> lineRects;

    if(text.empty())
        return {};
    else {
        for(size_t i = trueStart.fTextByteIndex; i < trueEnd.fTextByteIndex; i++) {
            skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
            if(paragraph->getGlyphClusterAt(i, &glyphInfo)) {
                int lineNumber = paragraph->getLineNumberAt(i);
                if(lineNumber == -1)
                    break;
                skia::textlayout::LineMetrics lineMetrics;
                if(paragraph->getLineMetricsAt(lineNumber, &lineMetrics)) {
                    SkRect r = SkRect::MakeLTRB(glyphInfo.fBounds.left(), lineMetrics.fBaseline - lineMetrics.fAscent, glyphInfo.fBounds.right(), lineMetrics.fBaseline + lineMetrics.fDescent);
                    auto it = std::find_if(lineRects.begin(), lineRects.end(), [&](const auto& p) {
                        SkRect checkCollideWith = p.second;
                        checkCollideWith.fLeft = checkCollideWith.fLeft - SELECTION_RECT_EXTRA_AREA;
                        checkCollideWith.fRight = checkCollideWith.fRight + SELECTION_RECT_EXTRA_AREA;
                        return p.first == lineNumber && checkCollideWith.intersects(r);
                    });
                    if(it != lineRects.end())
                        it->second.join(r);
                    else
                        lineRects.emplace_back(lineNumber, r);
                }
            }
        }
    }

    std::vector<SkRect> toRet;
    for(auto& l : lineRects)
        toRet.emplace_back(l.second);

    return toRet;
}

void RichTextBox::rects_between_text_positions_func(TextPosition start, TextPosition end, std::function<void(const SkRect& r)> f) {
    TextPosition trueStart = std::min(start, end);
    TextPosition trueEnd = std::max(start, end);

    if(text.empty())
        return;
    else {
        for(size_t i = trueStart.fTextByteIndex; i < trueEnd.fTextByteIndex; i++) {
            skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
            if(paragraph->getGlyphClusterAt(i, &glyphInfo)) {
                int lineNumber = paragraph->getLineNumberAt(i);
                if(lineNumber == -1)
                    break;
                skia::textlayout::LineMetrics lineMetrics;
                if(paragraph->getLineMetricsAt(lineNumber, &lineMetrics)) {
                    SkRect r = SkRect::MakeLTRB(glyphInfo.fBounds.left() - SELECTION_RECT_EXTRA_AREA, lineMetrics.fBaseline - lineMetrics.fAscent - SELECTION_RECT_EXTRA_AREA, glyphInfo.fBounds.right() + SELECTION_RECT_EXTRA_AREA, lineMetrics.fBaseline + lineMetrics.fDescent + SELECTION_RECT_EXTRA_AREA);
                    f(r);
                }
            }
        }
    }
}


void RichTextBox::paint(SkCanvas* canvas, const PaintOpts& paintOpts) {
    rebuild();

    if(paintOpts.cursor.has_value()) {
        auto& cur = paintOpts.cursor.value();
        SkPaint p{SkColor4f{paintOpts.cursorColor.x(), paintOpts.cursorColor.y(), paintOpts.cursorColor.z(), 1.0f}};
        if(cur.selectionBeginPos != cur.selectionEndPos) {
            canvas->saveLayerAlphaf(nullptr, 0.6f);
            rects_between_text_positions_func(cur.selectionBeginPos, cur.selectionEndPos, [&](const SkRect& rect) {
                canvas->drawRect(rect, p);
            });
            canvas->restore();
        }

        if(cur.pos.fTextByteIndex <= text.size()) {
            canvas->drawRect(get_cursor_rect(cur.pos), p);
        }
    }

    paragraph->paint(canvas, 0.0f, 0.0f);
}
