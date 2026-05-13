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

#include "ButtonHelpers.hpp"
#include "../Elements/SVGIcon.hpp"
#include "../Elements/LayoutElement.hpp"
#include "Helpers/ConvertVec.hpp"
#include "TextLabelHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

void text_button(GUIManager& gui, const char* id, std::string_view text, const TextButtonOptions& options) {
    SelectableButton::Data d = selectable_button_options_to_data(options);
    d.innerContent = [&gui, text, centered = options.centered] (const SelectableButton::InnerContentCallbackParameters&) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                .childAlignment = {.x = centered ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
            }
        }) {
            text_label(gui, text);
        }
    };
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = options.wide ? CLAY_SIZING_GROW(0) : CLAY_SIZING_FIT(0), .height = options.growHeight ? CLAY_SIZING_GROW(0) : CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = options.centered ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
        }
    }) {
        gui.element<SelectableButton>(id, d);
    }
}

void text_button_with_icon(GUIManager& gui, const char* id, const std::string& svgPath, std::string_view text, const TextButtonOptions& options) {
    gui.new_id(id, [&] {
        SelectableButton::Data d = selectable_button_options_to_data(options);
        d.innerContent = [&gui, svgPath, text, centered = options.centered] (const SelectableButton::InnerContentCallbackParameters&) {
            gui.element<LayoutElement>("layoutelem", [&] (LayoutElement* l, const Clay_ElementId& lId) {
                CLAY(lId, {
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                        .childGap = gui.io.theme->childGap1,
                        .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                    }
                }) {
                    if(l->get_bb().has_value()) {
                        float iconSize = l->get_bb().value().height();
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(iconSize), .height = CLAY_SIZING_FIXED(iconSize)},
                            },
                            .aspectRatio = {.aspectRatio = 1.0f}
                        }) {
                            gui.element<SVGIcon>("icon", svgPath, false);
                        }
                    }
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                            .childAlignment = {.x = centered ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                        }
                    }) {
                        text_label(gui, text);
                    }
                }
            });
        };
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = options.wide ? CLAY_SIZING_GROW(0) : CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
                .childAlignment = {.x = options.centered ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
            }
        }) {
            gui.element<SelectableButton>("button", d);
        }
    });
}

void text_button_sized(GUIManager& gui, const char* id, std::string_view text, Clay_SizingAxis x, Clay_SizingAxis y, const TextButtonOptions& options) {
    SelectableButton::Data d = selectable_button_options_to_data(options);
    d.innerContent = [&gui, text] (const SelectableButton::InnerContentCallbackParameters&) {
        text_label(gui, text);
    };
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = x, .height = y },
            .childAlignment = {.x = options.centered ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
        }
    }) {
        gui.element<SelectableButton>(id, d);
    }
}

SelectableButton* svg_icon_button(GUIManager& gui, const char* id, const std::string& svgPath, const SVGButtonOptions& options) {
    SelectableButton::Data d = selectable_button_options_to_data(options);
    d.innerContent = [&gui, &svgPath, &options] (const SelectableButton::InnerContentCallbackParameters& p) {
        gui.element<SVGIcon>("icon", svgPath, p.isSelected || p.isHovering || p.isHeld, options.drawType == SelectableButton::DrawType::FILLED_INVERSE);
    };
    SelectableButton* toRet;
    CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(options.size), .height = CLAY_SIZING_FIXED(options.size) } } }) {
        toRet = gui.element<SelectableButton>(id, d);
    }
    return toRet;
}

}}
