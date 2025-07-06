// This file has been copied from the Skia library, and may be slightly modified. Please check the NOTICE file for more details

#pragma once
#include "CollabTextBoxCursor.hpp"
#include <include/core/SkColor.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkRefCnt.h>
#include <include/core/SkTextBlob.h>

#include <vector>
#include <string_view>
#include <string>
#include <Eigen/Dense>

class SkCanvas;
class SkShaper;

using namespace Eigen;

namespace CollabTextBox {
    struct TextParagraph {
        TextParagraph(std::string_view t) : fText(t) {}
        TextParagraph() {}
    
        std::string fText;
        sk_sp<const SkTextBlob> fBlob;
        std::vector<SkRect> fCursorPos;
        std::vector<size_t> fLineEndOffsets;
        std::vector<bool> fWordBoundaries;
        SkIPoint fOrigin = {0, 0};
        int fHeight = 0;
        bool fShaped = false;
    };

    enum class Movement {
        kNowhere,
        kLeft,
        kUp,
        kRight,
        kDown,
        kHome,
        kEnd,
        kWordLeft,
        kWordRight,
    };

    bool is_utf8_continuation(char v);
    const char* next_utf8(const char* p, const char* end);
    const char* align_utf8(const char* p, const char* begin);
    const char* prev_utf8(const char* p, const char* begin);

    class Editor {
        public:
            // total height in canvas display units.
            int getHeight() const { return fHeight; }
        
            // set display width in canvas display units
            void setWidth(int w); // may force re-shape
        
            // get/set current font (used for shaping and displaying text)
            const SkFont& font() const { return fFont; }
            void setFont(SkFont font);
        
            void setFontMgr(sk_sp<SkFontMgr> fontMgr);
        
            struct Text {
                const std::vector<TextParagraph>& fLines;
                struct Iterator {
                    std::vector<TextParagraph>::const_iterator fPtr;
                    std::string_view operator*() { return std::string_view(fPtr->fText); }
                    void operator++() { ++fPtr; }
                    bool operator!=(const Iterator& other) const { return fPtr != other.fPtr; }
                };
                Iterator begin() const { return Iterator{fLines.begin()}; }
                Iterator end() const { return Iterator{fLines.end()}; }
            };
            // Loop over all the lines of text.  The lines are not '\0'- or '\n'-terminated.
            // For example, to dump the entire file to standard output:
            //     for (SkPlainTextEditor::StringView str : editor.text()) {
            //         std::cout.write(str.data, str.size) << '\n';
            //     }
            Text text() const { return Text{fLines}; }
            std::string get_string() const;

            // get size of line in canvas display units.
            int lineHeight(size_t index) const { return fLines[index].fHeight; }
        
            TextPosition move(Movement move, TextPosition pos) const;
            TextPosition getPosition(SkIPoint);
            SkRect getLocation(TextPosition);
            // insert into current text.
            TextPosition insert(TextPosition, std::string_view utf8Text);
            // remove text between two positions
            TextPosition remove(TextPosition, TextPosition);
        
            std::string copy(TextPosition pos1, TextPosition pos2) const;
            size_t lineCount() const { return fLines.size(); }
            std::string_view line(size_t i) const {
                return i < fLines.size() ? std::string_view(fLines[i].fText) : std::string_view();
            }
        
            struct PaintOpts {
                SkColor4f fBackgroundColor = {1, 1, 1, 1};
                SkColor4f fForegroundColor = {0, 0, 0, 1};
                SkColor4f cursorColor = {1, 0, 0, 1};
                bool showCursor = false;
                Cursor cursor;
            };
            void paint(SkCanvas* canvas, PaintOpts options);
            std::vector<TextParagraph> fLines;
        private:
            int fWidth = 0;
            int fHeight = 0;
            SkFont fFont;
            sk_sp<SkFontMgr> fFontMgr;
            bool fNeedsReshape = false;
            const char* fLocale = "en";  // TODO: make this setable
        
            void markDirty(TextParagraph*);
            void reshapeAll();
    };

}

