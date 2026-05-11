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

#include "LayoutHelpers.hpp"
#include "../Elements/LayoutElement.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../Elements/ManyElementScrollArea.hpp"

namespace GUIStuff { namespace ElementHelpers {

void top_to_bottom_window_popup_layout(GUIManager& gui, const char* id, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void(LayoutElement*)>& innerContent, const LayoutElement::Callbacks& callbacks) {
    gui.element<LayoutElement>(id, [&] (LayoutElement* l, const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = { 
                .sizing = {.width = x, .height = y },
                .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                .childGap = gui.io.theme->childGap1,
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(gui.io.theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(10),
            .floating = { .zIndex = gui.get_z_index(), .attachTo = CLAY_ATTACH_TO_PARENT },
            .border = {.color = convert_vec4<Clay_Color>(gui.io.theme->fillColor2), .width = CLAY_BORDER_OUTSIDE(1)}
        }) {
            innerContent(l);
        }
    }, callbacks);
}

void left_to_right_layout(GUIManager& gui, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& innerContent) {
    CLAY_AUTO_ID({.layout = { 
          .sizing = {.width = x, .height = y },
          .childGap = gui.io.theme->childGap1,
          .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
          .layoutDirection = CLAY_LEFT_TO_RIGHT,
    }
    }) {
        innerContent();
    }
}

void left_to_right_line_layout(GUIManager& gui, const std::function<void()>& innerContent) {
    left_to_right_layout(gui, CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0), innerContent);
}

void left_to_right_line_centered_layout(GUIManager& gui, const std::function<void()>& innerContent) {
    CLAY_AUTO_ID({.layout = { 
          .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
          .childGap = gui.io.theme->childGap1,
          .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
          .layoutDirection = CLAY_LEFT_TO_RIGHT,
    }
    }) {
        innerContent();
    }
}

void window_fill_side_bar(GUIManager& gui, const WindowFillSideBarConfig& config, const std::function<void()>& innerContent) {
    switch(config.dir) {
        case WindowFillSideBarConfig::Direction::TOP: {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(config.backgroundColor),
                .border = config.border
            }) {
                CLAY_AUTO_ID({
                    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(gui.io.safeWindowRect.min.y())}}
                }) {}
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    }
                }) {
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(gui.io.safeWindowRect.min.x()), .height = CLAY_SIZING_FIT(0)}}
                    }) {}
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(gui.io.safeWindowRect.width()), .height = CLAY_SIZING_FIT(0)}}
                    }) {
                        innerContent();
                    }
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(gui.io.windowSize.x() - gui.io.safeWindowRect.max.x()), .height = CLAY_SIZING_FIT(0)}}
                    }) {}
                }
            }
            break;
        }
        case WindowFillSideBarConfig::Direction::BOTTOM: {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(config.backgroundColor),
                .border = config.border
            }) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    }
                }) {
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(gui.io.safeWindowRect.min.x()), .height = CLAY_SIZING_FIT(0)}}
                    }) {}
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(gui.io.safeWindowRect.width()), .height = CLAY_SIZING_FIT(0)}}
                    }) {
                        innerContent();
                    }
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(gui.io.windowSize.x() - gui.io.safeWindowRect.max.x()), .height = CLAY_SIZING_FIT(0)}}
                    }) {}
                }
                CLAY_AUTO_ID({
                    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(gui.io.windowSize.y() - gui.io.safeWindowRect.max.y())}}
                }) {}
            }
            break;
        }
        case WindowFillSideBarConfig::Direction::LEFT: {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_GROW(0) },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                },
                .backgroundColor = convert_vec4<Clay_Color>(config.backgroundColor),
                .border = config.border
            }) {
                CLAY_AUTO_ID({
                    .layout = {.sizing = {.width = CLAY_SIZING_FIXED(gui.io.safeWindowRect.min.x()), .height = CLAY_SIZING_GROW(0)}}
                }) {}
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_GROW(0) },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    }
                }) {
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIXED(gui.io.safeWindowRect.min.y())}}
                    }) {}
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIXED(gui.io.safeWindowRect.height())}}
                    }) {
                        innerContent();
                    }
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIXED(gui.io.windowSize.y() - gui.io.safeWindowRect.max.y())}}
                    }) {}
                }
            }
            break;
        }
        case WindowFillSideBarConfig::Direction::RIGHT: {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_GROW(0) },
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                },
                .backgroundColor = convert_vec4<Clay_Color>(config.backgroundColor),
                .border = config.border
            }) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_GROW(0) },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    }
                }) {
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIXED(gui.io.safeWindowRect.min.y())}}
                    }) {}
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIXED(gui.io.safeWindowRect.height())}}
                    }) {
                        innerContent();
                    }
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIXED(gui.io.windowSize.y() - gui.io.safeWindowRect.max.y())}}
                    }) {}
                }
                CLAY_AUTO_ID({
                    .layout = {.sizing = {.width = CLAY_SIZING_FIXED(gui.io.windowSize.x() - gui.io.safeWindowRect.max.x()), .height = CLAY_SIZING_GROW(0)}}
                }) {}
            }
            break;
        }
    }
}

void window_gap_side_bar(GUIManager& gui, const WindowFillSideBarConfig::Direction& dir) {
    switch(dir) {
        case WindowFillSideBarConfig::Direction::TOP:
            CLAY_AUTO_ID({
                .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(gui.io.safeWindowRect.min.y())}}
            }) {}
            break;
        case WindowFillSideBarConfig::Direction::BOTTOM:
            CLAY_AUTO_ID({
                .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(gui.io.windowSize.y() - gui.io.safeWindowRect.max.y())}}
            }) {}
            break;
        case WindowFillSideBarConfig::Direction::LEFT:
            CLAY_AUTO_ID({
                .layout = {.sizing = {.width = CLAY_SIZING_FIT(gui.io.safeWindowRect.min.x()), .height = CLAY_SIZING_GROW(0)}}
            }) {}
            break;
        case WindowFillSideBarConfig::Direction::RIGHT:
            CLAY_AUTO_ID({
                .layout = {.sizing = {.width = CLAY_SIZING_FIT(gui.io.windowSize.x() - gui.io.safeWindowRect.max.x()), .height = CLAY_SIZING_GROW(0)}}
            }) {}
            break;
    }
}

void bottom_offset_setup(GUIManager& gui, float dropdownHeight, Clay_FloatingElementConfig& floatConfig, float& maxHeight, const DropDownPopupLayout& d) {
    auto& bb = d.button->get_bb().value();

    float xOffset = bb.center().x() - d.entrySize.x() * 0.5f;

    if(xOffset < 0.0f)
        xOffset = 0.0f;
    else if(xOffset + d.entrySize.x() > gui.io.safeWindowRect.max.x())
        xOffset = gui.io.safeWindowRect.max.x() - d.entrySize.x();

    floatConfig = {
        .offset = {
            .x = xOffset,
            .y = bb.max.y() + d.dropdownOffset
        },
        .zIndex = gui.get_z_index(),
        .attachPoints = {
            .element = CLAY_ATTACH_POINT_LEFT_TOP,
            .parent = CLAY_ATTACH_POINT_LEFT_TOP
        },
        .attachTo = CLAY_ATTACH_TO_ROOT
    };

    maxHeight = std::max(gui.io.safeWindowRect.max.y() - bb.max.y() - d.dropdownOffset, 0.0f);
}

void top_offset_setup(GUIManager& gui, float dropdownHeight, Clay_FloatingElementConfig& floatConfig, float& maxHeight, const DropDownPopupLayout& d) {
    auto& bb = d.button->get_bb().value();

    float xOffset = bb.center().x() - d.entrySize.x() * 0.5f;

    if(xOffset < 0.0f)
        xOffset = 0.0f;
    else if(xOffset + d.entrySize.x() > gui.io.safeWindowRect.max.x())
        xOffset = gui.io.safeWindowRect.max.x() - d.entrySize.x();

    floatConfig = {
        .offset = {
            .x = xOffset,
            .y = bb.min.y() - d.dropdownOffset
        },
        .zIndex = gui.get_z_index(),
        .attachPoints = {
            .element = CLAY_ATTACH_POINT_LEFT_BOTTOM,
            .parent = CLAY_ATTACH_POINT_LEFT_TOP
        },
        .attachTo = CLAY_ATTACH_TO_ROOT
    };

    maxHeight = std::max(bb.min.y() - gui.io.safeWindowRect.min.y() - d.dropdownOffset, 0.0f);
}

void left_right_offset_setup(GUIManager& gui, float dropdownHeight, Clay_FloatingElementConfig& floatConfig, float& maxHeight, bool isRightSide, const DropDownPopupLayout& d) {
    auto& bb = d.button->get_bb().value();
    float offsetY = bb.center().y() - dropdownHeight * 0.5f;
    if(dropdownHeight >= gui.io.safeWindowRect.height() || offsetY < gui.io.safeWindowRect.min.y())
        offsetY = gui.io.safeWindowRect.min.y();
    else if(offsetY + dropdownHeight > gui.io.safeWindowRect.max.y())
        offsetY -= (offsetY + dropdownHeight) - gui.io.safeWindowRect.max.y();
    floatConfig = {
        .offset = {
            .x = isRightSide ? bb.max.x() + d.dropdownOffset : bb.min.x() - bb.width() - d.dropdownOffset,
            .y = offsetY
        },
        .zIndex = gui.get_z_index(),
        .attachPoints = {
            .element = CLAY_ATTACH_POINT_LEFT_TOP,
            .parent = CLAY_ATTACH_POINT_LEFT_TOP
        },
        .attachTo = CLAY_ATTACH_TO_ROOT
    };
    maxHeight = gui.io.safeWindowRect.height();
}

void dropdown_many_element_popup_layout(GUIManager& gui, const char* id, const DropDownPopupLayout& d) {
    if(!d.button->get_bb().has_value())
        return;
    gui.element<LayoutElement>(id, [&](LayoutElement* l, const Clay_ElementId& lId) {
        float dropdownHeight = d.entrySize.y() * d.entryCount + d.dropdownOffset;
        Clay_FloatingElementConfig floatConfig;
        float maxHeight = gui.io.safeWindowRect.height();
        auto& buttonBB = d.button->get_bb().value();
        if(buttonBB.max.y() + dropdownHeight > gui.io.safeWindowRect.max.y()) {
            float rightSideWidth = gui.io.safeWindowRect.max.x() - buttonBB.max.x();
            float leftSideWidth = buttonBB.min.x() - gui.io.safeWindowRect.min.x();
            if(rightSideWidth > d.entrySize.x() || leftSideWidth > d.entrySize.x())
                left_right_offset_setup(gui, dropdownHeight, floatConfig, maxHeight, rightSideWidth >= leftSideWidth, d);
            else {
                if(gui.io.safeWindowRect.max.y() - buttonBB.max.y() >= buttonBB.min.y() - gui.io.safeWindowRect.min.y())
                    bottom_offset_setup(gui, dropdownHeight, floatConfig, maxHeight, d);
                else
                    top_offset_setup(gui, dropdownHeight, floatConfig, maxHeight, d);
            }
        }
        else
            bottom_offset_setup(gui, dropdownHeight, floatConfig, maxHeight, d);
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIXED(d.entrySize.x()), .height = CLAY_SIZING_FIT(0, maxHeight)},
                .childGap = 0
            },
            .backgroundColor = convert_vec4<Clay_Color>(gui.io.theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(4),
            .floating = floatConfig,
            .border = {
                .color = convert_vec4<Clay_Color>(gui.io.theme->fillColor2),
                .width = CLAY_BORDER_OUTSIDE(1)
            }
        }) {
            gui.element<ManyElementScrollArea>("dropdown scroll area", ManyElementScrollArea::Options{
                .entryHeight = d.entrySize.y(),
                .entryCount = d.entryCount,
                .clipHorizontal = true,
                .elementContent = d.entryLayout
            });
        }
    }, LayoutElement::Callbacks {
        .onClick = [&, d](LayoutElement* l, const InputManager::MouseButtonCallbackArgs& m) {
            if(!l->mouseHovering && !l->childMouseHovering && m.down && !d.button->mouseHovering && d.isOpen) {
                *d.isOpen = false;
                gui.set_to_layout();
            }
        }
    });
}

}}
