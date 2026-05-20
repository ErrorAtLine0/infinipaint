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

#include "FontPicker.hpp"
#include "Helpers/ConvertVec.hpp"
#include <algorithm>
#include <limits>
#include <modules/skparagraph/include/DartTypes.h>
#include <modules/skparagraph/include/ParagraphStyle.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include <Helpers/Random.hpp>
#include "../GUIManager.hpp"

#include "../ElementHelpers/ButtonHelpers.hpp"
#include "../ElementHelpers/TextBoxHelpers.hpp"
#include "TextParagraph.hpp"
#include "ManyElementScrollArea.hpp"

namespace GUIStuff {

std::vector<std::string> sortedFontListLowercase;
std::vector<std::string> sortedFontList;

const std::vector<std::string>& FontPicker::sorted_font_list(GUIManager& gui) {
    if(sortedFontList.empty()) {
        auto icu = SkUnicodes::ICU::Make();
        std::set<std::string> sortedFontSet;

        if(gui.io.fonts->localFontMgr) {
            for(int i = 0; i < gui.io.fonts->localFontMgr->countFamilies(); i++) {
                SkString familyNameSkString;
                gui.io.fonts->localFontMgr->getFamilyName(i, &familyNameSkString);
                if(familyNameSkString.isEmpty())
                    continue;
                std::string familyName(familyNameSkString.c_str(), familyNameSkString.size());
                sortedFontSet.emplace(familyName);
            }
        }
        for(int i = 0; i < gui.io.fonts->defaultFontMgr->countFamilies(); i++) {
            SkString familyNameSkString;
            gui.io.fonts->defaultFontMgr->getFamilyName(i, &familyNameSkString);
            if(familyNameSkString.isEmpty())
                continue;
            std::string familyName(familyNameSkString.c_str(), familyNameSkString.size());
            sortedFontSet.emplace(familyName);
        }
        sortedFontList = std::vector<std::string>(sortedFontSet.begin(), sortedFontSet.end());
        sortedFontListLowercase = sortedFontList;
        for(std::string& s : sortedFontListLowercase)
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    }
    return sortedFontList;
}

FontPicker::FontPicker(GUIManager& gui): Element(gui) {}

void FontPicker::layout(const Clay_ElementId& id, std::string* newFontName, const FontPickerData& newData) {
    using namespace ElementHelpers;
    fontName = newFontName;
    data = newData;

    sorted_font_list(gui);

    auto sortedFontListValIt = std::find(sortedFontList.begin(), sortedFontList.end(), *fontName);
    size_t val = sortedFontListValIt != sortedFontList.end() ? sortedFontListValIt - sortedFontList.begin() : std::numeric_limits<size_t>::max();

    const float ENTRY_HEIGHT = 25.0f;

    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_FIT(0)},
            .childGap = 4
        }
    }) {
        auto textboxElement = ElementHelpers::input_text(gui, "Font textbox", fontName, {
            .onEdit = [&] {
                auto newFontName = get_valid_font_name();
                if(newFontName.has_value()) {
                    *fontName = newFontName.value();
                    jumpToFontName = true;
                    if(data.onFontChange) data.onFontChange();
                }
                gui.set_to_layout();
            },
            .onSelect = [&] {
                dropdownOpen = true;
                gui.set_to_layout();
            }
        });

        auto dropdownButton = svg_icon_button(gui, "Open font dropdown", "data/icons/droparrowbold.svg", {
            .size = SMALL_BUTTON_SIZE,
            .onClick = [&] {
                dropdownOpen = !dropdownOpen;
            }
        });

        if(dropdownOpen) {
            gui.set_z_index(gui.get_z_index() + 1, [&] {
                gui.element<LayoutElement>("DROPDOWN", [&](LayoutElement*, const Clay_ElementId& lId) {
                    Clay_ElementData dropdownElemData = Clay_GetElementData(lId);
                    float calculatedDropdownMaxHeight = 0.0f;
                    if(dropdownElemData.found) {
                        calculatedDropdownMaxHeight = std::max(gui.io.windowSize.y() - dropdownElemData.boundingBox.y - 2.0f, 0.0f);
                        calculatedDropdownMaxHeight = std::min<float>(calculatedDropdownMaxHeight, 300);
                    }
                    CLAY(lId, {
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_FIT(0, calculatedDropdownMaxHeight)},
                            .childGap = 0
                        },
                        .backgroundColor = convert_vec4<Clay_Color>(gui.io.theme->backColor1),
                        .cornerRadius = CLAY_CORNER_RADIUS(4),
                        .floating = {
                            .offset = {
                                .y = 4
                            },
                            .zIndex = gui.get_z_index(),
                            .attachPoints = {
                                .element = CLAY_ATTACH_POINT_LEFT_TOP,
                                .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM
                            },
                            .attachTo = CLAY_ATTACH_TO_PARENT
                        },
                        .border = {
                            .color = convert_vec4<Clay_Color>(gui.io.theme->fillColor2),
                            .width = CLAY_BORDER_OUTSIDE(1)
                        }
                    }) {
                        gui.element<ManyElementScrollArea>("dropdown scroll area", ManyElementScrollArea::Options{
                            .entryHeight = ENTRY_HEIGHT,
                            .entryCount = sortedFontList.size(),
                            .clipHorizontal = true,
                            .elementContent = [&](size_t i) {
                                bool selectedEntry = val == i;
                                gui.element<LayoutElement>("elem", [&] (LayoutElement*, const Clay_ElementId& lId) {
                                    CLAY(lId, {
                                        .layout = {
                                            .sizing = {.width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_GROW(0)},
                                            .childGap = 1,
                                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                                        }
                                    }) {
                                        SkColor4f entryColor;
                                        if(selectedEntry)
                                            entryColor = gui.io.theme->backColor2;
                                        else if(hoveringOver == i)
                                            entryColor = gui.io.theme->backColor2;
                                        else
                                            entryColor = gui.io.theme->backColor1;
                                        CLAY_AUTO_ID({
                                            .layout = {
                                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                                .padding = {.left = 2, .right = 2},
                                                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                                            },
                                            .backgroundColor = convert_vec4<Clay_Color>(entryColor)
                                        }) {
                                            TextParagraph::Data d;
                                            RichText::TextData::Paragraph& par = d.text.paragraphs.emplace_back();
                                            par.text = sortedFontList[i];

                                            d.allowNewlines = false;
                                            d.ellipsis = false;

                                            RichText::PositionedTextStyleMod& positionedMod = d.text.tStyleMods.emplace_back();
                                            positionedMod.pos = {0, 0};
                                            positionedMod.mods[RichText::TextStyleModifier::ModifierType::FONT_FAMILIES] = std::make_shared<RichText::FontFamiliesTextStyleModifier>(std::vector<SkString>{SkString{sortedFontList[i].c_str(), sortedFontList[i].size()}});

                                            gui.element<TextParagraph>("font name", d);
                                        }
                                    }
                                }, LayoutElement::Callbacks{
                                    .mouseButton = [&, i](LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                                        if(l->mouseHovering && button.down && button.button == InputManager::MouseButton::LEFT) {
                                            *fontName = sortedFontList[i];
                                            gui.set_post_callback_func([&] { if(data.onFontChange) data.onFontChange(); });
                                            gui.set_to_layout();
                                        }
                                    },
                                    .mouseMotion = [&, i](LayoutElement* l, const InputManager::MouseMotionCallbackArgs& motion) {
                                        size_t oldHovering = hoveringOver;
                                        if(l->mouseHovering)
                                            hoveringOver = i;
                                        else if(hoveringOver == i)
                                            hoveringOver = std::numeric_limits<size_t>::max();
                                        if(oldHovering != hoveringOver)
                                            gui.set_to_layout();
                                    }
                                });
                            }
                        });
                    }
                }, LayoutElement::Callbacks{
                    .onClick = [&, textboxElement, dropdownButton] (LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                        if(!textboxElement->mouseHovering && !dropdownButton->mouseHovering && !l->mouseHovering && !l->childMouseHovering) {
                            dropdownOpen = false;
                            gui.set_to_layout();
                        }
                    }
                });
            });
        }
    }
}

std::optional<std::string> FontPicker::get_valid_font_name() {
    auto it = std::find(sortedFontList.begin(), sortedFontList.end(), *fontName);
    if(it != sortedFontList.end())
        return *fontName;
    else {
        std::string lowerFamilyName = *fontName;
        std::transform(lowerFamilyName.begin(), lowerFamilyName.end(), lowerFamilyName.begin(), ::tolower);
        auto itLower = std::find(sortedFontListLowercase.begin(), sortedFontListLowercase.end(), lowerFamilyName);
        if(itLower != sortedFontListLowercase.end()) {
            size_t val = itLower - sortedFontListLowercase.begin();
            return sortedFontList[val];
        }
    }
    return std::nullopt;
}

}
