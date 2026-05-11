/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ScrollArea.hpp"
#include "../GUIManager.hpp"
#include <Helpers/ConvertVec.hpp>
#include <Helpers/Logger.hpp>
#include "LayoutElement.hpp"

namespace GUIStuff {

constexpr float TOUCH_SCROLLER_WIDTH = 24.0f;
constexpr float DESKTOP_SCROLLER_WIDTH = 12.0f;
constexpr float SCROLL_WHEEL_SCROLL_SPEED = 20.0f;
constexpr float SCROLL_DAMPING = 0.15f; // Damping on the scroll area velocity when its scrolling on its own
constexpr float SCROLL_MINIMUM_MOTION = 0.15f; // When the scroll area is scrolling on its own, this is the minimum velocity required for it to continue
constexpr float SCROLL_MAX_DAMPING = 0.0001f; // Damping on the scrollAreaMotionMax variable
constexpr float SCROLL_MINIMUM_MOTION_TO_START_MOVE = 1.0f; // Right after releasing touch, this is the minimum velocity for the area to start scrolling on its own

ScrollArea::ScrollArea(GUIManager& gui): Element(gui) {}

void ScrollArea::layout(const Clay_ElementId& id, const Options& options) {
    opts = options;
    Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(id);
    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
            .childAlignment = {.x = opts.xAlign, .y = opts.yAlign},
            .layoutDirection = opts.layoutDirection
        },
        .clip = {.horizontal = opts.clipHorizontal, .vertical = opts.clipVertical, .childOffset = {.x = scrollOffset.x(), .y = scrollOffset.y()}}
    }) {
        if(scrollData.found)
            contentDimensions = {scrollData.contentDimensions.width, scrollData.contentDimensions.height};
        if(boundingBox.has_value()) {
            auto& bb = boundingBox.value();
            clamp_scroll();

            if((contentDimensions.y() > bb.height() && opts.scrollVertical) || (contentDimensions.x() > bb.width() && opts.scrollHorizontal)) {
                gui.in_dynamic_area([&] {
                    opts.innerContent({.contentDimensions = contentDimensions, .containerDimensions = bb.dim(), .scrollOffset = &scrollOffset});
                });
            }
            else
                opts.innerContent({.contentDimensions = contentDimensions, .containerDimensions = bb.dim(), .scrollOffset = &scrollOffset});

            clamp_scroll();

            if(contentDimensions.y() > bb.height() && opts.scrollVertical && opts.scrollbarY != ScrollbarType::NONE)
                y_scroll_bar();
            else {
                yScrollbar.scroller = nullptr;
                yScrollbar.scrollbar = nullptr;
            }

            if(contentDimensions.x() > bb.width() && opts.scrollHorizontal && opts.scrollbarX != ScrollbarType::NONE)
                x_scroll_bar();
            else {
                xScrollbar.scroller = nullptr;
                xScrollbar.scrollbar = nullptr;
            }
        }
    }
}

void ScrollArea::y_scroll_bar() {
    float sAreaDim = boundingBox.value().height();
    float sAreaStart = boundingBox.value().min.y();
    float contDim = contentDimensions.y();
    float scrollerSize = (sAreaDim / contDim) * sAreaDim;
    float scrollPosMax = contDim - sAreaDim; 
    float scrollerPos = std::fabs(scrollOffset.y() / scrollPosMax);
    float areaAboveScrollerSize = scrollerPos * (sAreaDim - scrollerSize);

    gui.set_z_index(gui.get_z_index() + 1, [&] {
        yScrollbar.scrollbar = gui.element<LayoutElement>("y scroll bar", [&] (LayoutElement*, const Clay_ElementId& lId) {
            CLAY(lId, {
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(gui.io.isTouchDevice ? TOUCH_SCROLLER_WIDTH : DESKTOP_SCROLLER_WIDTH), .height = CLAY_SIZING_GROW(0)},
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .floating = {
                    .zIndex = gui.get_z_index(),
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_RIGHT_TOP,
                        .parent = CLAY_ATTACH_POINT_RIGHT_TOP
                    },
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                }
            }) {
                SkColor4f scrollerColor;
                if(yScrollbar.isScrollbarHeld)
                    scrollerColor = gui.io.theme->fillColor1;
                else if(yScrollbar.isScrollbarHovered)
                    scrollerColor = gui.io.theme->fillColor1;
                else
                    scrollerColor = gui.io.theme->fillColor2;

                CLAY_AUTO_ID({ 
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(areaAboveScrollerSize)}
                    }
                }) {}

                yScrollbar.scroller = gui.element<LayoutElement>("scroller", [&] (LayoutElement*, const Clay_ElementId& lId2) {
                    CLAY(lId2, {
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(scrollerSize)},
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        }
                    }) {
                        CLAY_AUTO_ID({
                            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
                        }) {}
                        float scrollerWidthPercent = 0.0f;
                        bool highlighted = yScrollbar.isHoveringOverScroller || yScrollbar.isScrollbarHeld || (!gui.io.isTouchDevice && yScrollbar.isScrollbarHovered);
                        if(gui.io.isTouchDevice)
                            scrollerWidthPercent = highlighted ? 0.6f : 0.2f;
                        else
                            scrollerWidthPercent = highlighted ? 1.0f : 0.5f;
                        CLAY_AUTO_ID({
                            .layout = {.sizing = {.width = CLAY_SIZING_PERCENT(scrollerWidthPercent), .height = CLAY_SIZING_GROW(0)}},
                            .backgroundColor = convert_vec4<Clay_Color>(highlighted ? gui.io.theme->fillColor1 : gui.io.theme->fillColor2),
                            .cornerRadius = CLAY_CORNER_RADIUS(3),
                        }) {}
                    }
                });

                CLAY_AUTO_ID({ .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
            }
        }, LayoutElement::Callbacks{
            .onClick = [&, scrollPosMax, sAreaStart, sAreaDim, scrollerSize, areaAboveScrollerSize](LayoutElement* t, const InputManager::MouseButtonCallbackArgs& button) {
                bool newIsHoveringOverScroller = yScrollbar.scroller->mouseHovering && (button.deviceType != InputManager::MouseDeviceType::TOUCH || button.down);
                bool newIsScrollbarHovered = t->mouseHovering && (button.deviceType != InputManager::MouseDeviceType::TOUCH || button.down);
                if(yScrollbar.isHoveringOverScroller != newIsHoveringOverScroller) {
                    yScrollbar.isHoveringOverScroller = newIsHoveringOverScroller;
                    gui.set_to_layout();
                }
                if(yScrollbar.isScrollbarHovered != newIsScrollbarHovered) {
                    yScrollbar.isScrollbarHovered = newIsScrollbarHovered;
                    gui.set_to_layout();
                }
                if(button.button == InputManager::MouseButton::LEFT) {
                    if(button.down && yScrollbar.isScrollbarHovered) {
                        yScrollbar.isScrollbarHeld = true;
                        if(yScrollbar.isHoveringOverScroller)
                            yScrollbar.scrollerStartPos = sAreaStart + areaAboveScrollerSize + scrollerSize * 0.5f;
                        else
                            yScrollbar.scrollerStartPos = std::clamp(button.pos.y(), sAreaStart + scrollerSize * 0.5f, sAreaStart + sAreaDim - scrollerSize * 0.5f);
                        yScrollbar.mouseStartPos = button.pos.y();

                        float newScrollPosFrac;
                        newScrollPosFrac = std::clamp((yScrollbar.scrollerStartPos - (yScrollbar.mouseStartPos - button.pos.y()) - sAreaStart - scrollerSize * 0.5f) / (sAreaDim - scrollerSize), 0.0f, 1.0f);
                        scrollOffset.y() = newScrollPosFrac * (-scrollPosMax);
                        clamp_scroll();
                        scrollAreaMotionMax = scrollAreaMotion = {0.0f, 0.0f};
                        gui.set_to_layout();
                    }
                    else if(yScrollbar.isScrollbarHeld) {
                        yScrollbar.isScrollbarHeld = false;
                        scrollAreaMotionMax = scrollAreaMotion = {0.0f, 0.0f};
                        gui.set_to_layout();
                    }
                }
            },
            .onMotion = [&, scrollPosMax, sAreaStart, sAreaDim, scrollerSize](LayoutElement* t, const InputManager::MouseMotionCallbackArgs& motion) {
                bool newIsHoveringOverScroller = yScrollbar.scroller->mouseHovering && motion.deviceType != InputManager::MouseDeviceType::TOUCH;
                bool newIsScrollbarHovered = t->mouseHovering && motion.deviceType != InputManager::MouseDeviceType::TOUCH;
                if(yScrollbar.isHoveringOverScroller != newIsHoveringOverScroller) {
                    yScrollbar.isHoveringOverScroller = newIsHoveringOverScroller;
                    gui.set_to_layout();
                }
                if(yScrollbar.isScrollbarHovered != newIsScrollbarHovered) {
                    yScrollbar.isScrollbarHovered = newIsScrollbarHovered;
                    gui.set_to_layout();
                }
                if(yScrollbar.isScrollbarHeld) {
                    float oldScrollOffset = scrollOffset.y();

                    float newScrollPosFrac;
                    newScrollPosFrac = std::clamp((yScrollbar.scrollerStartPos - (yScrollbar.mouseStartPos - motion.pos.y()) - sAreaStart - scrollerSize * 0.5f) / (sAreaDim - scrollerSize), 0.0f, 1.0f);
                    scrollOffset.y() = newScrollPosFrac * (-scrollPosMax);
                    clamp_scroll();
                    scrollAreaMotionMax = scrollAreaMotion = {0.0f, 0.0f};
                    if(oldScrollOffset != scrollOffset.y())
                        gui.set_to_layout();
                }
            }
        });
    });
}

void ScrollArea::x_scroll_bar() {
    float sAreaDim = boundingBox.value().width();
    float sAreaStart = boundingBox.value().min.x();
    float contDim = contentDimensions.x();
    float scrollerSize = (sAreaDim / contDim) * sAreaDim;
    float scrollPosMax = contDim - sAreaDim; 
    float scrollerPos = std::fabs(scrollOffset.x() / scrollPosMax);
    float areaAboveScrollerSize = scrollerPos * (sAreaDim - scrollerSize);

    gui.set_z_index(gui.get_z_index() + 1, [&] {
        xScrollbar.scrollbar = gui.element<LayoutElement>("x scroll bar", [&] (LayoutElement*, const Clay_ElementId& lId) {
            CLAY(lId, {
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(gui.io.isTouchDevice ? TOUCH_SCROLLER_WIDTH : DESKTOP_SCROLLER_WIDTH)},
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                },
                .floating = {
                    .zIndex = gui.get_z_index(),
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                        .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM
                    },
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                }
            }) {
                SkColor4f scrollerColor;
                if(xScrollbar.isScrollbarHeld)
                    scrollerColor = gui.io.theme->fillColor1;
                else if(xScrollbar.isScrollbarHovered)
                    scrollerColor = gui.io.theme->fillColor1;
                else
                    scrollerColor = gui.io.theme->fillColor2;

                CLAY_AUTO_ID({ 
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIXED(areaAboveScrollerSize), .height = CLAY_SIZING_GROW(0)}
                    }
                }) {}

                xScrollbar.scroller = gui.element<LayoutElement>("scroller", [&] (LayoutElement*, const Clay_ElementId& lId2) {
                    CLAY(lId2, {
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(scrollerSize), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        }
                    }) {
                        CLAY_AUTO_ID({
                            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
                        }) {}
                        float scrollerWidthPercent = 0.0f;
                        bool highlighted = xScrollbar.isHoveringOverScroller || xScrollbar.isScrollbarHeld || (!gui.io.isTouchDevice && xScrollbar.isScrollbarHovered);
                        if(gui.io.isTouchDevice)
                            scrollerWidthPercent = highlighted ? 0.6f : 0.2f;
                        else
                            scrollerWidthPercent = highlighted ? 1.0f : 0.5f;
                        CLAY_AUTO_ID({
                            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_PERCENT(scrollerWidthPercent)}},
                            .backgroundColor = convert_vec4<Clay_Color>(highlighted ? gui.io.theme->fillColor1 : gui.io.theme->fillColor2),
                            .cornerRadius = CLAY_CORNER_RADIUS(3),
                        }) {}
                    }
                });

                CLAY_AUTO_ID({ .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
            }
        }, LayoutElement::Callbacks{
            .onClick = [&, scrollPosMax, sAreaStart, sAreaDim, scrollerSize, areaAboveScrollerSize](LayoutElement* t, const InputManager::MouseButtonCallbackArgs& button) {
                bool newIsHoveringOverScroller = xScrollbar.scroller->mouseHovering && (button.deviceType != InputManager::MouseDeviceType::TOUCH || button.down);
                bool newIsScrollbarHovered = t->mouseHovering && (button.deviceType != InputManager::MouseDeviceType::TOUCH || button.down);
                if(xScrollbar.isHoveringOverScroller != newIsHoveringOverScroller) {
                    xScrollbar.isHoveringOverScroller = newIsHoveringOverScroller;
                    gui.set_to_layout();
                }
                if(xScrollbar.isScrollbarHovered != newIsScrollbarHovered) {
                    xScrollbar.isScrollbarHovered = newIsScrollbarHovered;
                    gui.set_to_layout();
                }
                if(button.button == InputManager::MouseButton::LEFT) {
                    if(button.down && xScrollbar.isScrollbarHovered) {
                        xScrollbar.isScrollbarHeld = true;
                        if(xScrollbar.isHoveringOverScroller)
                            xScrollbar.scrollerStartPos = sAreaStart + areaAboveScrollerSize + scrollerSize * 0.5f;
                        else
                            xScrollbar.scrollerStartPos = std::clamp(button.pos.x(), sAreaStart + scrollerSize * 0.5f, sAreaStart + sAreaDim - scrollerSize * 0.5f);
                        xScrollbar.mouseStartPos = button.pos.x();

                        float newScrollPosFrac;
                        newScrollPosFrac = std::clamp((xScrollbar.scrollerStartPos - (xScrollbar.mouseStartPos - button.pos.x()) - sAreaStart - scrollerSize * 0.5f) / (sAreaDim - scrollerSize), 0.0f, 1.0f);
                        scrollOffset.x() = newScrollPosFrac * (-scrollPosMax);
                        clamp_scroll();
                        scrollAreaMotionMax = scrollAreaMotion = {0.0f, 0.0f};
                        gui.set_to_layout();
                    }
                    else if(xScrollbar.isScrollbarHeld) {
                        xScrollbar.isScrollbarHeld = false;
                        scrollAreaMotionMax = scrollAreaMotion = {0.0f, 0.0f};
                        gui.set_to_layout();
                    }
                }
            },
            .onMotion = [&, scrollPosMax, sAreaStart, sAreaDim, scrollerSize](LayoutElement* t, const InputManager::MouseMotionCallbackArgs& motion) {
                bool newIsHoveringOverScroller = xScrollbar.scroller->mouseHovering && motion.deviceType != InputManager::MouseDeviceType::TOUCH;
                bool newIsScrollbarHovered = t->mouseHovering && motion.deviceType != InputManager::MouseDeviceType::TOUCH;
                if(xScrollbar.isHoveringOverScroller != newIsHoveringOverScroller) {
                    xScrollbar.isHoveringOverScroller = newIsHoveringOverScroller;
                    gui.set_to_layout();
                }
                if(xScrollbar.isScrollbarHovered != newIsScrollbarHovered) {
                    xScrollbar.isScrollbarHovered = newIsScrollbarHovered;
                    gui.set_to_layout();
                }
                if(xScrollbar.isScrollbarHeld) {
                    float oldScrollOffset = scrollOffset.x();

                    float newScrollPosFrac;
                    newScrollPosFrac = std::clamp((xScrollbar.scrollerStartPos - (xScrollbar.mouseStartPos - motion.pos.x()) - sAreaStart - scrollerSize * 0.5f) / (sAreaDim - scrollerSize), 0.0f, 1.0f);
                    scrollOffset.x() = newScrollPosFrac * (-scrollPosMax);
                    clamp_scroll();
                    scrollAreaMotionMax = scrollAreaMotion = {0.0f, 0.0f};
                    if(oldScrollOffset != scrollOffset.x())
                        gui.set_to_layout();
                }
            }
        });
    });
}

void ScrollArea::reset_scroll() {
    scrollAreaMotion = scrollAreaMotionMax = scrollOffset = {0.0f, 0.0f};
}

void ScrollArea::clamp_scroll() {
    if(boundingBox.has_value()) {
        scrollOffset.x() = std::clamp(scrollOffset.x(), -std::max(0.0f, contentDimensions.x() - boundingBox.value().width()), 0.0f);
        scrollOffset.y() = std::clamp(scrollOffset.y(), -std::max(0.0f, contentDimensions.y() - boundingBox.value().height()), 0.0f);
    }
}

void ScrollArea::update() {
    if((scrollAreaMotion.x() != 0.0f || scrollAreaMotion.y() != 0.0f) && !scrollAreaHeld) {
        Vector2f oldScrollOffset = scrollOffset;
        scrollOffset.x() += scrollAreaMotion.x();
        scrollOffset.y() += scrollAreaMotion.y();
        clamp_scroll();
        scrollAreaMotion *= std::pow(SCROLL_DAMPING, gui.io.deltaTime);
        if(std::fabs(scrollAreaMotion.y()) <= SCROLL_MINIMUM_MOTION)
            scrollAreaMotion.y() = 0.0f;
        if(std::fabs(scrollAreaMotion.x()) <= SCROLL_MINIMUM_MOTION)
            scrollAreaMotion.x() = 0.0f;
        if(oldScrollOffset != scrollOffset)
            gui.set_to_layout();
        else
            scrollAreaMotion = {0.0f, 0.0f};
    }
    else if(scrollAreaHeld)
        scrollAreaMotionMax *= std::pow(SCROLL_MAX_DAMPING, gui.io.deltaTime);
}

void ScrollArea::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    if(mouseHovering || (xScrollbar.scrollbar && xScrollbar.scrollbar->mouseHovering) || (yScrollbar.scrollbar && yScrollbar.scrollbar->mouseHovering)) {
        Vector2f oldScrollOffset = scrollOffset;

        if(opts.scrollVertical) {
            scrollOffset.y() += wheel.amount.y() * SCROLL_WHEEL_SCROLL_SPEED;
            if(opts.scrollHorizontal)
                scrollOffset.x() += wheel.amount.x() * SCROLL_WHEEL_SCROLL_SPEED;
        }
        else if(opts.scrollHorizontal)
            scrollOffset.x() += wheel.amount.y() * SCROLL_WHEEL_SCROLL_SPEED;

        clamp_scroll();

        if(oldScrollOffset != scrollOffset)
            gui.set_to_layout();
    }
}

void ScrollArea::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.deviceType != InputManager::MouseDeviceType::TOUCH)
        scrollAreaMotion = scrollAreaMotionMax = {0.0f, 0.0f};
}

void ScrollArea::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(motion.deviceType != InputManager::MouseDeviceType::TOUCH)
        scrollAreaMotion = scrollAreaMotionMax = {0.0f, 0.0f};
}

void ScrollArea::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
    bool oldScrollAreaHeld = scrollAreaHeld;
    scrollAreaHeld = mouseHovering && touch.down;
    if(scrollAreaHeld)
        scrollAreaMotionMax = scrollAreaMotion = {0.0f, 0.0f};
    else if(oldScrollAreaHeld) {
        scrollAreaMotion = scrollAreaMotionMax;
        if(std::fabs(scrollAreaMotion.x()) < SCROLL_MINIMUM_MOTION_TO_START_MOVE)
            scrollAreaMotion.x() = 0.0f;
        if(std::fabs(scrollAreaMotion.y()) < SCROLL_MINIMUM_MOTION_TO_START_MOVE)
            scrollAreaMotion.y() = 0.0f;
    }
}

void ScrollArea::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
    if(scrollAreaHeld) {
        scrollAreaMotion = {opts.scrollHorizontal ? motion.move.x() : 0.0f, opts.scrollVertical ? motion.move.y() : 0.0f};
        if(std::fabs(scrollAreaMotion.x()) > std::fabs(scrollAreaMotionMax.x()))
            scrollAreaMotionMax.x() = scrollAreaMotion.x();
        if(std::fabs(scrollAreaMotion.y()) > std::fabs(scrollAreaMotionMax.y()))
            scrollAreaMotionMax.y() = scrollAreaMotion.y();

        Vector2f oldScrollOffset = scrollOffset;
        scrollOffset.x() += scrollAreaMotion.x();
        scrollOffset.y() += scrollAreaMotion.y();
        clamp_scroll();
        if(oldScrollOffset != scrollOffset)
            gui.set_to_layout();
    }
}

}
