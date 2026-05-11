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

#include "TextBoxEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../MainProgram.hpp"
#include "../../../DrawData.hpp"
#include "Helpers/SCollision.hpp"
#include <cereal/types/vector.hpp>
#include <include/core/SkFontStyle.h>
#include <modules/skparagraph/include/DartTypes.h>
#include <modules/skparagraph/include/ParagraphStyle.h>
#include <modules/skparagraph/include/TextStyle.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include <memory>
#include "../EditTool.hpp"

#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/DropdownHelpers.hpp"
#include "../../../GUIStuff/Elements/FontPicker.hpp"
#include "../../../GUIStuff/Elements/LayoutElement.hpp"
#include "../../../GUIStuff/Elements/ScrollArea.hpp"

using namespace RichText;

TextBoxEditTool::TextBoxEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp):
    DrawingProgramEditToolBase(initDrawP, initComp)
{}

void TextBoxEditTool::commit_update_func_no_android_update() {
    comp->obj->commit_update(drawP);
    if(userInput)
        userInput->android_force_update_modmap();
}

void TextBoxEditTool::commit_update_and_layout_func_no_android_update() {
    comp->obj->commit_update(drawP);
    drawP.world.main.g.gui.set_to_layout();
    // modmap update is required at the very least in response to input from the C++ side
    if(userInput)
        userInput->android_force_update_modmap();
}

void TextBoxEditTool::commit_update_func_and_android_update() {
    comp->obj->commit_update(drawP);
    if(userInput)
        userInput->android_force_update_textbox_and_cursor();
}

void TextBoxEditTool::commit_update_and_layout_func_and_android_update() {
    comp->obj->commit_update(drawP);
    drawP.world.main.g.gui.set_to_layout();
    if(userInput)
        userInput->android_force_update_textbox_and_cursor();
}

void TextBoxEditTool::edit_gui(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
    auto& gui = drawP.world.main.g.gui;
    auto& currentMods = *currentModsPtr;

    gui.new_id("edit tool text", [&] {
        text_label_centered(gui, "Edit Text");

        left_to_right_line_layout(gui, [&] {
            text_label(gui, "Font");
            gui.element<FontPicker>("Font picker", &newFontName, FontPickerData{
                .onFontChange = [&] {
                    currentMods[TextStyleModifier::ModifierType::FONT_FAMILIES] = std::make_shared<FontFamiliesTextStyleModifier>(std::vector<SkString>{SkString{newFontName.c_str(), newFontName.size()}});
                    add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::FONT_FAMILIES]);});
                    commit_update_func_and_android_update();
                }
            });
        });
        
        left_to_right_line_centered_layout(gui, [&] {
            svg_icon_button(gui, "Bold button", "data/icons/RemixIcon/bold.svg", {
                .isSelected = newIsBold,
                .onClick = [&] {
                    newIsBold = !newIsBold;
                    currentMods[TextStyleModifier::ModifierType::WEIGHT] = std::make_shared<WeightTextStyleModifier>(newIsBold ? SkFontStyle::Weight::kBold_Weight : SkFontStyle::Weight::kNormal_Weight);
                    add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::WEIGHT]);});
                    commit_update_func_and_android_update();
                }
            });

            svg_icon_button(gui, "Italic button", "data/icons/RemixIcon/italic.svg", {
                .isSelected = newIsItalic,
                .onClick = [&] {
                    newIsItalic = !newIsItalic;
                    currentMods[TextStyleModifier::ModifierType::SLANT] = std::make_shared<SlantTextStyleModifier>(newIsItalic ? SkFontStyle::Slant::kItalic_Slant : SkFontStyle::Slant::kUpright_Slant);
                    add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::SLANT]);});
                    commit_update_func_and_android_update();
                }
            });

            auto decorationChange = [&] {
                currentMods[TextStyleModifier::ModifierType::DECORATION] = std::make_shared<DecorationTextStyleModifier>(get_new_decoration_value());
                add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::DECORATION]);});
                commit_update_func_and_android_update();
            };

            svg_icon_button(gui, "Underline button", "data/icons/RemixIcon/underline.svg", {
                .isSelected = newIsUnderlined,
                .onClick = [&, decorationChange] {
                    newIsUnderlined = !newIsUnderlined;
                    decorationChange();
                }
            });

            svg_icon_button(gui, "Strikethrough button", "data/icons/RemixIcon/strikethrough.svg", {
                .isSelected = newIsLinethrough,
                .onClick = [&, decorationChange] {
                    newIsLinethrough = !newIsLinethrough;
                    decorationChange();
                }
            });

            svg_icon_button(gui, "Overline button", "data/icons/RemixIcon/overline.svg", {
                .isSelected = newIsOverline,
                .onClick = [&, decorationChange] {
                    newIsOverline = !newIsOverline;
                    decorationChange();
                }
            });
        });

        left_to_right_line_centered_layout(gui, [&]() {
            auto paragraph_operation_button = [&](const char* id, const char* svgPath, bool isSelected, const std::function<void()>& func) {
                svg_icon_button(gui, id, svgPath, {
                    .isSelected = isSelected,
                    .onClick = [&, func] {
                        add_undo(func);
                        commit_update_func_and_android_update();
                    }
                });
            };

            paragraph_operation_button("Align left button", "data/icons/RemixIcon/align-left.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kLeft, [&] {
                currentPStyle.textAlignment = skia::textlayout::TextAlign::kLeft;
                a.textBox->set_text_alignment_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kLeft);
            });
            paragraph_operation_button("Align center button", "data/icons/RemixIcon/align-center.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kCenter, [&] {
                currentPStyle.textAlignment = skia::textlayout::TextAlign::kCenter;
                a.textBox->set_text_alignment_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kCenter);
            });
            paragraph_operation_button("Align right button", "data/icons/RemixIcon/align-right.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kRight, [&] {
                currentPStyle.textAlignment = skia::textlayout::TextAlign::kRight;
                a.textBox->set_text_alignment_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kRight);
            });
            paragraph_operation_button("Align justify button", "data/icons/RemixIcon/align-justify.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kJustify, [&] {
                currentPStyle.textAlignment = skia::textlayout::TextAlign::kJustify;
                a.textBox->set_text_alignment_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kJustify);
            });
            paragraph_operation_button("Text direction left button", "data/icons/RemixIcon/text-direction-l.svg", currentPStyle.textDirection == skia::textlayout::TextDirection::kLtr, [&] {
                currentPStyle.textDirection = skia::textlayout::TextDirection::kLtr;
                a.textBox->set_text_direction_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextDirection::kLtr);
            });
            paragraph_operation_button("Text direction right button", "data/icons/RemixIcon/text-direction-r.svg", currentPStyle.textDirection == skia::textlayout::TextDirection::kRtl, [&] {
                currentPStyle.textDirection = skia::textlayout::TextDirection::kRtl;
                a.textBox->set_text_direction_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextDirection::kRtl);
            });
        });

        slider_scalar_field<uint32_t>(gui, "Font Size Slider", "Font Size", &newFontSize, 3, 100, TextBoxScalarOptions{
            .onEdit = [&] {
                currentMods[TextStyleModifier::ModifierType::SIZE] = std::make_shared<SizeTextStyleModifier>(newFontSize);
                a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::SIZE]);
                commit_update_func_and_android_update();
            },
            .onSelect = [&] { hold_undo_data("Font Size", a); },
            .onDeselect = [&] { release_undo_data("Font Size"); }
        });

        left_to_right_line_layout(gui, [&]() {
            t.color_button_right("Text Color", &newTextColor, {
                .onChange = [&] {
                    currentMods[TextStyleModifier::ModifierType::COLOR] = std::make_shared<ColorTextStyleModifier>(newTextColor);
                    a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::COLOR]);
                    commit_update_func_and_android_update();
                },
                .onSelect = [&] { hold_undo_data("Text Color", a); },
                .onDeselect = [&] { release_undo_data("Text Color"); }
            });
            text_label(gui, "Text Color");
        });

        left_to_right_line_layout(gui, [&]() {
            t.color_button_right("Highlight Color", &newHighlightColor, {
                .onSelectorButtonClick = [&] {
                    if(newHighlightColor.w() == 0.0f) {
                        newHighlightColor.w() = 1.0f;
                        currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR] = std::make_shared<HighlightColorTextStyleModifier>(newHighlightColor);
                        add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR]);});
                        commit_update_and_layout_func_and_android_update();
                    }
                },
                .onChange = [&] {
                    currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR] = std::make_shared<HighlightColorTextStyleModifier>(newHighlightColor);
                    a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR]);
                    commit_update_and_layout_func_and_android_update();
                },
                .onSelect = [&] { hold_undo_data("Highlight Color", a); },
                .onDeselect = [&] { release_undo_data("Highlight Color"); }
            });
            if(newHighlightColor.w() != 0.0f) {
                svg_icon_button(gui, "Remove Highlight Color", "data/icons/close.svg", {
                    .onClick = [&] {
                        newHighlightColor = {0.0f, 0.0f, 0.0f, 0.0f};
                        currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR] = std::make_shared<HighlightColorTextStyleModifier>(newHighlightColor);
                        add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR]);});
                        commit_update_and_layout_func_and_android_update();
                    }
                });
            }
            text_label(gui, "Highlight Color");
        });
    });
}

Vector4f* TextBoxEditTool::color_picker_color(Vector4f* oldColor) {
    if(oldColor == &newTextColor)
        return oldColor;
    if(oldColor == &newHighlightColor)
        return oldColor;
    return nullptr;
}

bool TextBoxEditTool::phone_gui_tool_specific_bottom_toolbar_exists() {
    return true;
}

void TextBoxEditTool::phone_gui_tool_specific_bottom_toolbar(PhoneDrawingProgramScreen& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        }
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            switch(phoneGUIData.popupType) {
                case PhoneGUIData::PopupType::NONE:
                    break;
                case PhoneGUIData::PopupType::TEXT_COLOR: {
                    if(t.colorPickerPopupData.screenType == PhoneDrawingProgramScreen::ColorPickerPopupData::ScreenType::NORMAL) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                            }
                        }) {}
                    }
                    t.color_settings_popup(&newTextColor, phoneGUIData.colorSelectorData, nullptr);
                    break;
                }
                case PhoneGUIData::PopupType::HIGHLIGHT_COLOR: {
                    if(t.colorPickerPopupData.screenType == PhoneDrawingProgramScreen::ColorPickerPopupData::ScreenType::NORMAL) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                            }
                        }) {}
                    }
                    t.color_settings_popup(&newHighlightColor, phoneGUIData.colorSelectorData, nullptr, true, true);
                    break;
                }
            }
        }
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            gui.element<LayoutElement>("bottom toolbar", [&](LayoutElement*, const Clay_ElementId& lId) {
                CLAY(lId, {
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)}
                    },
                    .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                    .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
                }) {
                    gui.clipping_element<ScrollArea>("tools scroll", ScrollArea::Options{
                        .scrollHorizontal = true,
                        .clipHorizontal = true,
                        .scrollbarX = ScrollArea::ScrollbarType::NONE,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .xAlign = CLAY_ALIGN_X_LEFT,
                        .yAlign = CLAY_ALIGN_Y_CENTER,
                        .innerContent = [&](auto&) {
                            phone_bottom_toolbar_gui(t);
                        }
                    });
                }
            });
        }
    }
}

void TextBoxEditTool::phone_bottom_toolbar_gui(PhoneDrawingProgramScreen& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;
    auto& gui = drawP.world.main.g.gui;
    auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
    auto& currentMods = *currentModsPtr;

    color_button(gui, "Text Color", &newTextColor, {
        .isSelected = phoneGUIData.popupType == PhoneGUIData::PopupType::TEXT_COLOR,
        .onClick = [&] {
            t.reset_color_picker_popup_data();
            if(phoneGUIData.popupType == PhoneGUIData::PopupType::TEXT_COLOR)
                phoneGUIData.popupType = PhoneGUIData::PopupType::NONE;
            else
                phoneGUIData.popupType = PhoneGUIData::PopupType::TEXT_COLOR;
            phoneGUIData.colorSelectorData = {
                .onChange = [&] {
                    currentMods[TextStyleModifier::ModifierType::COLOR] = std::make_shared<ColorTextStyleModifier>(newTextColor);
                    a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::COLOR]);
                    commit_update_func_and_android_update();
                },
                .onSelect = [&] { hold_undo_data("Text Color", a); },
                .onDeselect = [&] { release_undo_data("Text Color"); }
            };
        }
    });

    color_button(gui, "Highlight Color", &newHighlightColor, {
        .isSelected = phoneGUIData.popupType == PhoneGUIData::PopupType::HIGHLIGHT_COLOR,
        .onClick = [&] {
            t.reset_color_picker_popup_data();
            if(phoneGUIData.popupType == PhoneGUIData::PopupType::HIGHLIGHT_COLOR)
                phoneGUIData.popupType = PhoneGUIData::PopupType::NONE;
            else
                phoneGUIData.popupType = PhoneGUIData::PopupType::HIGHLIGHT_COLOR;
            phoneGUIData.colorSelectorData = {
                .onChange = [&] {
                    currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR] = std::make_shared<HighlightColorTextStyleModifier>(newHighlightColor);
                    a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR]);
                    commit_update_and_layout_func_and_android_update();
                },
                .onSelect = [&] { hold_undo_data("Highlight Color", a); },
                .onDeselect = [&] { release_undo_data("Highlight Color"); }
            };
        }
    });

    CLAY_AUTO_ID({
        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(80)}}
    }) {
        number_dropdown(gui, "Font size", &newFontSize, 2, 80, 2, [&] {
            currentMods[TextStyleModifier::ModifierType::SIZE] = std::make_shared<SizeTextStyleModifier>(newFontSize);
            add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::SIZE]);});
            commit_update_func_and_android_update();
        });
    }

    Element* fontPickerButton = svg_icon_button(gui, "Font Picker button", "data/icons/RemixIcon/font-serif.svg", {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = fontPickerIsOpen,
        .onClick = [&] {
            fontPickerIsOpen = !fontPickerIsOpen;
        }
    });

    if(fontPickerIsOpen) {
        gui.set_z_index(gui.get_z_index() + 1, [&] {
            dropdown_many_element_popup_layout(gui, "Font Picker Popup", DropDownPopupLayout{
                .button = fontPickerButton,
                .isOpen = &fontPickerIsOpen,
                .entrySize = {200.0f, BIG_BUTTON_SIZE},
                .entryCount = FontPicker::sorted_font_list(gui).size(),
                .entryLayout = [&](size_t i) {
                    gui.element<SelectableButton>("font button", SelectableButton::Data{
                        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                        .isSelected = newFontName == FontPicker::sorted_font_list(gui)[i],
                        .onClick = [&, i] {
                            newFontName = FontPicker::sorted_font_list(gui)[i];
                            currentMods[TextStyleModifier::ModifierType::FONT_FAMILIES] = std::make_shared<FontFamiliesTextStyleModifier>(std::vector<SkString>{SkString{newFontName.c_str(), newFontName.size()}});
                            add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::FONT_FAMILIES]);});
                            commit_update_func_and_android_update();
                            fontPickerIsOpen = false;
                        },
                        .innerContent = [&, i] (const SelectableButton::InnerContentCallbackParameters&) {
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                                }
                            }) {
                                TextParagraph::Data d;
                                RichText::TextData::Paragraph& par = d.text.paragraphs.emplace_back();
                                par.text = FontPicker::sorted_font_list(gui)[i];

                                d.allowNewlines = false;
                                d.ellipsis = false;

                                RichText::PositionedTextStyleMod& positionedMod = d.text.tStyleMods.emplace_back();
                                positionedMod.pos = {0, 0};
                                positionedMod.mods[RichText::TextStyleModifier::ModifierType::FONT_FAMILIES] = std::make_shared<RichText::FontFamiliesTextStyleModifier>(std::vector<SkString>{SkString{FontPicker::sorted_font_list(gui)[i].c_str(), FontPicker::sorted_font_list(gui)[i].size()}});

                                gui.element<TextParagraph>("font name", d);
                            }
                        }
                    });
                }
            });
        });
    }

    svg_icon_button(gui, "Bold button", "data/icons/RemixIcon/bold.svg", {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = newIsBold,
        .onClick = [&] {
            newIsBold = !newIsBold;
            currentMods[TextStyleModifier::ModifierType::WEIGHT] = std::make_shared<WeightTextStyleModifier>(newIsBold ? SkFontStyle::Weight::kBold_Weight : SkFontStyle::Weight::kNormal_Weight);
            add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::WEIGHT]);});
            commit_update_func_and_android_update();
        }
    });

    svg_icon_button(gui, "Italic button", "data/icons/RemixIcon/italic.svg", {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = newIsItalic,
        .onClick = [&] {
            newIsItalic = !newIsItalic;
            currentMods[TextStyleModifier::ModifierType::SLANT] = std::make_shared<SlantTextStyleModifier>(newIsItalic ? SkFontStyle::Slant::kItalic_Slant : SkFontStyle::Slant::kUpright_Slant);
            add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::SLANT]);});
            commit_update_func_and_android_update();
        }
    });

    auto decorationChange = [&] {
        currentMods[TextStyleModifier::ModifierType::DECORATION] = std::make_shared<DecorationTextStyleModifier>(get_new_decoration_value());
        add_undo_if_selecting_area(a, [&]() {a.textBox->set_text_style_modifier_between(a.cursor->selectionBeginPos, a.cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::DECORATION]);});
        commit_update_func_and_android_update();
    };

    svg_icon_button(gui, "Underline button", "data/icons/RemixIcon/underline.svg", {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = newIsUnderlined,
        .onClick = [&, decorationChange] {
            newIsUnderlined = !newIsUnderlined;
            decorationChange();
        }
    });

    svg_icon_button(gui, "Strikethrough button", "data/icons/RemixIcon/strikethrough.svg", {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = newIsLinethrough,
        .onClick = [&, decorationChange] {
            newIsLinethrough = !newIsLinethrough;
            decorationChange();
        }
    });

    svg_icon_button(gui, "Overline button", "data/icons/RemixIcon/overline.svg", {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = newIsOverline,
        .onClick = [&, decorationChange] {
            newIsOverline = !newIsOverline;
            decorationChange();
        }
    });

    auto paragraph_operation_button = [&](const char* id, const char* svgPath, bool isSelected, const std::function<void()>& func) {
        svg_icon_button(gui, id, svgPath, {
            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
            .isSelected = isSelected,
            .onClick = [&, func] {
                add_undo(func);
                commit_update_func_and_android_update();
            }
        });
    };

    paragraph_operation_button("Align left button", "data/icons/RemixIcon/align-left.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kLeft, [&] {
        currentPStyle.textAlignment = skia::textlayout::TextAlign::kLeft;
        a.textBox->set_text_alignment_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kLeft);
    });
    paragraph_operation_button("Align center button", "data/icons/RemixIcon/align-center.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kCenter, [&] {
        currentPStyle.textAlignment = skia::textlayout::TextAlign::kCenter;
        a.textBox->set_text_alignment_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kCenter);
    });
    paragraph_operation_button("Align right button", "data/icons/RemixIcon/align-right.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kRight, [&] {
        currentPStyle.textAlignment = skia::textlayout::TextAlign::kRight;
        a.textBox->set_text_alignment_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kRight);
    });
    paragraph_operation_button("Align justify button", "data/icons/RemixIcon/align-justify.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kJustify, [&] {
        currentPStyle.textAlignment = skia::textlayout::TextAlign::kJustify;
        a.textBox->set_text_alignment_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kJustify);
    });
    paragraph_operation_button("Text direction left button", "data/icons/RemixIcon/text-direction-l.svg", currentPStyle.textDirection == skia::textlayout::TextDirection::kLtr, [&] {
        currentPStyle.textDirection = skia::textlayout::TextDirection::kLtr;
        a.textBox->set_text_direction_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextDirection::kLtr);
    });
    paragraph_operation_button("Text direction right button", "data/icons/RemixIcon/text-direction-r.svg", currentPStyle.textDirection == skia::textlayout::TextDirection::kRtl, [&] {
        currentPStyle.textDirection = skia::textlayout::TextDirection::kRtl;
        a.textBox->set_text_direction_between(a.cursor->selectionBeginPos.fParagraphIndex, a.cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextDirection::kRtl);
    });
}

void TextBoxEditTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
}

void TextBoxEditTool::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    if(userInput && userInput->id == drawP.world.main.input.currentTextboxID.value()) {
        auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
        if(userInput->input_paste_callback(paste)) {
            set_styles_at_selection(a);
            commit_update_and_layout_func_no_android_update();
        }
    }
}

void TextBoxEditTool::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {
    if(userInput && userInput->id == drawP.world.main.input.currentTextboxID.value()) {
        auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
        auto changes = userInput->input_android_text_box_input_callback(textboxInput);
        if(changes.textEdited || changes.cursorChanged) {
            set_styles_at_selection(a);
            commit_update_and_layout_func_no_android_update();
        }
    }
}

void TextBoxEditTool::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(userInput && userInput->id == drawP.world.main.input.currentTextboxID.value()) {
        auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
        auto changes = userInput->input_key_callback(drawP.world.main.input, key);
        if(changes.textEdited || changes.cursorChanged) {
            set_styles_at_selection(a);
            commit_update_and_layout_func_no_android_update();
        }
    }
}

void TextBoxEditTool::input_text_callback(const InputManager::TextCallbackArgs& text) {
    if(userInput && userInput->id == drawP.world.main.input.currentTextboxID.value()) {
        auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
        userInput->add_text_to_textbox(text.str);
        set_styles_at_selection(a);
        commit_update_and_layout_func_no_android_update();
    }
}

void TextBoxEditTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint) {
    if(button.button == InputManager::MouseButton::LEFT && !isDraggingPoint && userInput) {
        auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());

        SCollision::ColliderCollection<float> mousePointCollection;
        mousePointCollection.circle.emplace_back(button.pos, 1.0f);
        mousePointCollection.recalculate_bounds();

        InputManager& input = drawP.world.main.input;

        bool collidesWithBox = comp->obj->collides_with_cam_coords(drawP.world.drawData.cam.c, mousePointCollection);

        auto oldCursor = *a.cursor;

        if(button.deviceType == InputManager::MouseDeviceType::TOUCH) {
            if(button.down)
                userInput->input_finger_touch_down(a.get_mouse_pos(drawP));
        }
        else
            userInput->process_mouse_left_button(a.get_mouse_pos(drawP), (button.down && button.clicks && collidesWithBox) ? button.clicks : 0, button.down, input.key(InputManager::KEY_GENERIC_LSHIFT).held);

        if(oldCursor != *a.cursor) {
            set_styles_at_selection(a);
            commit_update_and_layout_func_no_android_update();
        }
    }
}

void TextBoxEditTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint) {
    auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
    a.init_text_box(drawP);

    if(!isDraggingPoint && userInput) {
        auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());

        SCollision::ColliderCollection<float> mousePointCollection;
        mousePointCollection.circle.emplace_back(motion.pos, 1.0f);
        mousePointCollection.recalculate_bounds();

        InputManager& input = drawP.world.main.input;

        auto oldCursor = *a.cursor;

        if(motion.deviceType == InputManager::MouseDeviceType::TOUCH) {
            if(drawP.controls.leftClickHeld)
                userInput->input_finger_held_motion(a.get_mouse_pos(drawP));
        }
        else
            userInput->process_mouse_left_button(a.get_mouse_pos(drawP), 0, drawP.controls.leftClickHeld, input.key(InputManager::KEY_GENERIC_LSHIFT).held);

        if(oldCursor != *a.cursor) {
            set_styles_at_selection(a);
            commit_update_and_layout_func_no_android_update();
        }
    }
}

std::optional<InputManager::TextBoxStartInfo> TextBoxEditTool::get_text_box_start_info() {
    if(userInput) {
        auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
        InputManager::TextInputProperties tProps {
            .inputType = SDL_TextInputType::SDL_TEXTINPUT_TYPE_TEXT,
            .capitalization = SDL_Capitalization::SDL_CAPITALIZE_NONE,
            .autocorrect = true,
            .multiline = true,
            .androidInputType = InputManager::AndroidInputType::ANDROIDTEXT_TYPE_CLASS_TEXT
        };

        return InputManager::TextBoxStartInfo {
            .id = userInput->id,
            .rect = std::nullopt,
            .inputProperties = tProps,
            .textBox = a.textBox,
            .cursor = a.cursor,
            .modMap = currentModsPtr
        };
    }
    return std::nullopt;
}

void TextBoxEditTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;
    auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());

    drawP.right_click_action_menu(popupPos, [&] {
        text_label_light(gui, "Text menu");
        InputManager& input = drawP.world.main.input;
        drawP.popup_menu_action_button("Paste", "Paste", [&] {
            drawP.world.main.input.call_paste(CustomEvents::PasteEvent::DataType::TEXT, { .allowRichText = true });
        });
        drawP.popup_menu_action_button("Paste without formatting", "Paste without formatting", [&] {
            drawP.world.main.input.call_paste(CustomEvents::PasteEvent::DataType::TEXT, { .allowRichText = false });
        });
        if(a.cursor->selectionBeginPos != a.cursor->selectionEndPos) {
            drawP.popup_menu_action_button("Copy", "Copy", [&] {
                input.set_clipboard_plain_and_richtext_pair(a.textBox->process_copy(*a.cursor));
            });
            drawP.popup_menu_action_button("Cut", "Cut", [&] {
                userInput->do_textbox_operation_with_undo([&]() {
                    input.set_clipboard_plain_and_richtext_pair(a.textBox->process_cut(*a.cursor));
                });
                set_styles_at_selection(a);
                commit_update_and_layout_func_and_android_update();
            });
        }
    });
}

void TextBoxEditTool::hold_undo_data(const std::string& undoName, TextBoxCanvasComponent& a) {
    auto it = undoHeldData.find(undoName);
    if(it != undoHeldData.end()) {
        std::pair<RichText::TextBox::Cursor, RichText::TextData>& undoData = it->second;
        if(undoData.first != *a.cursor)
            undoHeldData.erase(it);
    }
    else if(a.cursor->selectionBeginPos != a.cursor->selectionEndPos)
        undoHeldData.emplace(undoName, std::pair<RichText::TextBox::Cursor, RichText::TextData>{*a.cursor, a.textBox->get_rich_text_data()});
}

void TextBoxEditTool::release_undo_data(const std::string& undoName) {
    auto it = undoHeldData.find(undoName);
    if(it != undoHeldData.end()) {
        std::pair<RichText::TextBox::Cursor, RichText::TextData>& undoData = it->second;
        userInput->add_textbox_undo(undoData.first, undoData.second);
        undoHeldData.erase(it);
    }
}

void TextBoxEditTool::add_undo_if_selecting_area(TextBoxCanvasComponent& a, const std::function<void()>& func) {
    if(a.cursor->selectionBeginPos == a.cursor->selectionEndPos)
        func();
    else
        userInput->do_textbox_operation_with_undo(func);
}

void TextBoxEditTool::add_undo(const std::function<void()>& func) {
    userInput->do_textbox_operation_with_undo(func);
}

uint8_t TextBoxEditTool::get_new_decoration_value() {
    uint8_t toRet = skia::textlayout::TextDecoration::kNoDecoration;
    if(newIsUnderlined)
        toRet |= skia::textlayout::TextDecoration::kUnderline;
    if(newIsLinethrough)
        toRet |= skia::textlayout::TextDecoration::kLineThrough;
    if(newIsOverline)
        toRet |= skia::textlayout::TextDecoration::kOverline;
    return toRet;
}

void TextBoxEditTool::set_styles_at_selection(TextBoxCanvasComponent& a) {
    auto& currentMods = *currentModsPtr;

    TextPosition start = std::min(a.cursor->selectionBeginPos, a.cursor->selectionEndPos);
    TextPosition end = std::max(a.cursor->selectionBeginPos, a.cursor->selectionEndPos);
    if(start == end)
        currentMods = a.textBox->get_mods_used_at_pos(a.textBox->move(TextBox::Movement::LEFT, start));
    else
        currentMods = a.textBox->get_mods_used_at_pos(start);

    newFontName = std::static_pointer_cast<FontFamiliesTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::FONT_FAMILIES])->get_families().back().c_str();
    newFontSize = std::static_pointer_cast<SizeTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::SIZE])->get_size();
    newTextColor = std::static_pointer_cast<ColorTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::COLOR])->get_color();
    newHighlightColor = std::static_pointer_cast<HighlightColorTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::HIGHLIGHT_COLOR])->get_color();
    newIsBold = std::static_pointer_cast<WeightTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::WEIGHT])->get_weight() == SkFontStyle::Weight::kBold_Weight;
    newIsItalic = std::static_pointer_cast<SlantTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::SLANT])->get_slant() == SkFontStyle::Slant::kItalic_Slant;
    newIsUnderlined = std::static_pointer_cast<DecorationTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::DECORATION])->get_decoration_value() & skia::textlayout::TextDecoration::kUnderline;
    newIsLinethrough = std::static_pointer_cast<DecorationTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::DECORATION])->get_decoration_value() & skia::textlayout::TextDecoration::kLineThrough;
    newIsOverline = std::static_pointer_cast<DecorationTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::DECORATION])->get_decoration_value() & skia::textlayout::TextDecoration::kOverline;

    currentPStyle = a.textBox->get_paragraph_style_data_at(start.fParagraphIndex);
}

void TextBoxEditTool::commit_edit_updates(std::any& prevData) {
    auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
    a.d.editing = false;
    comp->obj->commit_update(drawP);
    userInput = nullptr;
    CustomEvents::emit_event(CustomEvents::RefreshTextBoxInputEvent{});
}

TextBoxEditTool::TextBoxEditToolAllData TextBoxEditTool::get_all_data(const TextBoxCanvasComponent& a) {
    return {
        .textboxData = a.d,
        .richText = a.textBox->get_rich_text_data()
    };
}

void TextBoxEditTool::edit_start(EditTool& editTool, std::any& prevData) {
    auto& a = static_cast<TextBoxCanvasComponent&>(comp->obj->get_comp());
    auto& cur = a.cursor;
    auto& textbox = a.textBox;

    a.init_text_box(drawP);

    cur = std::make_shared<TextBox::Cursor>();
    Vector2f textSelectPos = a.get_mouse_pos(drawP);
    textbox->process_mouse_left_button(*cur, textSelectPos, 1, false, false);
    prevData = get_all_data(a);
    a.d.editing = true;

    set_styles_at_selection(a);

    editTool.add_point_handle({&a.d.p1, nullptr, &a.d.p2});
    editTool.add_point_handle({&a.d.p2, &a.d.p1, nullptr});

    comp->obj->commit_update(drawP);

    userInput = std::make_unique<RichTextUserInput>(CustomEvents::text_box_get_new_id(), textbox, cur, currentModsPtr);
    CustomEvents::emit_event(CustomEvents::RefreshTextBoxInputEvent{});
}

bool TextBoxEditTool::edit_update() {
    return true;
}
