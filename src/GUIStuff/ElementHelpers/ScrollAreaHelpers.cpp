#include "ScrollAreaHelpers.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff { namespace ElementHelpers {

void scroll_area_many_entries(GUIManager& gui, const char* id, const ScrollBarManyEntriesOptions& options) {
    gui.element<ScrollArea>(id, ScrollArea::Options{
        .scrollVertical = true,
        .clipHorizontal = options.clipHorizontal,
        .clipVertical = true,
        .showScrollbarY = true,
        .innerContent = [&, options](const ScrollArea::InnerContentParameters& params) {
            options.innerContentExtraCallback(params);

            float absScrollAmount = std::fabs(params.scrollOffset.y());
            size_t startPoint = absScrollAmount / options.entryHeight;
            size_t elementsContainable = (params.containerDimensions.y() / options.entryHeight) + 2; // Displaying 2 more elements ensures that there isn't any empty space in the container
            size_t endPoint = std::min(options.entryCount, startPoint + elementsContainable);

            if(startPoint != 0) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(startPoint * options.entryHeight)},
                    }
                }) { }
            }

            for(size_t i = startPoint; i < endPoint; i++)
                gui.new_id(i, [&] { options.elementContent(i); });

            if(endPoint != options.entryCount) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED((options.entryCount - endPoint) * options.entryHeight)},
                    }
                }) { }
            }
        }
    });
}

}}
