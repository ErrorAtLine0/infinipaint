#include "ScrollAreaHelpers.hpp"
#include "../GUIManager.hpp"
#include "../Elements/LayoutElement.hpp"

namespace GUIStuff { namespace ElementHelpers {

ScrollArea* scroll_area_many_entries(GUIManager& gui, const char* id, const ScrollBarManyEntriesOptions& options) {
    return gui.clipping_element<ScrollArea>(id, ScrollArea::Options{
        .scrollVertical = true,
        .clipHorizontal = options.clipHorizontal,
        .clipVertical = true,
        .showScrollbarY = true,
        .innerContent = [&](const ScrollArea::InnerContentParameters& params) {
            if(options.innerContentExtraCallback) options.innerContentExtraCallback(params);

            if(params.containerDimensions.y() == 0.0f)
                return;

            float absScrollAmount = std::fabs(params.scrollOffset->y());
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

            gui.new_id("many element list", [&] {
                for(size_t i = startPoint; i < endPoint; i++) {
                    gui.new_id(i - startPoint, [&] {
                        gui.element<LayoutElement>("elem", [&] (LayoutElement*, const Clay_ElementId& lId) {
                            CLAY(lId, {
                                .layout = { .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(options.entryHeight)}}
                            }) {
                                options.elementContent(i);
                            }
                        });
                    });
                }
            });

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
