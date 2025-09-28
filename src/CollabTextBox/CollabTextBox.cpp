// This file has been copied from the Skia library, and may be slightly modified. Please check the NOTICE file for more details

#include "CollabTextBox.hpp"
#include "CollabTextBoxShaper.hpp"
#include <include/core/SkCanvas.h>
#include <include/core/SkPath.h>
#include <src/base/SkUTF.h>

#include <algorithm>
#include <cfloat>
#include <cassert>
#include <utility>

using namespace CollabTextBox;

static inline SkRect offset(SkRect r, SkIPoint p) {
    return r.makeOffset((float)p.x(), (float)p.y());
}

static constexpr SkRect kUnsetRect{-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};

//static bool valid_utf8(const char* ptr, size_t size) { return SkUTF::CountUTF8(ptr, size) >= 0; }

// Kind of like Python's readlines(), but without any allocation.
// Calls f() on each line.
// F is [](const char*, size_t) -> void
template <typename F>
static void readlines(const void* data, size_t size, F f) {
    const char* start = (const char*)data;
    const char* end = start + size;
    const char* ptr = start;
    while (ptr < end) {
        while (*ptr++ != '\n' && ptr < end) {}
        size_t len = ptr - start;
        assert(len > 0);
        f(start, len);
        start = ptr;
    }
}

static std::string remove_newline(const char* str, size_t len) {
    return assert((str != nullptr) || (len == 0)), std::string(str, (len > 0 && str[len - 1] == '\n') ? len - 1 : len);
}

void Editor::markDirty(TextParagraph* line) {
    line->fBlob = nullptr;
    line->fShaped = false;
    line->fWordBoundaries = std::vector<bool>();
}

void Editor::setFont(SkFont font) {
    if (font != fFont) {
        fFont = std::move(font);
        fNeedsReshape = true;
        for (auto& l : fLines) { this->markDirty(&l); }
    }
}

void Editor::setFontMgr(sk_sp<SkFontMgr> fontMgr) {
    if(fontMgr != fFontMgr) {
        fFontMgr = fontMgr;
        fNeedsReshape = true;
        for (auto& l : fLines) { this->markDirty(&l); }
    }
}

void Editor::setWidth(int w) {
    if (fWidth != w) {
        fWidth = w;
        fNeedsReshape = true;
        for (auto& l : fLines) { this->markDirty(&l); }
    }
}
static SkPoint to_point(SkIPoint p) { return {(float)p.x(), (float)p.y()}; }

TextPosition Editor::getPosition(SkIPoint xy) {
    // Clamp input coordinates to stop function from hanging
    xy.fX = std::clamp(xy.fX, 0, fWidth - 1);
    xy.fY = std::clamp(xy.fY, 0, fHeight - 1);

    TextPosition approximatePosition{0, 0};
    this->reshapeAll();
    for (size_t j = 0; j < fLines.size(); ++j) {
        const TextParagraph& line = fLines[j];
        SkIRect lineRect = {0,
                            line.fOrigin.y(),
                            fWidth,
                            j + 1 < fLines.size() ? fLines[j + 1].fOrigin.y() : INT_MAX};
        if (const SkTextBlob* b = line.fBlob.get()) {
            SkIRect r = b->bounds().roundOut();
            r.offset(line.fOrigin);
            lineRect.join(r);
        }
        if (!lineRect.contains(xy.x(), xy.y())) {
            continue;
        }
        SkPoint pt = to_point(xy - line.fOrigin);
        const std::vector<SkRect>& pos = line.fCursorPos;
        for (size_t i = 0; i < pos.size(); ++i) {
            if (pos[i] != kUnsetRect && pos[i].contains(pt.x(), pt.y())) {
                return TextPosition{i, j};
            }
        }
        approximatePosition = {xy.x() <= line.fOrigin.x() ? 0 : line.fText.size(), j};
    }
    return approximatePosition;
}

bool CollabTextBox::is_utf8_continuation(char v) {
    return ((unsigned char)v & 0b11000000) ==
                               0b10000000;
}

const char* CollabTextBox::next_utf8(const char* p, const char* end) {
    if (p < end) {
        do {
            ++p;
        } while (p < end && is_utf8_continuation(*p));
    }
    return p;
}

const char* CollabTextBox::align_utf8(const char* p, const char* begin) {
    while (p > begin && is_utf8_continuation(*p)) {
        --p;
    }
    return p;
}

const char* CollabTextBox::prev_utf8(const char* p, const char* begin) {
    return p > begin ? align_utf8(p - 1, begin) : begin;
}

SkRect Editor::getLocation(TextPosition cursor) {
    this->reshapeAll();
    cursor = this->move(Movement::kNowhere, cursor);
    if (fLines.size() > 0) {
        const TextParagraph& cLine = fLines[cursor.fParagraphIndex];
        SkRect pos = {0, 0, 0, 0};
        if (cursor.fTextByteIndex < cLine.fCursorPos.size()) {
            pos = cLine.fCursorPos[cursor.fTextByteIndex];
        }
        pos.fRight = pos.fLeft + 1;
        pos.fLeft -= 1;
        return offset(pos, cLine.fOrigin);
    }
    return SkRect{0, 0, 0, 0};
}

static size_t count_char(const std::string& string, char value) {
    return std::ranges::count(string, value);
}

TextPosition Editor::insert(TextPosition pos, std::string_view utf8Text) {
    if (/*!valid_utf8(utf8Text, byteLen) ||*/ utf8Text.size() == 0) { // dont check for valid strings for now
        return pos;
    }
    pos = this->move(Movement::kNowhere, pos);
    fNeedsReshape = true;
    if (pos.fParagraphIndex < fLines.size()) {
        fLines[pos.fParagraphIndex].fText.insert(pos.fTextByteIndex, utf8Text.data(), utf8Text.size());
        this->markDirty(&fLines[pos.fParagraphIndex]);
    } else {
        assert(pos.fParagraphIndex == fLines.size());
        assert(pos.fTextByteIndex == 0);
        fLines.push_back(TextParagraph(utf8Text));
    }
    pos = TextPosition{pos.fTextByteIndex + utf8Text.size(), pos.fParagraphIndex};
    size_t newlinecount = count_char(fLines[pos.fParagraphIndex].fText, '\n');
    if (newlinecount > 0) {
        std::string src = fLines[pos.fParagraphIndex].fText;
        std::vector<TextParagraph>::const_iterator next = fLines.begin() + pos.fParagraphIndex + 1;
        fLines.insert(next, newlinecount, TextParagraph());
        TextParagraph* line = &fLines[pos.fParagraphIndex];
        readlines(src.c_str(), src.size(), [&line](const char* str, size_t l) {
            (line++)->fText = remove_newline(str, l);
        });
    }

    // Calculate the TextPosition post insert properly
    pos.fParagraphIndex = pos.fParagraphIndex + newlinecount;
    size_t lastNewline = utf8Text.find_last_of('\n');
    if(std::string_view::npos != lastNewline)
        pos.fTextByteIndex = utf8Text.size() - lastNewline - 1;
    if(utf8Text == "\n") { // Special case where the string is just a newline
        pos.fTextByteIndex = 0;
        pos.fParagraphIndex--;
    }

    return pos;
}

TextPosition Editor::remove(TextPosition pos1, TextPosition pos2) {
    pos1 = this->move(Movement::kNowhere, pos1);
    pos2 = this->move(Movement::kNowhere, pos2);
    auto cmp = [](const TextPosition& u, const TextPosition& v) { return u < v; };
    TextPosition start = std::min(pos1, pos2, cmp);
    TextPosition end = std::max(pos1, pos2, cmp);
    if (start == end || start.fParagraphIndex == fLines.size()) {
        return start;
    }
    fNeedsReshape = true;
    if (start.fParagraphIndex == end.fParagraphIndex) {
        assert(end.fTextByteIndex > start.fTextByteIndex);
        fLines[start.fParagraphIndex].fText.erase(start.fTextByteIndex, end.fTextByteIndex - start.fTextByteIndex);
        this->markDirty(&fLines[start.fParagraphIndex]);
    } else {
        assert(end.fParagraphIndex < fLines.size());
        auto& line = fLines[start.fParagraphIndex];
        line.fText.erase(start.fTextByteIndex, line.fText.size() - start.fTextByteIndex);
        line.fText.insert(start.fTextByteIndex, fLines[end.fParagraphIndex].fText.substr(end.fTextByteIndex));
        this->markDirty(&line);
        fLines.erase(fLines.begin() + start.fParagraphIndex + 1,
                     fLines.begin() + end.fParagraphIndex + 1);
    }
    return start;
}

std::string Editor::get_string() const {
    std::string toRet;
    for(const TextParagraph& p : fLines) {
        toRet += p.fText;
        toRet += '\n';
    }
    if(!toRet.empty())
        toRet.pop_back();
    return toRet;
}

std::string Editor::copy(TextPosition pos1, TextPosition pos2) const {
    pos1 = this->move(Movement::kNowhere, pos1);
    pos2 = this->move(Movement::kNowhere, pos2);
    auto cmp = [](const TextPosition& u, const TextPosition& v) { return u < v; };
    TextPosition start = std::min(pos1, pos2, cmp);
    TextPosition end = std::max(pos1, pos2, cmp);
    if(start == end || start.fParagraphIndex == fLines.size())
        return {};
    if(start.fParagraphIndex == end.fParagraphIndex) {
        assert(end.fTextByteIndex > start.fTextByteIndex);
        auto& str = fLines[start.fParagraphIndex].fText;
        return str.substr(start.fTextByteIndex, end.fTextByteIndex - start.fTextByteIndex);
    }
    assert(end.fParagraphIndex < fLines.size());
    const std::vector<TextParagraph>::const_iterator firstP = fLines.begin() + start.fParagraphIndex;
    const std::vector<TextParagraph>::const_iterator lastP  = fLines.begin() + end.fParagraphIndex;
    const auto& first = firstP->fText;
    const auto& last  = lastP->fText;

    std::string toRet;
    toRet += first.substr(start.fTextByteIndex);
    for (auto line = firstP + 1; line < lastP; ++line) {
        toRet += '\n';
        toRet += line->fText;
    }
    toRet += '\n';
    toRet += last.substr(0, end.fTextByteIndex);
    return toRet;
}

static inline const char* begin(const std::string& s) { return s.c_str(); }

static inline const char* end(const std::string& s) { return s.c_str() + s.size(); }

static size_t align_column(const std::string& str, size_t p) {
    if(p >= str.size())
        return str.size();
    return align_utf8(begin(str) + p, begin(str)) - begin(str);
}

// returns smallest i such that list[i] > value.  value > list[i-1]
// Use a binary search since list is monotonic
template <typename T>
static size_t find_first_larger(const std::vector<T>& list, T value) {
    return (size_t)(std::upper_bound(list.begin(), list.end(), value) - list.begin());
}

static size_t find_closest_x(const std::vector<SkRect>& bounds, float x, size_t b, size_t e) {
    if (b >= e) {
        return b;
    }
    assert(e <= bounds.size());
    size_t best_index = b;
    float best_diff = ::fabsf(bounds[best_index].x() - x);
    for (size_t i = b + 1; i < e; ++i) {
        float d = ::fabsf(bounds[i].x() - x);
        if (d < best_diff) {
            best_diff = d;
            best_index = i;
        }
    }
    return best_index;
}

TextPosition Editor::move(Movement move, TextPosition pos) const {
    if (fLines.empty()) {
        return {0, 0};
    }
    // First thing: fix possible bad input values.
    if (pos.fParagraphIndex >= fLines.size()) {
        pos.fParagraphIndex = fLines.size() - 1;
        pos.fTextByteIndex = fLines[pos.fParagraphIndex].fText.size();
    } else {
        pos.fTextByteIndex = align_column(fLines[pos.fParagraphIndex].fText, pos.fTextByteIndex);
    }

    assert(pos.fParagraphIndex < fLines.size());
    assert(pos.fTextByteIndex <= fLines[pos.fParagraphIndex].fText.size());

    assert(pos.fTextByteIndex == fLines[pos.fParagraphIndex].fText.size() ||
             !is_utf8_continuation(fLines[pos.fParagraphIndex].fText.begin()[pos.fTextByteIndex]));

    switch (move) {
        case Movement::kNowhere:
            break;
        case Movement::kLeft:
            if (0 == pos.fTextByteIndex) {
                if (pos.fParagraphIndex > 0) {
                    --pos.fParagraphIndex;
                    pos.fTextByteIndex = fLines[pos.fParagraphIndex].fText.size();
                }
            } else {
                const auto& str = fLines[pos.fParagraphIndex].fText;
                pos.fTextByteIndex =
                    prev_utf8(begin(str) + pos.fTextByteIndex, begin(str)) - begin(str);
            }
            break;
        case Movement::kRight:
            if (fLines[pos.fParagraphIndex].fText.size() == pos.fTextByteIndex) {
                if (pos.fParagraphIndex + 1 < fLines.size()) {
                    ++pos.fParagraphIndex;
                    pos.fTextByteIndex = 0;
                }
            } else {
                const auto& str = fLines[pos.fParagraphIndex].fText;
                pos.fTextByteIndex =
                    next_utf8(begin(str) + pos.fTextByteIndex, end(str)) - begin(str);
            }
            break;
        case Movement::kHome:
            {
                const std::vector<size_t>& list = fLines[pos.fParagraphIndex].fLineEndOffsets;
                size_t f = find_first_larger(list, pos.fTextByteIndex);
                pos.fTextByteIndex = f > 0 ? list[f - 1] : 0;
            }
            break;
        case Movement::kEnd:
            {
                const std::vector<size_t>& list = fLines[pos.fParagraphIndex].fLineEndOffsets;
                size_t f = find_first_larger(list, pos.fTextByteIndex);
                if (f < list.size()) {
                    pos.fTextByteIndex = list[f] > 0 ? list[f] - 1 : 0;
                } else {
                    pos.fTextByteIndex = fLines[pos.fParagraphIndex].fText.size();
                }
            }
            break;
        case Movement::kUp:
            {
                assert(pos.fTextByteIndex < fLines[pos.fParagraphIndex].fCursorPos.size());
                float x = fLines[pos.fParagraphIndex].fCursorPos[pos.fTextByteIndex].left();
                const std::vector<size_t>& list = fLines[pos.fParagraphIndex].fLineEndOffsets;
                size_t f = find_first_larger(list, pos.fTextByteIndex);
                // list[f] > value.  value > list[f-1]
                if (f > 0) {
                    // not the first line in paragraph.
                    pos.fTextByteIndex = find_closest_x(fLines[pos.fParagraphIndex].fCursorPos, x,
                                                        (f == 1) ? 0 : list[f - 2],
                                                        list[f - 1]);
                } else if (pos.fParagraphIndex > 0) {
                    --pos.fParagraphIndex;
                    const auto& newLine = fLines[pos.fParagraphIndex];
                    size_t r = newLine.fLineEndOffsets.size();
                    if (r > 0) {
                        pos.fTextByteIndex = find_closest_x(newLine.fCursorPos, x,
                                                            newLine.fLineEndOffsets[r - 1],
                                                            newLine.fCursorPos.size());
                    } else {
                        pos.fTextByteIndex = find_closest_x(newLine.fCursorPos, x, 0,
                                                            newLine.fCursorPos.size());
                    }
                }
                pos.fTextByteIndex =
                    align_column(fLines[pos.fParagraphIndex].fText, pos.fTextByteIndex);
            }
            break;
        case Movement::kDown:
            {
                const std::vector<size_t>& list = fLines[pos.fParagraphIndex].fLineEndOffsets;
                float x = fLines[pos.fParagraphIndex].fCursorPos[pos.fTextByteIndex].left();

                size_t f = find_first_larger(list, pos.fTextByteIndex);
                if (f < list.size()) {
                    const auto& bounds = fLines[pos.fParagraphIndex].fCursorPos;
                    pos.fTextByteIndex = find_closest_x(bounds, x, list[f],
                                                        f + 1 < list.size() ? list[f + 1]
                                                                            : bounds.size());
                } else if (pos.fParagraphIndex + 1 < fLines.size()) {
                    ++pos.fParagraphIndex;
                    const auto& bounds = fLines[pos.fParagraphIndex].fCursorPos;
                    const std::vector<size_t>& l2 = fLines[pos.fParagraphIndex].fLineEndOffsets;
                    pos.fTextByteIndex = find_closest_x(bounds, x, 0,
                                                        l2.size() > 0 ? l2[0] : bounds.size());
                } else {
                    pos.fTextByteIndex = fLines[pos.fParagraphIndex].fText.size();
                }
                pos.fTextByteIndex =
                    align_column(fLines[pos.fParagraphIndex].fText, pos.fTextByteIndex);
            }
            break;
        case Movement::kWordLeft:
            {
                for(;;) {
                    TextPosition prevCharPos = this->move(Movement::kLeft, pos);
                    if(prevCharPos != TextPosition{0, 0} && std::isspace(fLines[prevCharPos.fParagraphIndex].fText[prevCharPos.fTextByteIndex]))
                        pos = prevCharPos;
                    else
                        break;
                }

                if (pos.fTextByteIndex == 0) {
                    pos = this->move(Movement::kLeft, pos);
                    break;
                }

                const std::vector<bool>& words = fLines[pos.fParagraphIndex].fWordBoundaries;
                assert(words.size() == fLines[pos.fParagraphIndex].fText.size());
                do {
                    --pos.fTextByteIndex;
                } while (pos.fTextByteIndex > 0 && !words[pos.fTextByteIndex]);
            }
            break;
        case Movement::kWordRight:
            {
                const std::string& text = fLines[pos.fParagraphIndex].fText;
                if (pos.fTextByteIndex == text.size()) {
                    pos = this->move(Movement::kRight, pos);
                    break;
                }
                const std::vector<bool>& words = fLines[pos.fParagraphIndex].fWordBoundaries;
                assert(words.size() == text.size());
                do {
                    ++pos.fTextByteIndex;
                } while (pos.fTextByteIndex < text.size() && !words[pos.fTextByteIndex]);

                for(;;) {
                    const std::string& text = fLines[pos.fParagraphIndex].fText;
                    if(pos.fTextByteIndex != text.size() && std::isspace(text[pos.fTextByteIndex]))
                        pos = this->move(Movement::kRight, pos);
                    else
                        break;
                }
            }
            break;

    }
    return pos;
}

void Editor::paint(SkCanvas* c, PaintOpts options) {
    this->reshapeAll();
    if(!c)
        return;

    c->drawPaint(SkPaint(options.fBackgroundColor));

    if(options.showCursor) {
        auto& cur = options.cursor;
        SkPaint selection = SkPaint(SkColor4f{options.cursorColor[0], options.cursorColor[1], options.cursorColor[2], 0.3f});
        auto cmp = [](const TextPosition& u, const TextPosition& v) { return u < v; };
        for(TextPosition pos = std::min(cur.selectionBeginPos, cur.selectionEndPos, cmp), end = std::max(cur.selectionBeginPos, cur.selectionEndPos, cmp);
             pos < end;
             pos = this->move(Movement::kRight, pos))
        {
            assert(pos.fParagraphIndex < fLines.size());
            const TextParagraph& l = fLines[pos.fParagraphIndex];
            c->drawRect(offset(l.fCursorPos[pos.fTextByteIndex], l.fOrigin), selection);
        }

        if(fLines.size() > 0)
            c->drawRect(Editor::getLocation(cur.pos), SkPaint(options.cursorColor));
    }

    SkPaint foreground = SkPaint(options.fForegroundColor);
    for(const TextParagraph& line : fLines) {
        if(line.fBlob)
            c->drawTextBlob(line.fBlob.get(), line.fOrigin.x(), line.fOrigin.y(), foreground);
    }
}

void Editor::reshapeAll() {
    if(fNeedsReshape && fFontMgr) { // fFontMgr must not be null for text to be reshaped
        if(fLines.empty())
            fLines.push_back(TextParagraph());
        float shape_width = (float)(fWidth);
        for(TextParagraph& line : fLines) {
            if(!line.fShaped) {
                ShapeResult result = shape(line.fText.c_str(), line.fText.size(), fFont, fFontMgr, fLocale, shape_width);
                line.fBlob = std::move(result.blob);
                line.fLineEndOffsets = std::move(result.lineBreakOffsets);
                line.fCursorPos = std::move(result.glyphBounds);
                line.fWordBoundaries = std::move(result.wordBreaks);
                line.fHeight = result.verticalAdvance;
                line.fShaped = true;
            }
        }
        int y = 0;
        for (TextParagraph& line : fLines) {
            line.fOrigin = {0, y};
            y += line.fHeight;
        }
        fHeight = y;
        fNeedsReshape = false;
    }
}

