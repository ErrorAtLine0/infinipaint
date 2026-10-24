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

#include "AboutDisplayGUI.hpp"
#include "../../MainProgram.hpp"
#include "../../GUIStuff/Elements/ScrollArea.hpp"
#include "../../GUIStuff/Elements/LayoutElement.hpp"
#include "../../GUIStuff/Elements/TextParagraph.hpp"
#include "../../GUIStuff/Elements/SelectableButton.hpp"
#include "../../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

using namespace GUIStuff;
using namespace ElementHelpers;

void AboutDisplayGUI::bottom_layout(MainProgram& main) {
    auto& gui = main.g.gui;
    gui.element<LayoutElement>("about display gui bottom bar", [&](LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(40)},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            gui.clipping_element<ScrollArea>("about scroll area", ScrollArea::Options{
                .scrollHorizontal = true,
                .clipHorizontal = true,
                .scrollbarX = ScrollArea::ScrollbarType::NORMAL,
                .innerContent = [&] (auto&) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT
                        },
                    }) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)}
                            }
                        }) {
                            text_button(gui, "infinipaintnoticebutton", "InfiniPaint", {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .isSelected = selectedLicense == -1,
                                .onClick = [&] { selectedLicense = -1; }
                            });
                        }
                        for(int i = 0; i < static_cast<int>(main.conf.thirdPartyLicenses.size()); i++) {
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                    .padding = CLAY_PADDING_ALL(1)
                                }
                            }) {
                                gui.new_id(i, [&] {
                                    text_button(gui, "noticebutton", main.conf.thirdPartyLicenses[i].first, {
                                        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                        .isSelected = selectedLicense == i,
                                        .onClick = [&, i] { selectedLicense = i; }
                                    });
                                });
                            }
                        }
                    }
                }
            });
        }
    });
}

void AboutDisplayGUI::side_layout(MainProgram& main) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    gui.new_id("about display gui Menu Selector", [&] {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(200), .height = CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            gui.clipping_element<ScrollArea>("About Menu Selector Scroll Area", ScrollArea::Options{
                .scrollVertical = true,
                .clipVertical = true,
                .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
                .innerContent = [&](const ScrollArea::InnerContentParameters&) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)}
                        }
                    }) {
                        text_button(gui, "infinipaintnoticebutton", "InfiniPaint", {
                            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                            .isSelected = selectedLicense == -1,
                            .wide = true,
                            .centered = false,
                            .onClick = [&] { selectedLicense = -1; }
                        });
                    }
                    text_label_light_centered(gui, "Third Party Components");
                    for(int i = 0; i < static_cast<int>(main.conf.thirdPartyLicenses.size()); i++) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                .padding = CLAY_PADDING_ALL(1)
                            }
                        }) {
                            gui.new_id(i, [&] {
                                text_button(gui, "noticebutton", main.conf.thirdPartyLicenses[i].first, {
                                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                    .isSelected = selectedLicense == i,
                                    .wide = true,
                                    .centered = false,
                                    .onClick = [&, i] { selectedLicense = i; }
                                });
                            });
                        }
                    }
                }
            });
        }
    });
}

ScrollArea* AboutDisplayGUI::inner_layout(MainProgram& main) {
    auto& gui = main.g.gui;
    return gui.clipping_element<ScrollArea>("about display gui inner scroll area", ScrollArea::Options{
        .scrollVertical = true,
        .clipVertical = true,
        .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
        .innerContent = [&] (const ScrollArea::InnerContentParameters& contentDim) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                },
            }) {
                RichText::TextData licenseText;
                licenseText.paragraphs.emplace_back((selectedLicense == -1) ? main.conf.ownLicenseText : main.conf.thirdPartyLicenses[selectedLicense].second);
                licenseText.tStyleMods.emplace_back();
                licenseText.tStyleMods.back().pos = {0, 0};
                licenseText.tStyleMods.back().mods[RichText::TextStyleModifier::ModifierType::SIZE] = std::make_shared<RichText::SizeTextStyleModifier>(gui.io.fontSize * 0.8f);
                gui.element<TextParagraph>("license text", TextParagraph::Data{
                    .text = licenseText,
                    .maxGrowX = contentDim.containerDimensions.x(),
                    .ellipsis = false
                });
            }
        }
    });
}
