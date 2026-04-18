#pragma once
#include "Element.hpp"
#include "Helpers/ConvertVec.hpp"
#include "ScrollArea.hpp"

namespace GUIStuff {

class GridScrollArea : public Element {
    public:
        GridScrollArea(GUIManager& gui);

        struct Options {
            float entryMaximumWidth;
            Clay_LayoutAlignmentX childAlignmentX = CLAY_ALIGN_X_LEFT;
            float entryHeight;
            size_t entryCount;
            std::function<void(size_t elementIndex)> elementContent;
            std::function<void(const ScrollArea::InnerContentParameters&)> innerContentExtraCallback;
        };

        void layout(const Clay_ElementId& id, const Options& options);
    private:
        float rowWidth = 0.0f;
};

}
