#include "ParagraphStyleData.hpp"

namespace RichText {

skia::textlayout::ParagraphStyle ParagraphStyleData::get_paragraph_style() const {
    skia::textlayout::ParagraphStyle pStyle;
    pStyle.setTextAlign(textAlignment);
    pStyle.setTextDirection(textDirection);
    return pStyle;
}

}
