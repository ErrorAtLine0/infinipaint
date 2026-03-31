#include "ScrollArea.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

ScrollArea::ScrollArea(GUIManager& gui): Element(gui) {}

void ScrollArea::layout(const Clay_ElementId& id, const Options& options) {
    opts = options;
    CLAY(id, {
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
    }) {
        Clay_ElementId localID = CLAY_ID_LOCAL("SCROLL_AREA");

        Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(localID);

        if(scrollData.found) {
            contentDimensions = {scrollData.contentDimensions.width, scrollData.contentDimensions.height};
            containerDimensions = {scrollData.scrollContainerDimensions.width, scrollData.scrollContainerDimensions.height};
        }

        CLAY(localID, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = opts.layoutDirection
            },
            .clip = {.horizontal = opts.clipHorizontal, .vertical = opts.clipVertical, .childOffset = {.x = scrollOffset.x(), .y = scrollOffset.y()}}
        }) {
            opts.innerContent({.contentDimensions = contentDimensions, .containerDimensions = containerDimensions, .scrollOffset = scrollOffset});
        }

        //if(scrollData.contentDimensions.height > scrollData.scrollContainerDimensions.height) {
        //    float sAreaDim = scrollData.scrollContainerDimensions.height;
        //    float contDim = scrollData.contentDimensions.height;
        //    float scrollerSize = (sAreaDim / contDim) * sAreaDim;
        //    float scrollPosMax = contDim - sAreaDim; 
        //    float scrollerPos = std::fabs(scrollData.scrollPosition->y / scrollPosMax);
        //    float areaAboveScrollerSize = scrollerPos * (sAreaDim - scrollerSize);

        //    currentScrollPos = scrollData.scrollPosition->y;

        //    CLAY_AUTO_ID({
        //        .layout = {
        //            .sizing = {.width = CLAY_SIZING_FIXED(12), .height = CLAY_SIZING_GROW(0)},
        //            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
        //            .layoutDirection = CLAY_TOP_TO_BOTTOM
        //        },
        //        .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor2)
        //    }) {
        //        SkColor4f scrollerColor;
        //        if(sD.isMoving)
        //            scrollerColor = io.theme->fillColor1;
        //        else if(Clay_Hovered())
        //            scrollerColor = io.theme->fillColor1;
        //        else
        //            scrollerColor = io.theme->fillColor2;

        //        bool isHoveringOverScroller = false;
        //        CLAY_AUTO_ID({ 
        //            .layout = {
        //                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(areaAboveScrollerSize)}
        //            }
        //        }) {}
        //        CLAY_AUTO_ID({
        //            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(scrollerSize)}},
        //            .backgroundColor = convert_vec4<Clay_Color>(scrollerColor),
        //            .cornerRadius = CLAY_CORNER_RADIUS(3),
        //        }) {
        //            if(Clay_Hovered())
        //                isHoveringOverScroller = true;
        //        }
        //        CLAY_AUTO_ID({ .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
        //        if(Clay_Hovered() && io.mouse.leftClick) {
        //            isMoving = true;
        //            if(isHoveringOverScroller)
        //                scrollerStartPos = scrollAreaBB.y + areaAboveScrollerSize + scrollerSize * 0.5f;
        //            else
        //                scrollerStartPos = std::clamp(io.mouse.pos.y(), scrollAreaBB.y + scrollerSize * 0.5f, scrollAreaBB.y + scrollAreaBB.height - scrollerSize * 0.5f);
        //            mouseStartPos = io.mouse.pos.y();
        //        }
        //        if(!io.mouse.leftHeld)
        //            isMoving = false;
        //        if(isMoving) {
        //            float newScrollPosFrac;
        //            newScrollPosFrac = std::clamp((scrollerStartPos - (mouseStartPos - io.mouse.pos.y()) - scrollAreaBB.y - scrollerSize * 0.5f) / (scrollAreaBB.height - scrollerSize), 0.0f, 1.0f);
        //            currentScrollPos = newScrollPosFrac * (-scrollPosMax);
        //        }
        //        currentScrollPos = std::clamp(currentScrollPos, -scrollPosMax, 0.0f);
        //        scrollData.scrollPosition->y = currentScrollPos;
        //    }
        //}
        //else
        //    sD.currentScrollPos = 0.0f;
    }
}

bool ScrollArea::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
    return Element::input_mouse_button_callback(button, mouseHovering);
}

bool ScrollArea::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) {
    return Element::input_mouse_motion_callback(motion, mouseHovering);
}

bool ScrollArea::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel, bool mouseHovering) {
    if(mouseHovering) {
        if(opts.scrollVertical)
            scrollOffset.y() += wheel.amount.y();
        if(opts.scrollHorizontal)
            scrollOffset.x() += wheel.amount.x();
        gui.set_to_layout();
    }
    return Element::input_mouse_wheel_callback(wheel, mouseHovering);
}

}
