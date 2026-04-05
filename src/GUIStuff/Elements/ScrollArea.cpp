#include "ScrollArea.hpp"
#include "../GUIManager.hpp"
#include "LayoutElement.hpp"

namespace GUIStuff {

ScrollArea::ScrollArea(GUIManager& gui): Element(gui) {}

void ScrollArea::layout(const Clay_ElementId& id, const Options& options) {
    opts = options;
    CLAY(id, {
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
    }) {
        Clay_ElementId localID = CLAY_ID_LOCAL("SCROLL_AREA");

        Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(localID);
        Clay_ElementData elemData = Clay_GetElementData(localID);

        Vector2f bbPos = {0.0f, 0.0f};

        if(scrollData.found) {
            contentDimensions = {scrollData.contentDimensions.width, scrollData.contentDimensions.height};
            containerDimensions = {scrollData.scrollContainerDimensions.width, scrollData.scrollContainerDimensions.height};
        }

        if(elemData.found)
            bbPos = {elemData.boundingBox.x, elemData.boundingBox.y};

        clamp_scroll();

        CLAY(localID, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = opts.layoutDirection
            },
            .clip = {.horizontal = opts.clipHorizontal, .vertical = opts.clipVertical, .childOffset = {.x = scrollOffset.x(), .y = scrollOffset.y()}}
        }) {
            opts.innerContent({.contentDimensions = contentDimensions, .containerDimensions = containerDimensions, .scrollOffset = &scrollOffset});
            clamp_scroll();
        }

        if(scrollData.contentDimensions.height > scrollData.scrollContainerDimensions.height) {
            float sAreaDim = containerDimensions.y();
            float sAreaStart = bbPos.y();
            float contDim = contentDimensions.y();
            float scrollerSize = (sAreaDim / contDim) * sAreaDim;
            float scrollPosMax = contDim - sAreaDim; 
            float scrollerPos = std::fabs(scrollOffset.y() / scrollPosMax);
            float areaAboveScrollerSize = scrollerPos * (sAreaDim - scrollerSize);

            gui.set_z_index(gui.get_z_index() + 1, [&] {
                gui.element<LayoutElement>("scroll bar", [&] (LayoutElement*, const Clay_ElementId& lId) {
                    CLAY(lId, {
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(12), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                        .backgroundColor = convert_vec4<Clay_Color>(gui.io.theme->backColor2)
                    }) {
                        SkColor4f scrollerColor;
                        if(isScrollbarHeldY)
                            scrollerColor = gui.io.theme->fillColor1;
                        else if(isScrollbarHoveredY)
                            scrollerColor = gui.io.theme->fillColor1;
                        else
                            scrollerColor = gui.io.theme->fillColor2;

                        CLAY_AUTO_ID({ 
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(areaAboveScrollerSize)}
                            }
                        }) {}

                        gui.element<LayoutElement>("scroller", [&] (LayoutElement*, const Clay_ElementId& lId2) {
                            CLAY(lId2, {
                                .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(scrollerSize)}},
                                .backgroundColor = convert_vec4<Clay_Color>(scrollerColor),
                                .cornerRadius = CLAY_CORNER_RADIUS(3),
                            }) {}
                        }, LayoutElement::Callbacks{
                            .mouseMotion = [&] (LayoutElement* t, const InputManager::MouseMotionCallbackArgs& motion) {
                                if(isHoveringOverScrollerY != t->mouseHovering) {
                                    isHoveringOverScrollerY = t->mouseHovering;
                                    gui.set_to_layout();
                                }
                            }
                        });

                        CLAY_AUTO_ID({ .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                    }
                }, LayoutElement::Callbacks{
                    .mouseButton = [&, scrollPosMax, sAreaStart, sAreaDim, scrollerSize, areaAboveScrollerSize](LayoutElement* t, const InputManager::MouseButtonCallbackArgs& button) {
                        if(button.button == InputManager::MouseButton::LEFT) {
                            if(button.down && t->mouseHovering) {
                                isScrollbarHeldY = true;
                                if(isHoveringOverScrollerY)
                                    scrollerStartPosY = sAreaStart + areaAboveScrollerSize + scrollerSize * 0.5f;
                                else
                                    scrollerStartPosY = std::clamp(button.pos.y(), sAreaStart + scrollerSize * 0.5f, sAreaStart + sAreaDim - scrollerSize * 0.5f);
                                mouseStartPosY = button.pos.y();

                                float newScrollPosFrac;
                                newScrollPosFrac = std::clamp((scrollerStartPosY - (mouseStartPosY - button.pos.y()) - sAreaStart - scrollerSize * 0.5f) / (sAreaDim - scrollerSize), 0.0f, 1.0f);
                                scrollOffset.y() = newScrollPosFrac * (-scrollPosMax);

                                clamp_scroll();
                                gui.set_to_layout();
                            }
                            else if(isScrollbarHeldY) {
                                isScrollbarHeldY = false;
                                gui.set_to_layout();
                            }
                        }
                    },
                    .mouseMotion = [&, scrollPosMax, sAreaStart, sAreaDim, scrollerSize](LayoutElement* t, const InputManager::MouseMotionCallbackArgs& motion) {
                        if(isScrollbarHoveredY != t->mouseHovering) {
                            isScrollbarHoveredY = t->mouseHovering;
                            gui.set_to_layout();
                        }
                        if(isScrollbarHeldY) {
                            float oldScrollOffset = scrollOffset.y();

                            float newScrollPosFrac;
                            newScrollPosFrac = std::clamp((scrollerStartPosY - (mouseStartPosY - motion.pos.y()) - sAreaStart - scrollerSize * 0.5f) / (sAreaDim - scrollerSize), 0.0f, 1.0f);
                            scrollOffset.y() = newScrollPosFrac * (-scrollPosMax);

                            clamp_scroll();

                            if(oldScrollOffset != scrollOffset.y())
                                gui.set_to_layout();
                        }
                    }
                });
            });
        }
    }
}

void ScrollArea::reset_scroll() {
    scrollOffset = {0.0f, 0.0f};
}

void ScrollArea::clamp_scroll() {
    scrollOffset.x() = std::clamp(scrollOffset.x(), -std::max(0.0f, contentDimensions.x() - containerDimensions.x()), 0.0f);
    scrollOffset.y() = std::clamp(scrollOffset.y(), -std::max(0.0f, contentDimensions.y() - containerDimensions.y()), 0.0f);
}

void ScrollArea::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    if(mouseHovering) {
        Vector2f oldScrollOffset = scrollOffset;

        if(opts.scrollVertical)
            scrollOffset.y() = scrollOffset.y() + wheel.amount.y() * 10.0f;
        if(opts.scrollHorizontal)
            scrollOffset.y() = scrollOffset.x() + wheel.amount.x() * 10.0f;
        clamp_scroll();

        if(oldScrollOffset != scrollOffset)
            gui.set_to_layout();
    }
}

}
