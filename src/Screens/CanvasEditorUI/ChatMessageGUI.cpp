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

#include "ChatMessageGUI.hpp"
#include "../CanvasEditorScreen.hpp"
#include "../../World.hpp"
#include "../../MainProgram.hpp"
#include "../../GUIStuff/Elements/TextParagraph.hpp"
#include "../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/ButtonHelpers.hpp"

using namespace GUIStuff;
using namespace ElementHelpers;

ChatMessageGUI::ChatMessageGUI(CanvasEditorScreen& initScreen):
    s(initScreen)
{}

void ChatMessageGUI::update() {
    if(!is_open()) {
        for(auto& chatMessage : s.world->chatMessages) {
            bool wasShown = chatMessage.time < World::ChatMessage::DISPLAY_TIME;
            chatMessage.time.update_time_since();
            bool isShown = chatMessage.time < World::ChatMessage::DISPLAY_TIME;
            bool isFading = chatMessage.time >= World::ChatMessage::FADE_START_TIME;
            if((isFading && isShown) || (wasShown && !isShown))
                s.main.g.gui.set_to_layout();
        }
    }
}

void ChatMessageGUI::set_open(bool newState) {
    if(isChatboxOpen != newState) {
        if(newState) // Only clear on open, because sometimes message has to send when closed
            chatMessageInput.clear();
        isChatboxOpen = newState;
        s.main.g.gui.set_to_layout();
    }
}

bool ChatMessageGUI::is_open() {
    return isChatboxOpen;
}

RichText::TextData build_paragraph_from_chat_message(const GUIStuff::Theme& theme, const World::ChatMessage& message, float alpha) {
    RichText::TextData toRet;

    auto& par = toRet.paragraphs.emplace_back();

    RichText::PositionedTextStyleMod& positionedModInit = toRet.tStyleMods.emplace_back();
    positionedModInit.pos = {0, 0};
    positionedModInit.mods[RichText::TextStyleModifier::ModifierType::WEIGHT] = std::make_shared<RichText::WeightTextStyleModifier>(SkFontStyle::Weight::kBold_Weight);
    positionedModInit.mods[RichText::TextStyleModifier::ModifierType::COLOR] = std::make_shared<RichText::ColorTextStyleModifier>(convert_vec4<Vector4f>(color_mul_alpha(message.type == World::ChatMessage::JOIN ? theme.warningColor : theme.frontColor1, alpha)));
    if(message.type == World::ChatMessage::JOIN)
        par.text += message.name + " ";
    else
        par.text += "[" + message.name + "] ";

    RichText::PositionedTextStyleMod& positionedModMessage = toRet.tStyleMods.emplace_back();
    positionedModMessage.pos = {0, par.text.size()};
    positionedModMessage.mods[RichText::TextStyleModifier::ModifierType::WEIGHT] = std::make_shared<RichText::WeightTextStyleModifier>(SkFontStyle::Weight::kNormal_Weight);
    par.text += message.message;

    return toRet;
}

void ChatMessageGUI::layout(const Vector2f& offset, Clay_FloatingAttachPoints attachPoints) {
    auto& gui = s.main.g.gui;
    auto& io = gui.io;

    if(s.world->is_connected()) {
        gui.element<LayoutElement>("Infinipaint chat box", [&](LayoutElement* l, const Clay_ElementId& lId) {
            CLAY(lId, {
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0, std::clamp(500.0f, 10.0f, io.safeWindowRect.width() - offset.x() * 2.0f)), .height = CLAY_SIZING_FIT(0) },
                    .childGap = 0,
                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .floating = {.offset = {offset.x(), offset.y()}, .zIndex = gui.get_z_index(), .attachPoints = attachPoints, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                if(l->get_bb().has_value()) {
                    if(is_open()) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                .padding = CLAY_PADDING_ALL(0),
                                .childGap = 0,
                                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM
                            },
                            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1)
                        }) {
                            int id = 0;
                            for(auto& chatMessage : s.world->chatMessages | std::views::reverse) {
                                gui.new_id(id++, [&] {
                                    gui.element<TextParagraph>("text", TextParagraph::Data{
                                        .text = build_paragraph_from_chat_message(*io.theme, chatMessage, 1.0f),
                                        .maxGrowX = l->get_bb()->width(),
                                        .ellipsis = false
                                    });
                                });
                            }
                        }

                        left_to_right_line_layout(gui, [&] {
                            input_text(gui, "message input", &chatMessageInput, {
                                .onEnter = [&] {
                                    if(!chatMessageInput.empty())
                                        s.world->send_chat_message(chatMessageInput);
                                    set_open(false);
                                },
                                .onDeselect = [&] {
                                    set_open(false);
                                }
                            })->select();
                            text_button(gui, "send button", "Send", {
                                .instantResponse = true,
                                .onClick = [&] {
                                    if(!chatMessageInput.empty())
                                        s.world->send_chat_message(chatMessageInput);
                                    set_open(false);
                                }
                            });
                        });
                    }
                    else {
                        gui.new_id("Message popups", [&] {
                            int id = 0;
                            for(auto& chatMessage : s.world->chatMessages | std::views::reverse) {
                                chatMessage.time.update_time_since();
                                if(chatMessage.time < World::ChatMessage::DISPLAY_TIME) {
                                    gui.new_id(id++, [&] {
                                        float a = 1.0f - lerp_time<float>(chatMessage.time, World::ChatMessage::DISPLAY_TIME, World::ChatMessage::FADE_START_TIME);
                                        gui.element<LayoutElement>("Chat message", [&](LayoutElement*, const Clay_ElementId& lId) {
                                            CLAY(lId, {
                                                .layout = {
                                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                                    .padding = CLAY_PADDING_ALL(0),
                                                    .childGap = 0,
                                                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                                                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                                                },
                                                .backgroundColor = convert_vec4<Clay_Color>(color_mul_alpha(io.theme->backColor1, a)),
                                            }) {
                                                gui.element<TextParagraph>("message", TextParagraph::Data{
                                                    .text = build_paragraph_from_chat_message(*io.theme, chatMessage, a),
                                                    .maxGrowX = l->get_bb()->width(),
                                                    .ellipsis = false
                                                });
                                            }
                                        });
                                    });
                                }
                            }
                        });
                    }
                }
            }
        });
    }
}
