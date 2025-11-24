#include "TextBox.hpp"
#include "Helpers/Networking/ByteStream.hpp"
#include "TextStyleModifier.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <limits>
#include <modules/skparagraph/include/DartTypes.h>
#include <modules/skparagraph/include/Metrics.h>
#include <modules/skparagraph/include/TextStyle.h>
#include <modules/skparagraph/src/TextLine.h>
#include <modules/skunicode/include/SkUnicode.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/src/ParagraphImpl.h>
#include <Helpers/MathExtras.hpp>
#include <src/base/SkUTF.h>
#include <cereal/types/string.hpp>

namespace RichText {

std::string TextData::get_serialized() const {
    std::stringstream s;
    {
        cereal::PortableBinaryOutputArchive a(s);
        a(*this);
    }
    return s.str();
}

void TextData::save(cereal::PortableBinaryOutputArchive& a) const {
    a(paragraphs);
    a(static_cast<uint32_t>(tStyleMods.size()));
    for(auto& [pos, modsInPos] : tStyleMods) {
        a(pos, static_cast<uint16_t>(modsInPos.size()));
        for(auto& [modType, modifier] : modsInPos)
            a(modType, *modifier);
    }
}

void TextData::load(cereal::PortableBinaryInputArchive& a) {
    a(paragraphs);
    uint32_t tStyleModCount;
    a(tStyleModCount);
    tStyleMods.clear();
    for(uint32_t i = 0; i < tStyleModCount; i++) {
        auto& modsInPos = tStyleMods.emplace_back();
        uint16_t modsInPosSize;
        a(modsInPos.pos, modsInPosSize);
        for(uint16_t j = 0; j < modsInPosSize; j++) {
            TextStyleModifier::ModifierType modType;
            a(modType);
            auto modifier = TextStyleModifier::allocate_modifier(modType);
            a(*modifier);
            modsInPos.mods[modType] = modifier;
        }
    }
}

TextData TextData::deserialize_string(std::string& s) {
    if(s.empty()) {
        RichText::TextData textData;
        textData.paragraphs.emplace_back();
        return textData;
    }
    RichText::TextData textData;
    ByteMemStream ss(s.data(), s.length());
    {
        cereal::PortableBinaryInputArchive a(ss);
        a(textData);
    }
    return textData;
}

std::string TextData::get_plain_text() const {
    if(paragraphs.empty())
        return "";
    std::string toRet;
    for(auto& p : paragraphs) {
        toRet += p.text;
        toRet += '\n';
    }
    toRet.pop_back();
    return toRet;
}

bool TextPosition::operator<(const TextPosition& o) const {
    return (this->fParagraphIndex < o.fParagraphIndex) || (this->fParagraphIndex == o.fParagraphIndex && this->fTextByteIndex < o.fTextByteIndex);
}

bool TextPosition::operator>(const TextPosition& o) const {
    return !(*this < o) && !(*this == o);
}

bool TextPosition::operator>=(const TextPosition& o) const {
    return (*this > o) || (*this == o);
}

bool TextPosition::operator<=(const TextPosition& o) const {
    return (*this < o) || (*this == o);
}

bool TextBox::Cursor::operator==(const Cursor& o) const {
    return o.selectionBeginPos == selectionBeginPos && o.selectionEndPos == selectionEndPos && o.pos == pos;
}

bool TextBox::Cursor::operator!=(const Cursor& o) const {
    return !(*this == o);
}

TextBox::TextBox() {
    paragraphs.emplace_back();
}

void TextBox::process_key_input(Cursor& cur, InputKey in, bool ctrl, bool shift, const std::optional<TextStyleModifier::ModifierMap>& inputModMap) {
    bool moved = false;

    switch(in) {
        case InputKey::LEFT:
            cur.pos = cur.selectionBeginPos = move(ctrl ? Movement::LEFT_WORD : Movement::LEFT, cur.pos, nullptr, true);
            moved = true;
            break;
        case InputKey::RIGHT:
            cur.pos = cur.selectionBeginPos = move(ctrl ? Movement::RIGHT_WORD : Movement::RIGHT, cur.pos, nullptr, true);
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
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.pos, move(ctrl ? Movement::LEFT_WORD : Movement::LEFT, cur.pos));
            break;
        case InputKey::DEL:
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
            else
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.pos, move(ctrl ? Movement::RIGHT_WORD : Movement::RIGHT, cur.pos));
            break;
        case InputKey::ENTER:
            process_text_input(cur, "\n", inputModMap);
            break;
        case InputKey::TAB: 
            process_text_input(cur, "\t", inputModMap);
            break;
        case InputKey::SELECT_ALL:
            cur.selectionBeginPos = move(Movement::HOME, cur.pos);
            cur.pos = cur.selectionEndPos = move(Movement::END, cur.pos);
            break;
    }

    if(moved && !shift)
        cur.selectionEndPos = cur.selectionBeginPos;

    if(in != InputKey::UP && in != InputKey::DOWN)
        cur.previousX = std::nullopt;

    inputChangedTextBox = true;
}

void TextBox::process_mouse_left_button(Cursor& cur, const Vector2f& pos, bool clicked, bool held, bool shift) {
    if(clicked || held) {
        rebuild();

        Cursor oldCursor = cur;

        cur.pos = get_text_pos_closest_to_point(pos);
        cur.selectionBeginPos = cur.pos;
        if(clicked && !shift)
            cur.selectionEndPos = cur.selectionBeginPos;
        cur.previousX = std::nullopt;

        inputChangedTextBox |= (oldCursor != cur);
    }
}

std::pair<std::string, TextData> TextBox::process_copy(Cursor& cur) {
    return {get_text_between(cur.selectionBeginPos, cur.selectionEndPos), get_rich_text_data_between(cur.selectionBeginPos, cur.selectionEndPos)};
}

std::pair<std::string, TextData> TextBox::process_cut(Cursor& cur) {
    auto toRet = process_copy(cur);
    if(cur.selectionBeginPos != cur.selectionEndPos) {
        cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
        cur.previousX = std::nullopt;
        inputChangedTextBox = true;
    }
    return toRet;
}

void TextBox::process_text_input(Cursor& cur, const std::string& in, const std::optional<TextStyleModifier::ModifierMap>& inputModMap) {
    if(!in.empty()) {
        if(cur.selectionBeginPos != cur.selectionEndPos)
            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
        cur.selectionEndPos = cur.selectionBeginPos = cur.pos = insert(cur.pos, in, inputModMap);
        cur.previousX = std::nullopt;
        inputChangedTextBox = true;
    }
}

void TextBox::process_rich_text_input(Cursor& cur, const TextData& richText) {
    if(!richText.paragraphs.empty() && !(richText.paragraphs.size() == 1 && richText.paragraphs[0].text.empty())) {
        if(cur.selectionBeginPos != cur.selectionEndPos)
            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = remove(cur.selectionBeginPos, cur.selectionEndPos);
        cur.selectionEndPos = cur.selectionBeginPos = cur.pos = insert_rich_text(cur.pos, richText);
        cur.previousX = std::nullopt;
        inputChangedTextBox = true;
    }
}

size_t TextBox::count_grapheme(const std::string& text) {
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

size_t TextBox::next_grapheme(const std::string& text, size_t textBytePos) {
    if(textBytePos == text.size())
        return text.size();
    auto u = SkUnicodes::ICU::Make();
    auto breakIterator = u->makeBreakIterator(SkUnicode::BreakType::kGraphemes);
    breakIterator->setText(text.c_str() + textBytePos, text.size() - textBytePos);
    size_t count = breakIterator->next();
    return count + textBytePos;
}

size_t TextBox::prev_grapheme(const std::string& text, size_t textBytePos) {
    if(textBytePos == 0)
        return 0;
    auto u = SkUnicodes::ICU::Make();
    auto breakIterator = u->makeBreakIterator(SkUnicode::BreakType::kGraphemes);
    breakIterator->setText(text.c_str(), text.size());
    size_t toRet = 0;
    for(;;) {
        size_t potentialNext = breakIterator->next();
        if(potentialNext >= textBytePos)
            return toRet;
        toRet = potentialNext;
    }
}

TextPosition TextBox::get_text_pos_from_byte_pos(const std::string& text, size_t textBytePos) {
    TextPosition toRet{0, 0};
    for(size_t i = 0; i < textBytePos; i++) {
        char c = text[i];
        if(c == '\n') {
            toRet.fTextByteIndex = 0;
            toRet.fParagraphIndex++;
        }
        else
            toRet.fTextByteIndex++;
    }
    return toRet;
}

size_t TextBox::get_byte_pos_from_text_pos(TextPosition textPos) {
    size_t toRet = 0;
    for(size_t pIndex = 0; pIndex < textPos.fParagraphIndex; pIndex++)
        toRet += paragraphs[pIndex].text.size() + 1; // Add 1 byte for newline
    toRet += textPos.fTextByteIndex;
    return toRet;
}

TextPosition TextBox::get_text_pos_closest_to_point(Vector2f point) {
    rebuild();

    skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;

    size_t pIndex = 0;

    for(; pIndex < (paragraphs.size() - 1) && point.y() > paragraphs[pIndex].p->getHeight(); pIndex++)
        point.y() -= paragraphs[pIndex].p->getHeight();

    ParagraphData& pData = paragraphs[pIndex];

    bool foundGlyph = pData.p->getClosestGlyphClusterAt(point.x(), point.y(), &glyphInfo);
    if(foundGlyph) {
        TextPosition toRet = {pIndex, 0};
        toRet.fTextByteIndex = glyphInfo.fClusterTextRange.start;

        if(glyphInfo.fGlyphClusterPosition == skia::textlayout::TextDirection::kLtr) {
            if(std::abs(glyphInfo.fBounds.right() - point.x()) < std::abs(glyphInfo.fBounds.left() - point.x()))
                toRet.fTextByteIndex = next_grapheme(pData.text, toRet.fTextByteIndex);
        }
        else {
            if(std::abs(glyphInfo.fBounds.left() - point.x()) < std::abs(glyphInfo.fBounds.right() - point.x()))
                toRet.fTextByteIndex = next_grapheme(pData.text, toRet.fTextByteIndex);
        }

        // Space at the end of a line will bring the cursor to the next line, check if the cursor moved to the next line
        if(toRet.fTextByteIndex != pData.text.size()) {
            skia::textlayout::LineMetrics lineMetrics;
            int lineNumber = pData.p->getLineNumberAt(toRet.fTextByteIndex);
            if(lineNumber > 0) {
                bool lineMetricsFound = pData.p->getLineMetricsAt(lineNumber, &lineMetrics);
                if(lineMetricsFound) {
                    float top = lineMetrics.fBaseline - lineMetrics.fAscent;
                    if(point.y() < top)
                        toRet.fTextByteIndex = prev_grapheme(pData.text, toRet.fTextByteIndex);
                }
            }
        }

        return toRet;
    }
    else
        return TextPosition{pIndex, 0};
}

TextData TextBox::get_rich_text_data() {
    TextData toRet;
    for(auto& p : paragraphs) {
        toRet.paragraphs.emplace_back();
        toRet.paragraphs.back().text = p.text;
        toRet.paragraphs.back().pStyleData = p.pStyleData;
    }
    toRet.tStyleMods = tStyleMods;
    return toRet;
}

TextData TextBox::get_rich_text_data_between(TextPosition p1, TextPosition p2) {
    auto [start, end] = get_start_end_text_pos(p1, p2);

    TextData toRet;

    if(start == end) {
        toRet.paragraphs.emplace_back();
        return toRet;
    }

    for(size_t pIndex = start.fParagraphIndex; pIndex <= end.fParagraphIndex; pIndex++) {
        size_t textIndexStart = pIndex == start.fParagraphIndex ? start.fTextByteIndex : 0;
        size_t textIndexEnd = pIndex == end.fParagraphIndex ? end.fTextByteIndex : paragraphs[pIndex].text.size();
        toRet.paragraphs.emplace_back();
        toRet.paragraphs.back().text = paragraphs[pIndex].text.substr(textIndexStart, textIndexEnd - textIndexStart);
        toRet.paragraphs.back().pStyleData = paragraphs[pIndex].pStyleData;
    }

    toRet.tStyleMods.emplace_back(TextPosition{0, 0}, get_mods_used_at_pos(start));

    for(auto& [modPos, modifiers] : tStyleMods) {
        if(modPos <= start)
            continue;
        else if(modPos > end)
            break;
        TextPosition insertPos;
        insertPos.fParagraphIndex = modPos.fParagraphIndex - start.fParagraphIndex;
        if(modPos.fParagraphIndex == start.fParagraphIndex)
            insertPos.fTextByteIndex = modPos.fTextByteIndex - start.fTextByteIndex;
        else
            insertPos.fTextByteIndex = modPos.fTextByteIndex;
        toRet.tStyleMods.emplace_back(insertPos, modifiers);
    }

    return toRet;
}

TextPosition TextBox::insert_rich_text(TextPosition p, const TextData& richText) {
    p = move(Movement::NOWHERE, p);

    if(richText.paragraphs.empty())
        return p;

    std::string strToInsert;
    for(auto& p : richText.paragraphs) {
        strToInsert += p.text;
        strToInsert += '\n';
    }
    strToInsert.pop_back();
    TextPosition toRet = insert(p, strToInsert);

    TextPosition lastPos;
    lastPos.fParagraphIndex = p.fParagraphIndex + richText.paragraphs.size() - 1;
    if(richText.paragraphs.size() == 1)
        lastPos.fTextByteIndex = p.fTextByteIndex + richText.paragraphs[0].text.size();
    else
        lastPos.fTextByteIndex = richText.paragraphs.back().text.size();

    for(auto& [modPos, modMap] : richText.tStyleMods) {
        TextPosition shiftedModPos;
        shiftedModPos.fParagraphIndex = modPos.fParagraphIndex + p.fParagraphIndex;
        if(shiftedModPos.fParagraphIndex == p.fParagraphIndex)
            shiftedModPos.fTextByteIndex = p.fTextByteIndex + modPos.fTextByteIndex;
        else
            shiftedModPos.fTextByteIndex = modPos.fTextByteIndex;
        for(auto& [modType, modifier] : modMap)
            set_text_style_modifier_between(shiftedModPos, lastPos, modifier);
    }

    for(size_t pIndex = p.fParagraphIndex; pIndex <= lastPos.fParagraphIndex; pIndex++) {
        // If the first line was empty before pasting (it's not empty right now, which is why the comparison is required), we will apply the paragraph style to the first line as well
        if((pIndex == p.fParagraphIndex && paragraphs[pIndex].text.size() == richText.paragraphs[0].text.size()) || pIndex != p.fParagraphIndex)
            paragraphs[pIndex].pStyleData = richText.paragraphs[pIndex - p.fParagraphIndex].pStyleData;
    }

    inputChangedTextBox = true;
    needsRebuild = true;

    return toRet;
}

void TextBox::clear_text() {
    paragraphs.clear();
    tStyleMods.clear();
    paragraphs.emplace_back();
}

void TextBox::set_string(const std::string& str) {
    clear_text();
    insert({0, 0}, str);
}

void TextBox::set_rich_text_data(const TextData& richText) {
    paragraphs.clear();
    for(auto& p : richText.paragraphs) {
        paragraphs.emplace_back();
        paragraphs.back().text = p.text;
        paragraphs.back().pStyleData = p.pStyleData;
    }
    if(richText.paragraphs.empty())
        paragraphs.emplace_back();
    tStyleMods = richText.tStyleMods;
    inputChangedTextBox = true;
    needsRebuild = true;
}

std::string TextBox::get_string() {
    return get_text_between({0, 0}, move(TextBox::Movement::END, {0, 0}));
}

std::string TextBox::get_text_between(TextPosition p1, TextPosition p2) {
    auto [start, end] = get_start_end_text_pos(p1, p2);

    if(start == end)
        return "";

    std::string toRet;

    for(size_t pIndex = start.fParagraphIndex; pIndex <= end.fParagraphIndex; pIndex++) {
        size_t textIndexStart = pIndex == start.fParagraphIndex ? start.fTextByteIndex : 0;
        size_t textIndexEnd = pIndex == end.fParagraphIndex ? end.fTextByteIndex : paragraphs[pIndex].text.size();
        toRet += paragraphs[pIndex].text.substr(textIndexStart, textIndexEnd - textIndexStart);
        if(pIndex != end.fParagraphIndex)
            toRet += '\n';
    }

    return toRet;
}


void TextBox::set_initial_text_style(const skia::textlayout::TextStyle& tStyle) {
    if(!tStyle.equals(initialTStyle)) {
        initialTStyle = tStyle;
        inputChangedTextBox = true;
        needsRebuild = true;
    }
}

void TextBox::set_initial_text_style_modifier(const std::shared_ptr<TextStyleModifier>& modifier) {
    if(tStyleMods.empty() || tStyleMods[0].pos != TextPosition{0, 0}) {
        if(tStyleMods.empty())
            tStyleMods.emplace_back();
        else
            tStyleMods.insert(tStyleMods.begin(), {});
        tStyleMods[0].pos = {0, 0};
    }
    tStyleMods[0].mods[modifier->get_type()] = modifier;
    remove_duplicate_text_style_mods();
}

TextStyleModifier::ModifierMap TextBox::get_mods_used_at_pos(TextPosition p) {
    p = move(Movement::NOWHERE, p);

    TextStyleModifier::ModifierMap toRet = TextStyleModifier::get_default_modifiers();

    for(auto& [pos, tStyleModInPos] : tStyleMods) {
        if(pos <= p) {
            for(auto& [modType, tStyle] : tStyleModInPos)
                toRet[modType] = tStyle;
        }
        else
            break;
    }

    return toRet;
}

std::shared_ptr<TextStyleModifier> TextBox::get_last_text_style_mod_before_pos(TextPosition pos, TextStyleModifier::ModifierType modType) {
    std::shared_ptr<TextStyleModifier> lastModOfThisTypeBeforeEnd = TextStyleModifier::get_default_modifier(modType);
    for(auto& [tPos, tStyleModsInPos] : tStyleMods) {
        if(tPos <= pos) {
            auto containedOfSameType = tStyleModsInPos.find(modType);
            if(containedOfSameType != tStyleModsInPos.end())
                lastModOfThisTypeBeforeEnd = containedOfSameType->second;
        }
        else
            break;
    }
    return lastModOfThisTypeBeforeEnd;
}

void TextBox::erase_if_over_all_styles_until_pos(TextPosition pos, const std::function<bool(TextPosition, const std::shared_ptr<TextStyleModifier>&)>& func) {
    for(auto& [tPos, tStyleModsInPos] : tStyleMods) {
        if(tPos <= pos) {
            std::erase_if(tStyleModsInPos, [&](auto& item) {
                return func(tPos, item.second);
            });
        }
        else
            break;
    }
}

void TextBox::set_text_style_modifier_between(TextPosition p1, TextPosition p2, const std::shared_ptr<TextStyleModifier>& modifier) {
    auto [start, end] = get_start_end_text_pos(p1, p2);
    if(start != end) {
        auto lastModOfThisTypeBeforeEnd = get_last_text_style_mod_before_pos(end, modifier->get_type());
        assert(lastModOfThisTypeBeforeEnd != nullptr);
        erase_if_over_all_styles_until_pos(end, [&](TextPosition p, const std::shared_ptr<TextStyleModifier>& modToCheck) {
            return p >= start && modifier->get_type() == modToCheck->get_type();
        });
        insert_style_at_pos(start, modifier);
        insert_style_at_pos(end, lastModOfThisTypeBeforeEnd); // Even if the end is the literal end of the text (which wont be seen), the end style must still be placed at the end so that it can merge with and delete the start style when the styled text is erased
        remove_duplicate_text_style_mods();
        inputChangedTextBox = true;
        needsRebuild = true;
    }
}

void TextBox::set_text_alignment_between(size_t paragraphIndex1, size_t paragraphIndex2, skia::textlayout::TextAlign newAlignment) {
    auto [start, end] = get_start_end_paragraph_pos(paragraphIndex1, paragraphIndex2);
    bool anythingChanged = false;
    for(size_t i = start; i <= end; i++) {
        if(paragraphs[i].pStyleData.textAlignment != newAlignment) {
            anythingChanged = true;
            paragraphs[i].pStyleData.textAlignment = newAlignment;
        }
    }
    if(anythingChanged) {
        inputChangedTextBox = true;
        needsRebuild = true;
    }
}

void TextBox::set_text_direction_between(size_t paragraphIndex1, size_t paragraphIndex2, skia::textlayout::TextDirection newDirection) {
    auto [start, end] = get_start_end_paragraph_pos(paragraphIndex1, paragraphIndex2);
    bool anythingChanged = false;
    for(size_t i = start; i <= end; i++) {
        if(paragraphs[i].pStyleData.textDirection != newDirection) {
            anythingChanged = true;
            paragraphs[i].pStyleData.textDirection = newDirection;
        }
    }
    if(anythingChanged) {
        inputChangedTextBox = true;
        needsRebuild = true;
    }
}

ParagraphStyleData TextBox::get_paragraph_style_data_at(size_t paragraphIndex) {
    return paragraphs[std::min(paragraphIndex, paragraphs.size() - 1)].pStyleData;
}

void TextBox::insert_style_at_pos(TextPosition pos, const std::shared_ptr<TextStyleModifier>& modifier) {
    size_t indexToPlaceAt = 0;
    bool finishedInsertingStyle = false;
    for(auto& [tPos, tStyleModsInPos] : tStyleMods) {
        if(tPos > pos)
            break;
        else if(tPos == pos) {
            tStyleModsInPos[modifier->get_type()] = modifier;
            finishedInsertingStyle = true;
            break;
        }
        else
            indexToPlaceAt++;
    }
    if(!finishedInsertingStyle) {
        auto it = tStyleMods.insert(tStyleMods.begin() + indexToPlaceAt, PositionedTextStyleMod{});
        it->pos = pos;
        it->mods[modifier->get_type()] = modifier;
    }
}

TextPosition TextBox::move(Movement movement, TextPosition pos, std::optional<float>* previousX, bool flipDependingOnTextDirection) {
    if(pos.fParagraphIndex >= paragraphs.size()) {
        pos.fParagraphIndex = paragraphs.size() - 1;
        pos.fTextByteIndex = paragraphs[pos.fParagraphIndex].text.size();
    }

    if(pos.fTextByteIndex >= paragraphs[pos.fParagraphIndex].text.size())
        pos.fTextByteIndex = paragraphs[pos.fParagraphIndex].text.size();

    if(movement != Movement::NOWHERE && movement != Movement::HOME && movement != Movement::END) {
        rebuild();
        if(flipDependingOnTextDirection && paragraphs[pos.fParagraphIndex].pStyleData.textDirection == skia::textlayout::TextDirection::kRtl) {
            if(movement == Movement::LEFT)
                movement = Movement::RIGHT;
            else if(movement == Movement::RIGHT)
                movement = Movement::LEFT;
            else if(movement == Movement::LEFT_WORD)
                movement = Movement::RIGHT_WORD;
            else if(movement == Movement::RIGHT_WORD)
                movement = Movement::LEFT_WORD;
        }
    }

    switch (movement) {
        case Movement::NOWHERE:
            break;
        case Movement::LEFT: {
            if(pos.fTextByteIndex == 0) {
                if(pos.fParagraphIndex != 0) {
                    pos.fParagraphIndex--;
                    pos.fTextByteIndex = paragraphs[pos.fParagraphIndex].text.size();
                }
            }
            else
                pos.fTextByteIndex = prev_grapheme(paragraphs[pos.fParagraphIndex].text, pos.fTextByteIndex);
            break;
        }
        case Movement::RIGHT: {
            if(pos.fTextByteIndex == paragraphs[pos.fParagraphIndex].text.size()) {
                if(pos.fParagraphIndex != paragraphs.size() - 1) {
                    pos.fParagraphIndex++;
                    pos.fTextByteIndex = 0;
                }
            }
            else
                pos.fTextByteIndex = next_grapheme(paragraphs[pos.fParagraphIndex].text, pos.fTextByteIndex);
            break;
        }
        case Movement::UP: {
            assert(previousX != nullptr);

            int lineNumber = paragraphs[pos.fParagraphIndex].p->getLineNumberAt(pos.fTextByteIndex == paragraphs[pos.fParagraphIndex].text.size() ? prev_grapheme(paragraphs[pos.fParagraphIndex].text, pos.fTextByteIndex) : pos.fTextByteIndex);
            if(lineNumber <= 0 && pos.fParagraphIndex == 0) {
                pos.fTextByteIndex = 0;
                if(previousX != nullptr)
                    *previousX = get_cursor_rect(pos).centerX();
            }
            else {
                if(lineNumber <= 0 && paragraphs[pos.fParagraphIndex - 1].text.empty()) {
                    if(previousX != nullptr && !previousX->has_value())
                        *previousX = get_cursor_rect(pos).centerX();
                    pos = TextPosition{pos.fParagraphIndex - 1, 0};
                }
                else {
                    skia::textlayout::LineMetrics prevLineMetrics;
                    SkRect cursorRect = get_cursor_rect(pos);
                    if(lineNumber <= 0) {
                        pos.fParagraphIndex--;
                        lineNumber = paragraphs[pos.fParagraphIndex].p->lineNumber();
                    }
                    paragraphs[pos.fParagraphIndex].p->getLineMetricsAt(lineNumber - 1, &prevLineMetrics);
                    Vector2f pointToCheck = {(previousX != nullptr && previousX->has_value()) ? previousX->value() : cursorRect.centerX(), prevLineMetrics.fBaseline + paragraphs[pos.fParagraphIndex].heightOffset};
                    pos = get_text_pos_closest_to_point(pointToCheck);

                    if(previousX != nullptr && !previousX->has_value())
                        *previousX = pointToCheck.x();
                }
            }
            break;
        }
        case Movement::DOWN: {
            assert(previousX != nullptr);

            int lineNumber = paragraphs[pos.fParagraphIndex].p->getLineNumberAt(pos.fTextByteIndex);
            if((lineNumber == -1 || lineNumber == static_cast<int>(paragraphs[pos.fParagraphIndex].p->lineNumber() - 1)) && pos.fParagraphIndex == (paragraphs.size() - 1)) {
                pos.fTextByteIndex = paragraphs[pos.fParagraphIndex].text.size();
                if(previousX != nullptr)
                    *previousX = get_cursor_rect(pos).centerX();
            }
            else {
                if((lineNumber == -1 || lineNumber == static_cast<int>(paragraphs[pos.fParagraphIndex].p->lineNumber() - 1)) && paragraphs[pos.fParagraphIndex + 1].text.empty()) {
                    if(previousX != nullptr && !previousX->has_value())
                        *previousX = get_cursor_rect(pos).centerX();
                    pos = TextPosition{pos.fParagraphIndex + 1, 0};
                }
                else {
                    skia::textlayout::LineMetrics nextLineMetrics;
                    SkRect cursorRect = get_cursor_rect(pos);
                    if(lineNumber == -1 || lineNumber == static_cast<int>(paragraphs[pos.fParagraphIndex].p->lineNumber() - 1)) {
                        pos.fParagraphIndex++;
                        lineNumber = -1;
                    }
                    paragraphs[pos.fParagraphIndex].p->getLineMetricsAt(lineNumber + 1, &nextLineMetrics);
                    Vector2f pointToCheck = {(previousX != nullptr && previousX->has_value()) ? previousX->value() : cursorRect.centerX(), nextLineMetrics.fBaseline + paragraphs[pos.fParagraphIndex].heightOffset};
                    pos = get_text_pos_closest_to_point(pointToCheck);

                    if(previousX != nullptr && !previousX->has_value())
                        *previousX = pointToCheck.x();
                }
            }
            break;
        }
        case Movement::LEFT_WORD: {
            if(pos == TextPosition{0, 0})
                break;

            std::string fullText = get_string();
            size_t bytePosInFullText = get_byte_pos_from_text_pos(pos);

            auto u = SkUnicodes::ICU::Make();

            size_t byteIndexToRet = 0;

            auto breakIterator = u->makeBreakIterator(SkUnicode::BreakType::kWords);
            breakIterator->setText(fullText.c_str(), fullText.size());

            while(!breakIterator->isDone()) {
                size_t nextByteIndex = breakIterator->next();
                if(nextByteIndex >= bytePosInFullText)
                    return get_text_pos_from_byte_pos(fullText, byteIndexToRet);

                const char* tPtr = fullText.c_str() + nextByteIndex;
                SkUnichar u = SkUTF::NextUTF8(&tPtr, fullText.c_str() + fullText.size());
                if(!SkUnicodes::ICU::Make()->isWhitespace(u))
                    byteIndexToRet = nextByteIndex;
            }
            break;
        }
        case Movement::RIGHT_WORD: {
            TextPosition endPos = move(Movement::END, {0, 0});
            if(pos == endPos)
                break;

            std::string fullText = get_string();
            size_t bytePosInFullText = get_byte_pos_from_text_pos(pos);

            auto u = SkUnicodes::ICU::Make();
            auto breakIterator = u->makeBreakIterator(SkUnicode::BreakType::kWords);

            breakIterator->setText(fullText.c_str() + bytePosInFullText, fullText.size() - bytePosInFullText);

            while(!breakIterator->isDone()) {
                size_t p = breakIterator->next() + bytePosInFullText;
                if(p == fullText.size())
                    return get_text_pos_from_byte_pos(fullText, p);
                const char* tPtr = fullText.c_str() + p;
                SkUnichar u = SkUTF::NextUTF8(&tPtr, fullText.c_str() + fullText.size());
                if(!SkUnicodes::ICU::Make()->isWhitespace(u))
                    return get_text_pos_from_byte_pos(fullText, p);
            }

            break;
        }
        case Movement::HOME:
            pos.fTextByteIndex = 0;
            pos.fParagraphIndex = 0;
            break;
        case Movement::END:
            pos.fTextByteIndex = paragraphs.back().text.size();
            pos.fParagraphIndex = paragraphs.size() - 1;
            break;
    }
    return pos;

}

void TextBox::set_width(float newWidth) {
    if(width != newWidth) {
        width = std::max(newWidth, 4.0f);
        inputChangedTextBox = true;
        needsRebuild = true;
    }
}

float TextBox::get_height() {
    rebuild();
    return paragraphs.back().heightOffset + paragraphs.back().p->getHeight();
}

void TextBox::set_allow_newlines(bool allow) {
    newlinesAllowed = allow;
}

void TextBox::set_font_data(const std::shared_ptr<FontData>& fD) {
    if(fontData != fD) {
        fontData = fD;
        inputChangedTextBox = true;
        needsRebuild = true;
    }
}

void TextBox::rebuild() {
    if(needsRebuild) {
        float heightOffset = 0.0f;

        auto nextTStyleModIt = tStyleMods.begin();

        skia::textlayout::TextStyle tStyle = initialTStyle;

        FontFamiliesTextStyleModifier::globalFontData = fontData;

        for(size_t pIndex = 0; pIndex < paragraphs.size(); pIndex++) {
            ParagraphData& pData = paragraphs[pIndex];
            skia::textlayout::ParagraphStyle pStyle = pData.pStyleData.get_paragraph_style();
            pStyle.setTextStyle(tStyle);

            if(!newlinesAllowed) {
                pStyle.setTextHeightBehavior(skia::textlayout::kDisableAll);

                skia::textlayout::StrutStyle strutStyle;
                strutStyle.setStrutEnabled(true);
                strutStyle.setForceStrutHeight(true);
                pStyle.setStrutStyle(strutStyle);
            }

            size_t tIndex = 0;
            skia::textlayout::ParagraphBuilderImpl a(pStyle, fontData->collection, SkUnicodes::ICU::Make());

            for(;;) {
                if(nextTStyleModIt == tStyleMods.end() || nextTStyleModIt->pos.fParagraphIndex != pIndex) {
                    a.pushStyle(tStyle);
                    a.addText(pData.text.c_str() + tIndex, pData.text.length() - tIndex);
                    a.pop();
                    break;
                }
                else {
                    auto& nextTStyleMod = *nextTStyleModIt;

                    // Print with previous style

                    a.pushStyle(tStyle);
                    a.addText(pData.text.c_str() + tIndex, nextTStyleMod.pos.fTextByteIndex - tIndex);
                    a.pop();
                    tIndex = nextTStyleMod.pos.fTextByteIndex;

                    // Modify tStyle with new data
                    for(auto& [modType, modifier] : nextTStyleMod.mods)
                        modifier->modify_text_style(tStyle);

                    ++nextTStyleModIt;
                }
            }

            pData.p = a.Build();
            pData.p->layout(width);
            pData.heightOffset = heightOffset;
            heightOffset += pData.p->getHeight();
        }

        //if(tStyleMods.size() != 0) {
        //    std::cout << "-------" << std::endl;
        //    for(auto& s : tStyleMods)
        //        std::cout << s.pos.fParagraphIndex << " " << s.pos.fTextByteIndex << std::endl;
        //    std::cout << "size: " << tStyleMods.size() << std::endl;
        //}

        needsRebuild = false;
    }
}

void TextBox::remove_duplicate_text_style_mods() {
    TextStyleModifier::ModifierMap previouslyAppliedMods = TextStyleModifier::get_default_modifiers();

    std::erase_if(tStyleMods, [&previouslyAppliedMods](PositionedTextStyleMod& tStyleModsInPos) {
        std::erase_if(tStyleModsInPos.mods, [&previouslyAppliedMods](const std::pair<TextStyleModifier::ModifierType, std::shared_ptr<TextStyleModifier>>& tStyleModPair) {
            auto [placedIt, insertionTookPlace] = previouslyAppliedMods.emplace(tStyleModPair.first, tStyleModPair.second);
            if(!insertionTookPlace) {
                if(placedIt->second->equivalent(*tStyleModPair.second))
                    return true;
                placedIt->second = tStyleModPair.second;
            }
            return false;
        });
        return tStyleModsInPos.mods.empty();
    });
}

void TextBox::set_tab_space_width(unsigned newTabWidth) {
    tabWidth = newTabWidth;
}

TextPosition TextBox::insert(TextPosition pos, std::string_view textToInsert, const std::optional<TextStyleModifier::ModifierMap>& inputModMap) {
    TextPosition oldPos = pos = move(Movement::NOWHERE, pos);

    for(char c : textToInsert) {
        if(c == '\n') {
            if(newlinesAllowed) {
                for(auto& [p, tStylesInPos] : tStyleMods) {
                    if(p.fParagraphIndex > pos.fParagraphIndex)
                        p.fParagraphIndex++;
                    else if(p.fParagraphIndex == pos.fParagraphIndex && p.fTextByteIndex >= pos.fTextByteIndex && !(p.fTextByteIndex == pos.fTextByteIndex && p.fTextByteIndex == 0)) {
                        p.fTextByteIndex = p.fTextByteIndex - pos.fTextByteIndex;
                        p.fParagraphIndex++;
                    }
                }
                paragraphs.insert(paragraphs.begin() + pos.fParagraphIndex + 1, ParagraphData{});
                paragraphs[pos.fParagraphIndex + 1].pStyleData = paragraphs[pos.fParagraphIndex].pStyleData;
                paragraphs[pos.fParagraphIndex + 1].text = paragraphs[pos.fParagraphIndex].text.substr(pos.fTextByteIndex, paragraphs[pos.fParagraphIndex].text.size() - pos.fTextByteIndex);
                paragraphs[pos.fParagraphIndex].text.erase(pos.fTextByteIndex, paragraphs[pos.fParagraphIndex].text.size() - pos.fTextByteIndex);
                pos.fParagraphIndex++;
                pos.fTextByteIndex = 0;
            }
        }
        else if(c == '\t') {
            for(auto& [p, tStylesInPos] : tStyleMods) {
                if(p.fParagraphIndex == pos.fParagraphIndex && p.fTextByteIndex >= pos.fTextByteIndex && !(p.fTextByteIndex == pos.fTextByteIndex && p.fTextByteIndex == 0))
                    p.fTextByteIndex += tabWidth;
            }
            paragraphs[pos.fParagraphIndex].text.insert(pos.fTextByteIndex, std::string(tabWidth, ' '));
            pos.fTextByteIndex += tabWidth;
        }
        else {
            for(auto& [p, tStylesInPos] : tStyleMods) {
                if(p.fParagraphIndex == pos.fParagraphIndex && p.fTextByteIndex >= pos.fTextByteIndex && !(p.fTextByteIndex == pos.fTextByteIndex && p.fTextByteIndex == 0))
                    p.fTextByteIndex++;
            }
            paragraphs[pos.fParagraphIndex].text.insert(paragraphs[pos.fParagraphIndex].text.begin() + pos.fTextByteIndex, c);
            pos.fTextByteIndex++;
        }
    }

    if(!textToInsert.empty()) {
        remove_duplicate_text_style_mods();
        if(inputModMap.has_value()) {
            for(const auto& [modType, modifier] : inputModMap.value())
                set_text_style_modifier_between(oldPos, pos, modifier);
        }
        inputChangedTextBox = true;
        needsRebuild = true;
    }

    return pos;
}

TextPosition TextBox::remove(TextPosition p1, TextPosition p2) {
    auto [start, end] = get_start_end_text_pos(p1, p2);

    if(start == end || start.fParagraphIndex == paragraphs.size())
        return start;
    if(start.fParagraphIndex == end.fParagraphIndex) {
        assert(end.fTextByteIndex > start.fTextByteIndex);
        paragraphs[start.fParagraphIndex].text.erase(start.fTextByteIndex, end.fTextByteIndex - start.fTextByteIndex);
    }
    else {
        assert(end.fParagraphIndex < paragraphs.size());
        auto& paragraph = paragraphs[start.fParagraphIndex];
        paragraph.text.erase(start.fTextByteIndex, paragraph.text.size() - start.fTextByteIndex);
        paragraph.text.insert(start.fTextByteIndex, paragraphs[end.fParagraphIndex].text.substr(end.fTextByteIndex));
        paragraphs.erase(paragraphs.begin() + start.fParagraphIndex + 1, paragraphs.begin() + end.fParagraphIndex + 1);
    }

    for(int32_t i = 0; i < static_cast<int32_t>(tStyleMods.size()); i++) {
        PositionedTextStyleMod& tStyleMod = tStyleMods[i];
        auto& p = tStyleMod.pos;
        if(p > start) {
            if(end > p)
                p = start;
            else if(start.fParagraphIndex == p.fParagraphIndex)
                p.fTextByteIndex -= (end.fTextByteIndex - start.fTextByteIndex);
            else if(end.fParagraphIndex == p.fParagraphIndex) {
                p.fParagraphIndex = start.fParagraphIndex;
                p.fTextByteIndex = start.fTextByteIndex + (p.fTextByteIndex - end.fTextByteIndex);
            }
            else
                p.fParagraphIndex -= (end.fParagraphIndex - start.fParagraphIndex);
        }
        if(i > 0 && tStyleMods[i - 1].pos == p) {
            auto& tStyleModsToMergeWith = tStyleMods[i - 1].mods;
            for(auto& [modType, tStyle] : tStyleMod.mods)
                tStyleModsToMergeWith[modType] = tStyle;
            tStyleMods.erase(tStyleMods.begin() + i);
            i--;
        }
    }

    remove_duplicate_text_style_mods();
    inputChangedTextBox = true;
    needsRebuild = true;

    return start;
}

SkRect TextBox::get_cursor_rect(TextPosition pos) {
    pos = move(Movement::NOWHERE, pos);

    SkPoint topPoint;
    float height;

    const ParagraphData& pData = paragraphs[pos.fParagraphIndex];
    if(pData.text.empty()) {
        // Left aligned
        if((pData.pStyleData.textAlignment == skia::textlayout::TextAlign::kLeft) || (pData.pStyleData.textAlignment == skia::textlayout::TextAlign::kJustify && pData.pStyleData.textDirection == skia::textlayout::TextDirection::kLtr))
            topPoint.fX = 0.0f;
        // Right aligned
        else if((pData.pStyleData.textAlignment == skia::textlayout::TextAlign::kRight) || (pData.pStyleData.textAlignment == skia::textlayout::TextAlign::kJustify && pData.pStyleData.textDirection == skia::textlayout::TextDirection::kRtl))
            topPoint.fX = pData.p->getMaxWidth();
        // Centered
        else
            topPoint.fX = pData.p->getMaxWidth() * 0.5f;
        topPoint.fY = pData.heightOffset;
        height = pData.p->getHeight();
    }
    else {
        if(pos.fTextByteIndex == 0) {
            skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
            bool exists = pData.p->getGlyphClusterAt(pos.fTextByteIndex, &glyphInfo);
            if(exists) {
                bool isLtr = glyphInfo.fGlyphClusterPosition == skia::textlayout::TextDirection::kLtr;
                topPoint = (isLtr ? glyphInfo.fBounds.TL() : glyphInfo.fBounds.TR()) + SkPoint{0.0f, pData.heightOffset};
                height = glyphInfo.fBounds.height();
            }
            else {
                topPoint = {0.0f, pData.heightOffset};
                height = 0.0f;
            }
        }
        else {
            skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
            bool exists = pData.p->getGlyphClusterAt(prev_grapheme(pData.text, pos.fTextByteIndex), &glyphInfo);
            if(exists) {
                bool isLtr = glyphInfo.fGlyphClusterPosition == skia::textlayout::TextDirection::kLtr;
                topPoint = (isLtr ? glyphInfo.fBounds.TR() : glyphInfo.fBounds.TL()) + SkPoint{0.0f, pData.heightOffset};
                height = glyphInfo.fBounds.height();
            }
            else {
                topPoint = {0.0f, pData.heightOffset};
                height = 0.0f;
            }
        }
    }

    constexpr float CURSOR_WIDTH = 2.0f;

    return SkRect::MakeXYWH(topPoint.x() - CURSOR_WIDTH * 0.5f, topPoint.y(), CURSOR_WIDTH, height);
}

skia::textlayout::TextDirection TextBox::get_suggested_direction(size_t pIndex) {
    rebuild();

    auto u = SkUnicodes::ICU::Make();

    pIndex = std::min(pIndex, paragraphs.size() - 1);
    auto& pData = paragraphs[pIndex];
    if(pData.text.empty())
        return skia::textlayout::TextDirection::kLtr;
    skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
    bool exists = pData.p->getGlyphClusterAt(0, &glyphInfo);
    if(!exists)
        return skia::textlayout::TextDirection::kLtr;
    return glyphInfo.fGlyphClusterPosition;
}

void TextBox::rects_between_text_positions_func(TextPosition p1, TextPosition p2, std::function<void(const SkRect& r)> f) {
    auto [start, end] = get_start_end_text_pos(p1, p2);

    constexpr float SELECTION_RECT_EXTRA_AREA = 1.0f;

    for(size_t p = start.fParagraphIndex; p <= end.fParagraphIndex; p++) {
        size_t tStart = p == start.fParagraphIndex ? start.fTextByteIndex : 0;
        size_t tEnd = p == end.fParagraphIndex ? end.fTextByteIndex : paragraphs[p].text.size();
        ParagraphData& pData = paragraphs[p];
        for(size_t t = tStart; t < tEnd; t++) {
            skia::textlayout::Paragraph::GlyphClusterInfo glyphInfo;
            if(pData.p->getGlyphClusterAt(t, &glyphInfo)) {
                int lineNumber = pData.p->getLineNumberAt(t);
                if(lineNumber == -1)
                    break;
                skia::textlayout::LineMetrics lineMetrics;
                if(pData.p->getLineMetricsAt(lineNumber, &lineMetrics)) {
                    SkRect r = SkRect::MakeLTRB(glyphInfo.fBounds.left() - SELECTION_RECT_EXTRA_AREA, lineMetrics.fBaseline - lineMetrics.fAscent - SELECTION_RECT_EXTRA_AREA + pData.heightOffset, glyphInfo.fBounds.right() + SELECTION_RECT_EXTRA_AREA, lineMetrics.fBaseline + lineMetrics.fDescent + pData.heightOffset + SELECTION_RECT_EXTRA_AREA);
                    f(r);
                }
            }
        }
    }
}

std::pair<TextPosition, TextPosition> TextBox::get_start_end_text_pos(TextPosition p1, TextPosition p2) {
    p1 = move(Movement::NOWHERE, p1);
    p2 = move(Movement::NOWHERE, p2);
    return {std::min(p1, p2), std::max(p1, p2)};
}

std::pair<size_t, size_t> TextBox::get_start_end_paragraph_pos(size_t p1, size_t p2) {
    p1 = std::min(p1, paragraphs.size() - 1);
    p2 = std::min(p2, paragraphs.size() - 1);
    return {std::min(p1, p2), std::max(p1, p2)};
}

void TextBox::paint(SkCanvas* canvas, const PaintOpts& paintOpts) {
    rebuild();

    for(auto& pData : paragraphs)
        pData.p->paint(canvas, 0.0f, pData.heightOffset);

    if(paintOpts.cursor.has_value()) {
        auto& cur = paintOpts.cursor.value();
        SkPaint selectionP{SkColor4f{paintOpts.cursorColor.x(), paintOpts.cursorColor.y(), paintOpts.cursorColor.z(), 1.0f}};
        if(cur.selectionBeginPos != cur.selectionEndPos) {
            canvas->saveLayerAlphaf(nullptr, 0.5f);
            rects_between_text_positions_func(cur.selectionBeginPos, cur.selectionEndPos, [&](const SkRect& rect) {
                canvas->drawRect(rect, selectionP);
            });
            canvas->restore();
        }
        SkPaint cursorP{SkColor4f{paintOpts.cursorColor.x(), paintOpts.cursorColor.y(), paintOpts.cursorColor.z(), 1.0f}};
        canvas->drawRect(get_cursor_rect(cur.pos), cursorP);
    }
}

}
