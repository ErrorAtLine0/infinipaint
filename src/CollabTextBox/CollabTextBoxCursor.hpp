// This file has been copied from the Skia library, and may be slightly modified. Please check the NOTICE file for more details

#pragma once
#include <iostream>
#include <limits>

namespace CollabTextBox {
    struct TextPosition {
        // Set to uint32_t values because theyll be saved and sent over networks. Size has to be consistent
        size_t fTextByteIndex = std::numeric_limits<uint32_t>::max();
        size_t fParagraphIndex = std::numeric_limits<uint32_t>::max();
        template <typename Archive> void serialize(Archive& a) {
            a((uint32_t)fTextByteIndex, (uint32_t)fParagraphIndex);
        }
        bool operator==(const TextPosition& o) const = default;
    };
    
    static inline bool operator<(const CollabTextBox::TextPosition& u,
                                 const CollabTextBox::TextPosition& v) {
        return u.fParagraphIndex < v.fParagraphIndex ||
               (u.fParagraphIndex == v.fParagraphIndex && u.fTextByteIndex < v.fTextByteIndex);
    }

    struct Cursor {
        TextPosition pos{0, 0};
        TextPosition selectionBeginPos{0, 0};
        TextPosition selectionEndPos{0, 0};
        template <typename Archive> void serialize(Archive& a) {
            a(pos, selectionBeginPos, selectionEndPos);
        }
        bool operator==(const Cursor& o) const = default;
    };
}
