#include "GridScrollArea.hpp"
#include "../ElementHelpers/ScrollAreaHelpers.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

GridScrollArea::GridScrollArea(GUIManager& gui): Element(gui) {}

void GridScrollArea::layout(const Clay_ElementId& id, const Options& options) {
    using namespace ElementHelpers;

    ScrollBarManyEntriesOptions opts;
    opts.clipHorizontal = true;
    opts.entryHeight = options.entryHeight;
    opts.innerContentExtraCallback = [&, extraCallback = options.innerContentExtraCallback] (const ScrollArea::InnerContentParameters& contParams) {
        rowWidth = std::fabs(contParams.containerDimensions.x());
        if(extraCallback) extraCallback(contParams);
    };
    size_t entriesPerRow = static_cast<size_t>(rowWidth / options.entryMaximumWidth) + 1;
    size_t rowCount = options.entryCount / entriesPerRow;
    float entryWidth = rowWidth == 0.0f ? 100.0f : (rowWidth / entriesPerRow);
    opts.entryCount = rowCount;
    opts.elementContent = [&] (size_t rowIndex) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(options.entryHeight)},
                .childAlignment = { .x = options.childAlignmentX}
            }
        }) {
            for(size_t i = 0; i < entriesPerRow; i++) {
                size_t entryNum = rowIndex * entriesPerRow + i;
                gui.new_id(static_cast<int64_t>(i), [&] {
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(entryWidth), .height = CLAY_SIZING_FIXED(options.entryHeight)}}
                    }) {
                        options.elementContent(entryNum);
                    }
                });
            }
        }
    };
    scroll_area_many_entries(gui, "scroll many area", opts);
}

}
