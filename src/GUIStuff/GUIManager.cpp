#include "GUIManager.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkPath.h>
#include "Elements/SVGIcon.hpp"
#include "Elements/SelectableButton.hpp"
#include "Elements/RotateWheel.hpp"
#include "Elements/MovableTabList.hpp"
#include "Elements/Element.hpp"
#include <Helpers/ConvertVec.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>

namespace GUIStuff {

GUIManager::GUIManager()
{
    Clay_SetMaxMeasureTextCacheWordCount(100000);
    clayArena = Clay_CreateArenaWithCapacityAndMemory(Clay_MinMemorySize(), malloc(Clay_MinMemorySize()));
    clayInstance = Clay_Initialize(clayArena, Clay_Dimensions(1.0f, 1.0f), (Clay_ErrorHandler)clay_error_handler);
    Clay_SetMeasureTextFunction(clay_skia_measure_text, this);
    //Clay_SetDebugModeEnabled(true);
}

Clay_Dimensions GUIManager::clay_skia_measure_text(Clay_StringSlice str, Clay_TextElementConfig* config, void* userData) {
    GUIManager* window = static_cast<GUIManager*>(userData);
    SkFont font = window->io->get_font(config->fontSize);
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    float nextText = font.measureText(str.chars, str.length, SkTextEncoding::kUTF8, nullptr);
    return Clay_Dimensions(nextText, - metrics.fAscent + metrics.fDescent);
}

void GUIManager::begin() {
    io->mouse.pos = io->mouse.globalPos - windowPos;
    io->strArena = &strArena;
    Clay_SetLayoutDimensions(Clay_Dimensions(windowSize.x(), windowSize.y()));
    Clay_SetPointerState(Clay_Vector2((float)io->mouse.pos.x(), (float)io->mouse.pos.y()), io->mouse.leftHeld);
    Clay_UpdateScrollContainers(false, Clay_Vector2(io->mouse.scroll.y(), io->mouse.scroll.y()), io->deltaTime * 2.0f);
    Clay_SetCurrentContext(clayInstance);

    strArena.reset();

    Clay_BeginLayout();
}

void GUIManager::end() {
    renderCommands = Clay_EndLayout();
    if(!idStack.empty())
        throw std::runtime_error("[GUIManager::end] ID Stack is not empty on end (push_id and pop_id calls not equal)");
}

void GUIManager::draw(SkCanvas* canvas) {
    canvas->save();
    canvas->translate(windowPos.x(), windowPos.y());

    for(size_t i = 0; i < static_cast<size_t>(renderCommands.length); i++) {
        Clay_RenderCommand* command = Clay_RenderCommandArray_Get(&renderCommands, i);
        Clay_BoundingBox bb = command->boundingBox;
        switch(command->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData* config = &command->renderData.rectangle;
                SkVector radii[4] = {
                    {config->cornerRadius.topLeft, config->cornerRadius.topLeft},
                    {config->cornerRadius.topRight, config->cornerRadius.topRight},
                    {config->cornerRadius.bottomRight, config->cornerRadius.bottomRight},
                    {config->cornerRadius.bottomLeft, config->cornerRadius.bottomLeft}
                };

                SkRRect rrect;
                rrect.setRectRadii(
                    SkRect::MakeXYWH(bb.x, bb.y, bb.width, bb.height),
                    radii
                );

                SkPaint paint;
                paint.setStyle(SkPaint::kFill_Style);
                paint.setColor4f(convert_vec4<SkColor4f>(config->backgroundColor));
                canvas->drawRRect(rrect, paint);

                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData* config = &command->renderData.text;
                SkPaint paint;
                paint.setColor4f(convert_vec4<SkColor4f>(config->textColor));
                SkFont f = io->get_font(config->fontSize);
                SkFontMetrics metrics;
                f.getMetrics(&metrics);
                canvas->drawSimpleText(config->stringContents.chars, config->stringContents.length, SkTextEncoding::kUTF8, bb.x, (bb.y + bb.height - metrics.fDescent), io->get_font(config->fontSize), paint);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Clay_BorderRenderData* config = &command->renderData.border;

                SkPaint p;
                p.setColor4f(convert_vec4<SkColor4f>(config->color));
                p.setStyle(SkPaint::kStroke_Style);

                float halfLineWidth = 0.0f;
                // Top Left corner
                if (config->cornerRadius.topLeft > 0.0f) {
                    SkPath path;
                    float lineWidth = config->width.top;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo(bb.x + halfLineWidth, bb.y + config->cornerRadius.topLeft + halfLineWidth);
                    path.arcTo((bb.x + halfLineWidth), (bb.y + halfLineWidth), (bb.x + config->cornerRadius.topLeft + halfLineWidth), (bb.y + halfLineWidth), config->cornerRadius.topLeft);
                    canvas->drawPath(path, p);
                }
                // Top border
                if (config->width.top > 0.0f) {
                    SkPath path;
                    float lineWidth = config->width.top;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + config->cornerRadius.topLeft + halfLineWidth), (bb.y + halfLineWidth));
                    path.lineTo((bb.x + bb.width - config->cornerRadius.topRight - halfLineWidth), (bb.y + halfLineWidth));
                    canvas->drawPath(path, p);
                }
                // Top Right Corner
                if (config->cornerRadius.topRight > 0.0f) {
                    SkPath path;
                    float lineWidth = config->width.top;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + bb.width - config->cornerRadius.topRight - halfLineWidth), (bb.y + halfLineWidth));
                    path.arcTo((bb.x + bb.width - halfLineWidth), (bb.y + halfLineWidth), (bb.x + bb.width - halfLineWidth), (bb.y + config->cornerRadius.topRight + halfLineWidth), config->cornerRadius.topRight);
                    canvas->drawPath(path, p);
                }
                // Right border
                if (config->width.right > 0.0f) {
                    SkPath path;
                    float lineWidth = config->width.right;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + bb.width - halfLineWidth), (bb.y + config->cornerRadius.topRight + halfLineWidth));
                    path.lineTo((bb.x + bb.width - halfLineWidth), (bb.y + bb.height - config->cornerRadius.bottomRight - halfLineWidth));
                    canvas->drawPath(path, p);
                }
                // Bottom Right Corner
                if (config->cornerRadius.bottomRight > 0.0f) {
                    SkPath path;
                    float lineWidth = config->width.bottom;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + bb.width - halfLineWidth), (bb.y + bb.height - config->cornerRadius.bottomRight - halfLineWidth));
                    path.arcTo((bb.x + bb.width - halfLineWidth), (bb.y + bb.height - halfLineWidth), (bb.x + bb.width - config->cornerRadius.bottomRight - halfLineWidth), (bb.y + bb.height - halfLineWidth), config->cornerRadius.bottomRight);
                    canvas->drawPath(path, p);
                }
                // Bottom Border
                if (config->width.bottom > 0.0f) {
                    SkPath path;
                    float lineWidth = config->width.bottom;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + config->cornerRadius.bottomLeft + halfLineWidth), (bb.y + bb.height - halfLineWidth));
                    path.lineTo((bb.x + bb.width - config->cornerRadius.bottomRight - halfLineWidth), (bb.y + bb.height - halfLineWidth));
                    canvas->drawPath(path, p);
                }
                // Bottom Left Corner
                if (config->cornerRadius.bottomLeft > 0.0f) {
                    SkPath path;
                    float lineWidth = config->width.bottom;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + config->cornerRadius.bottomLeft + halfLineWidth), (bb.y + bb.height - halfLineWidth));
                    path.arcTo((bb.x + halfLineWidth), (bb.y + bb.height - halfLineWidth), (bb.x + halfLineWidth), (bb.y + bb.height - config->cornerRadius.bottomLeft - halfLineWidth), config->cornerRadius.bottomLeft);
                    canvas->drawPath(path, p);
                }
                // Left Border
                if (config->width.left > 0.0f) {
                    SkPath path;
                    float lineWidth = config->width.left;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + halfLineWidth), (bb.y + bb.height - config->cornerRadius.bottomLeft - halfLineWidth));
                    path.lineTo((bb.x + halfLineWidth), (bb.y + config->cornerRadius.topRight + halfLineWidth));
                    canvas->drawPath(path, p);
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                canvas->save();
                SkRect clipRect = SkRect::MakeXYWH(bb.x, bb.y, bb.width, bb.height);
                canvas->clipRect(clipRect);
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                canvas->restore();
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
                Element* customElement = (Element*)command->renderData.custom.customData;
                customElement->clay_draw(canvas, *io, command);
                break;
            }
            default:
                break;
        }
    }

    canvas->restore();
}

void GUIManager::top_to_bottom_window_popup_layout(Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& elemUpdate) {
    CLAY({.layout = { 
            .sizing = {.width = x, .height = y },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
    },
    .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
    .cornerRadius = CLAY_CORNER_RADIUS(10),
    .floating = { .attachTo = CLAY_ATTACH_TO_PARENT },
    .border = {.color = convert_vec4<Clay_Color>(io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(2)}
    }) {
        if(Clay_Hovered())
            io->hoverObstructed = true;
        elemUpdate();
    }
}

void GUIManager::left_to_right_layout(Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& elemUpdate) {
    CLAY({.layout = { 
          .sizing = {.width = x, .height = y },
          .childGap = io->theme->childGap1,
          .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
          .layoutDirection = CLAY_LEFT_TO_RIGHT,
    }
    }) {
        elemUpdate();
    }
}

void GUIManager::left_to_right_line_layout(const std::function<void()>& elemUpdate) {
    left_to_right_layout(CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0), elemUpdate);
}

void GUIManager::text_label_size(const std::string& val, float modifier) {
    CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io->theme->frontColor1), .fontSize = static_cast<uint16_t>(io->fontSize * modifier)}));
}

void GUIManager::text_label_color(const std::string& val, const SkColor4f& color) {
    CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(color), .fontSize = io->fontSize }));
}

void GUIManager::text_label(const std::string& val) {
    CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io->theme->frontColor1), .fontSize = io->fontSize }));
}

void GUIManager::text_label_centered(const std::string& val) {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        }
    }) {
        CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io->theme->frontColor1), .fontSize = io->fontSize }));
    }
}

bool GUIManager::radio_button(const std::string& id, bool val, const std::function<void()>& elemUpdate) {
    push_id(id);
    RadioButton* e = insert_element<RadioButton>(); 
    e->update(*io, val, elemUpdate);
    pop_id();
    return e->selection.clicked;
}

void GUIManager::checkbox(const std::string& id, bool* val, const std::function<void()>& elemUpdate) {
    push_id(id);
    CheckBox* e = insert_element<CheckBox>(); 
    e->update(*io, *val, elemUpdate);
    if(e->selection.clicked)
        *val = !(*val);
    pop_id();
}

void GUIManager::input_text_field(const std::string& id, const std::string& name, std::string* val, const std::function<void(SelectionHelper&)>& elemUpdate) {
    left_to_right_line_layout([&]() {
        text_label(name);
        input_text(id, val, elemUpdate);
    });
}

void GUIManager::input_path_field(const std::string& id, const std::string& name, std::filesystem::path* val, std::filesystem::file_type fileTypeRestriction, const std::function<void(SelectionHelper&)>& elemUpdate) {
    left_to_right_line_layout([&]() {
        text_label(name);
        input_path(id, val, fileTypeRestriction, elemUpdate);
    });
}

void GUIManager::checkbox_field(const std::string& id, const std::string& name, bool* val, const std::function<void()>& elemUpdate) {
    left_to_right_line_layout([&]() {
        checkbox(id, val, elemUpdate);
        text_label(name);
    });
}

bool GUIManager::radio_button_field(const std::string& id, const std::string& name, bool val, const std::function<void()>& elemUpdate) {
    bool toRet;
    left_to_right_line_layout([&]() {
        toRet = radio_button(id, val, elemUpdate);
        text_label(name);
    });
    return toRet;
}

void GUIManager::input_color_component_255(const std::string& id, float* val, const std::function<void(SelectionHelper&)>& elemUpdate) {
    push_id(id);
    insert_element<TextBox<float>>()->update(*io, val,
        [&](const std::string& str) {
            int roundTo255;
            std::stringstream ss;
            ss << str;
            ss >> roundTo255;
            return std::clamp<float>(static_cast<float>(roundTo255) / 255.0f, 0.0f, 1.0f);
        },
        [&](const float& colorFloat) {
            int convertTo255 = colorFloat * 255;
            std::stringstream ss;
            ss << convertTo255;
            return ss.str();
        }, true, false, elemUpdate);
    pop_id();
}

void GUIManager::input_text(const std::string& id, std::string* val, const std::function<void(SelectionHelper&)>& elemUpdate) {
    push_id(id);
    insert_element<TextBox<std::string>>()->update(*io, val,
        [&](const std::string& str) {
            return str;
        },
        [&](const std::string& str) {
            return str;
        }, true, true, elemUpdate);
    pop_id();
}

bool GUIManager::selectable_button(const std::string& id, const std::function<void(SelectionHelper&, bool)>& elemUpdate, bool hasBorder, bool hasBorderPadding, bool isSelected) {
    push_id(id);
    SelectableButton* e = insert_element<SelectableButton>();
    e->update(*io, hasBorder, hasBorderPadding, elemUpdate, isSelected);
    bool toRet = e->selection.clicked;
    pop_id();
    return toRet;
}

void GUIManager::svg_icon(const std::string& id, const std::string& svgPath, bool isHighlighted, const std::function<void()>& elemUpdate) {
    push_id(id);
    insert_element<SVGIcon>()->update(*io, svgPath, isHighlighted, elemUpdate);
    pop_id();
}

bool GUIManager::svg_icon_button(const std::string& id, const std::string& svgPath, bool isSelected, float size, bool hasBorder, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    push_id(id);
    CLAY ({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size) } } }) {
        toRet = selectable_button("0", [&](SelectionHelper& s, bool is) {
            svg_icon("1", svgPath, is || s.held || s.hovered);
            if(elemUpdate)
                elemUpdate();
        }, hasBorder, true, isSelected);
    }
    pop_id();
    return toRet;
}

bool GUIManager::text_button_sized(const std::string& id, const std::string& text, Clay_SizingAxis x, Clay_SizingAxis y, bool isSelected, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    CLAY ({.layout = {.sizing = {.width = x, .height = y } } }) {
        toRet = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            text_label(text);
            if(elemUpdate)
                elemUpdate();
        }, true, true, isSelected);
    }
    return toRet;
}

bool GUIManager::text_button(const std::string& id, const std::string& text, bool isSelected, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    CLAY ({.layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) } } }) {
        toRet = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            text_label(text);
            if(elemUpdate)
                elemUpdate();
        }, true, true, isSelected);
    }
    return toRet;
}

bool GUIManager::text_button_wide(const std::string& id, const std::string& text, bool isSelected, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    CLAY ({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) } } }) {
        toRet = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            text_label(text);
            if(elemUpdate)
                elemUpdate();
        }, true, true, isSelected);
    }
    return toRet;
}

void GUIManager::input_scalar(const std::string& id, uint8_t* val, uint8_t min, uint8_t max, int decimalPrecision, const std::function<void(SelectionHelper&)>& elemUpdate) {
    push_id(id);
    insert_element<TextBox<uint8_t>>()->update(*io, val, 
        [&](const std::string& a) {
            if(a.empty())
                return min;
            uint32_t toRet;
            std::stringstream ss;
            ss << a;
            ss >> toRet;
            if(ss.fail())
                return min;
            return std::clamp<uint8_t>(toRet, min, max);
        },
        [&](const uint8_t& a) {
            return std::to_string(static_cast<uint32_t>(a));
        }, true, false, elemUpdate);
    pop_id();
}

void GUIManager::input_path(const std::string& id, std::filesystem::path* val, std::filesystem::file_type fileTypeRestriction, const std::function<void(SelectionHelper&)>& elemUpdate) {
    push_id(id);
    insert_element<TextBox<std::filesystem::path>>()->update(*io, val, 
        [&](const std::string& a) {
            std::filesystem::path toRet = std::filesystem::path(a);
            if(fileTypeRestriction != std::filesystem::file_type::none && std::filesystem::status(toRet).type() != fileTypeRestriction)
                return std::optional<std::filesystem::path>(std::nullopt);
            return std::optional<std::filesystem::path>(toRet);
        },
        [&](const std::filesystem::path& a) {
            return a.string();
        }, true, false, elemUpdate);
    pop_id();
}

void GUIManager::tab_list(const std::string& id, const std::vector<std::pair<std::string, std::string>>& tabNames, size_t& selectedTab, std::optional<size_t>& closedTab, const std::function<void()>& elemUpdate) {
    push_id(id);
    insert_element<MovableTabList>()->update(*io, tabNames, selectedTab, closedTab, elemUpdate);
    pop_id();
}

void GUIManager::dropdown_select(const std::string& id, size_t* val, const std::vector<std::string>& selections, float width, const std::function<void()>& hoverboxElemUpdate) {
    push_id(id);
    bool& dropDownOpen = insert_any_with_id(0, false);
    left_to_right_layout(CLAY_SIZING_FIXED(width), CLAY_SIZING_FIT(0), [&]() {
        bool click = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                    .padding = {.left = 4, .right = 4},
                    .childGap = io->theme->childGap1,
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }
            }) {
                text_label(selections[*val]);
                CLAY({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(20), .height = CLAY_SIZING_FIT(20)}
                    }
                }) {
                    svg_icon("dropico", "data/icons/droparrow.svg", dropDownOpen);
                }
            }
        }, true, false, dropDownOpen);
        if(dropDownOpen) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(width), .height = CLAY_SIZING_FIT(0)},
                    .padding = {.left = 2, .right = 2},
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .cornerRadius = CLAY_CORNER_RADIUS(4),
                .floating = {
                    .offset = {
                        .y = 4
                    },
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_LEFT_TOP,
                        .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM
                    },
                    .attachTo = CLAY_ATTACH_TO_PARENT
                },
                .border = {
                    .color = convert_vec4<Clay_Color>(io->theme->backColor3),
                    .width = CLAY_BORDER_OUTSIDE(4)
                }
            }) {
                obstructing_window();
                for(size_t i = 0; i < selections.size(); i++) {
                    bool selectedEntry = *val == i;
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                        },
                    }) {
                        SkColor4f entryColor;
                        if(selectedEntry)
                            entryColor = io->theme->backColor2;
                        else if(Clay_Hovered())
                            entryColor = io->theme->backColor3;
                        else
                            entryColor = io->theme->backColor1;
                        CLAY({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                            },
                            .backgroundColor = convert_vec4<Clay_Color>(entryColor)
                        }) {
                            text_label(selections[i]);
                            if(io->mouse.leftClick && Clay_Hovered()) {
                                *val = i;
                                dropDownOpen = false;
                            }
                        }
                    }
                }
                if(hoverboxElemUpdate)
                    hoverboxElemUpdate();
            }
        }
        if(click)
            dropDownOpen = !dropDownOpen;
    });
    pop_id();
}

void GUIManager::scroll_bar_area(const std::string& uniqueId, const std::function<void(float, float, float)>& elemUpdate) {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        }
    }) {
        struct ScrollAreaData {
            float currentScrollPos = 0.0f;
            bool isMoving = false;
            float contentDimensions = 100.0f;
            float containerDimensions = 100.0f;
        };
        push_id(uniqueId);
        ScrollAreaData& sD = insert_any(ScrollAreaData{});
        pop_id();

        Clay_ElementId clayID = Clay_GetElementId(strArena.std_str_to_clay_str(uniqueId));

        CLAY({
            .id = clayID,
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .clip = {.vertical = true, .childOffset = Clay_GetScrollOffset()}
        }) {
            elemUpdate(sD.contentDimensions, sD.containerDimensions, sD.currentScrollPos);
        }

        // Make sure you are accessing this data after the element has been created
        Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(clayID);
        Clay_BoundingBox scrollAreaBB = Clay_GetElementData(clayID).boundingBox;

        sD.contentDimensions = scrollData.contentDimensions.height;
        sD.containerDimensions = scrollData.scrollContainerDimensions.height;

        if(scrollData.contentDimensions.height > scrollData.scrollContainerDimensions.height) {
            float sAreaDim = scrollData.scrollContainerDimensions.height;
            float contDim = scrollData.contentDimensions.height;
            float scrollerSize = (sAreaDim / contDim) * sAreaDim;
            float scrollPosMax = (contDim - sAreaDim) + 3.0f; // NOTE: Adding + 3.0f to prevent bug with scroller that pushes everything under it when its at the bottom of the scroll area
            float scrollerPos = std::fabs(scrollData.scrollPosition->y / scrollPosMax);
            float areaAboveScrollerSize = scrollerPos * (sAreaDim - scrollerSize);

            if(Clay_Hovered())
                sD.currentScrollPos += io->mouse.scroll.y() * io->deltaTime * 3000.0f;

            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(12), .height = CLAY_SIZING_GROW(0)},
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1)
            }) {
                SkColor4f scrollerColor;
                if(sD.isMoving)
                    scrollerColor = io->theme->fillColor1;
                else if(Clay_Hovered())
                    scrollerColor = io->theme->fillColor2;
                else
                    scrollerColor = io->theme->backColor3;

                CLAY({ 
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(areaAboveScrollerSize)}
                    }
                }) {}
                CLAY({
                    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(scrollerSize)}},
                    .backgroundColor = convert_vec4<Clay_Color>(scrollerColor),
                    .cornerRadius = CLAY_CORNER_RADIUS(3),
                }) {
                }
                CLAY({ .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                if(Clay_Hovered() && io->mouse.leftClick)
                    sD.isMoving = true;
                if(!io->mouse.leftHeld)
                    sD.isMoving = false;
                if(sD.isMoving)
                    sD.currentScrollPos = ((io->mouse.pos.y() - scrollAreaBB.y) / scrollAreaBB.height) * (-scrollPosMax);
                sD.currentScrollPos = std::clamp(sD.currentScrollPos, -scrollPosMax, 0.0f);
                scrollData.scrollPosition->y = sD.currentScrollPos;
            }
        }
        else
            sD.currentScrollPos = 0.0f;
    }
}


void GUIManager::scroll_bar_many_entries_area(const std::string& uniqueId, float entryHeight, size_t entryCount, const std::function<void(size_t, bool)>& entryUpdate, const std::function<void(float, float, float)>& elemUpdate) {
    push_id(uniqueId);
    scroll_bar_area(uniqueId, [&](float scrollContentHeight, float containerHeight, float scrollAmount) {
        if(elemUpdate)
            elemUpdate(scrollContentHeight, containerHeight, scrollAmount);

        bool listHovered = Clay_Hovered();

        scrollAmount = std::fabs(scrollAmount);
        float entryHeight = 25.0f;
        size_t startPoint = scrollAmount / entryHeight;
        size_t elementsContainable = (containerHeight / entryHeight) + 1;
        size_t endPoint = std::min(entryCount, startPoint + elementsContainable);

        if(startPoint != 0) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(startPoint * entryHeight)},
                }
            }) { }
        }

        for(size_t i = startPoint; i < endPoint; i++) {
            push_id(i);
            entryUpdate(i, listHovered);
            pop_id();
        }

        if(endPoint != entryCount) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED((entryCount - endPoint) * entryHeight)},
                }
            }) { }
        }
    });
    pop_id();
}

bool GUIManager::rotate_wheel(const std::string& id, double* angle, float size, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    push_id(id);
    CLAY ({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size) } } }) {
        RotateWheel* e = insert_element<RotateWheel>();
        e->update(*io, angle, elemUpdate);
        toRet = e->selection.clicked;
    }
    pop_id();
    return toRet;
}

void GUIManager::paint_circle_popup_menu(const std::string& id, const Vector2f& centerPos, const PaintCircleMenu::Data& val, const std::function<void()>& elemUpdate) {
    float size = 300.0f;
    push_id(id);
    CLAY({.layout = { 
            .sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size)}
        },
        .floating = {
            .offset = {centerPos.x() - size * 0.5f, centerPos.y() - size * 0.5f},
            .zIndex = -1,
            .attachTo = CLAY_ATTACH_TO_ROOT
        }
    }) {
        PaintCircleMenu* e = insert_element<PaintCircleMenu>();
        e->update(*io, val, elemUpdate);
    }
    pop_id();
}

void GUIManager::obstructing_window() {
    if(Clay_Hovered())
        io->hoverObstructed = true;
}

void GUIManager::clay_error_handler(Clay_ErrorData errorData) {
    std::cout << "[Clay Error] " << errorData.errorText.chars << std::endl;
}

void GUIManager::push_id(int64_t id) {
    idStack.emplace_back(id);
}

void GUIManager::push_id(const std::string& id) {
    idStack.emplace_back(id);
}

void GUIManager::pop_id() {
    idStack.pop_back();
}

}
