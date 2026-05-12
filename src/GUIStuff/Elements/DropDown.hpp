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

#pragma once
#include "Element.hpp"
#include "Helpers/ConvertVec.hpp"
#include "SelectableButton.hpp"
#include "../ElementHelpers/LayoutHelpers.hpp"
#include "../ElementHelpers/ButtonHelpers.hpp"
#include "TextParagraph.hpp"
#include "SVGIcon.hpp"

namespace GUIStuff {

struct DropdownOptions {
    std::function<void()> onClick;
};

template <typename T> class DropDown : public Element {
    public:
        static constexpr float DROPDOWN_OFFSET = 4.0f;

        DropDown(GUIManager& gui): Element(gui) {}
        void layout(const Clay_ElementId& id, T* data, const std::vector<std::string>& selections, const DropdownOptions& options = {}) {
            using namespace ElementHelpers;
            opts = options;
            d = data;


            CLAY(id, {
                .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)}}
            }) {
                dropdownButton = gui.element<SelectableButton>("dropdown", SelectableButton::Data{
                    .drawType = SelectableButton::DrawType::FILLED,
                    .isSelected = isOpen,
                    .onClick = [&] {
                        isOpen = !isOpen;
                        gui.set_to_layout();
                    },
                    .innerContent = [&](const SelectableButton::InnerContentCallbackParameters&) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                                .padding = {.left = 4, .right = 4},
                                .childGap = gui.io.theme->childGap1,
                                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            }
                        }) {
                            RichText::TextData t;
                            t.paragraphs.emplace_back();
                            t.paragraphs.back().text = selections[static_cast<size_t>(*data)];
                            gui.element<LayoutElement>("dropdown inner text area", [&](LayoutElement* l, const Clay_ElementId& lId) {
                                CLAY(lId, {
                                    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
                                }) {
                                    if(l->get_bb().has_value()) {
                                        gui.element<TextParagraph>("dropdown inner text", TextParagraph::Data{
                                            .text = t,
                                            .maxGrowX = l->get_bb().value().width(),
                                            .maxGrowY = 0.0f,
                                            .ellipsis = true
                                        });
                                    }
                                }
                            });
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_FIT(ElementHelpers::SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIT(ElementHelpers::SMALL_BUTTON_SIZE)}
                                }
                            }) {
                                gui.element<SVGIcon>("dropico", "data/icons/droparrow.svg", isOpen);
                            }
                        }
                    }
                });

                if(isOpen && boundingBox.has_value()) {
                    gui.set_z_index(gui.get_z_index() + 1, [&] {
                        dropdown_many_element_popup_layout(gui, "DROPDOWN", {
                            .button = this,
                            .clickAwayCallback = [&] { isOpen = false; },
                            .entrySize = boundingBox.value().dim(),
                            .entryCount = selections.size(),
                            .entryLayout = [&](size_t i) {
                                text_transparent_option_button("option", selections[i].c_str(), [&, i] {
                                    *d = static_cast<T>(i);
                                    if(opts.onClick) opts.onClick();
                                    isOpen = false;
                                });
                            }
                        });
                    });
                }
            }
        }
    private:
        void text_transparent_option_button(const char* id, const char* text, const std::function<void()>& onClick) {
            using namespace ElementHelpers;
            text_button(gui, id, text, {
                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                .wide = true,
                .centered = false,
                .onClick = onClick
            });
        }
        T* d;
        SelectableButton* dropdownButton = nullptr;
        DropdownOptions opts;
        bool isOpen = false;
};

}
