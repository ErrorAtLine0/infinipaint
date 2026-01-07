#include "GUIManager.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkPath.h>
#include <include/core/SkPathBuilder.h>
#include "Elements/SVGIcon.hpp"
#include "Elements/FontPicker.hpp"
#include "Elements/SelectableButton.hpp"
#include "Elements/RotateWheel.hpp"
#include "Elements/TextParagraph.hpp"
#include "Elements/MovableTabList.hpp"
#include "Elements/Element.hpp"
#include "Elements/TextParagraph.hpp"
#include <Helpers/ConvertVec.hpp>
#include <filesystem>
#include <fstream>
#include <Helpers/Logger.hpp>
#include <iostream>
#include <cmath>
#include <limits>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/include/ParagraphStyle.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/TextStyle.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include "../FontData.hpp"

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

    skia::textlayout::ParagraphStyle pStyle;
    pStyle.setTextAlign(skia::textlayout::TextAlign::kLeft);
    skia::textlayout::TextStyle tStyle;
    tStyle.setFontSize(config->fontSize);
    tStyle.setFontFamilies(window->io->fonts->get_default_font_families());
    pStyle.setTextStyle(tStyle);

    skia::textlayout::ParagraphBuilderImpl a(pStyle, window->io->fonts->collection, SkUnicodes::ICU::Make());
    a.addText(str.chars, str.length);
    std::unique_ptr<skia::textlayout::Paragraph> p = a.Build();
    p->layout(std::numeric_limits<float>::max());

    return Clay_Dimensions(p->getMaxIntrinsicWidth(), p->getHeight());
}

void GUIManager::begin() {
    for(auto& [id, e] : elements)
        e.isUsedThisFrame = false;

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
    std::erase_if(elements, [](auto& p) {
        return !p.second.isUsedThisFrame;
    });
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

                skia::textlayout::ParagraphStyle pStyle;
                pStyle.setTextAlign(skia::textlayout::TextAlign::kLeft);
                skia::textlayout::TextStyle tStyle;
                tStyle.setFontSize(config->fontSize);
                tStyle.setFontFamilies(io->fonts->get_default_font_families());
                tStyle.setColor(convert_vec4<SkColor4f>(config->textColor).toSkColor());
                pStyle.setTextStyle(tStyle);

                skia::textlayout::ParagraphBuilderImpl a(pStyle, io->fonts->collection, SkUnicodes::ICU::Make());
                a.addText(config->stringContents.chars, config->stringContents.length);
                std::unique_ptr<skia::textlayout::Paragraph> p = a.Build();
                p->layout(std::numeric_limits<float>::max());
                p->paint(canvas, bb.x, bb.y);

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
                    SkPathBuilder path;
                    float lineWidth = config->width.top;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo(bb.x + halfLineWidth, bb.y + config->cornerRadius.topLeft + halfLineWidth);
                    path.arcTo(SkPoint{(bb.x + halfLineWidth), (bb.y + halfLineWidth)}, SkPoint{(bb.x + config->cornerRadius.topLeft + halfLineWidth), (bb.y + halfLineWidth)}, config->cornerRadius.topLeft);
                    canvas->drawPath(path.detach(), p);
                }
                // Top border
                if (config->width.top > 0.0f) {
                    SkPathBuilder path;
                    float lineWidth = config->width.top;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + config->cornerRadius.topLeft + halfLineWidth), (bb.y + halfLineWidth));
                    path.lineTo((bb.x + bb.width - config->cornerRadius.topRight - halfLineWidth), (bb.y + halfLineWidth));
                    canvas->drawPath(path.detach(), p);
                }
                // Top Right Corner
                if (config->cornerRadius.topRight > 0.0f) {
                    SkPathBuilder path;
                    float lineWidth = config->width.top;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + bb.width - config->cornerRadius.topRight - halfLineWidth), (bb.y + halfLineWidth));
                    path.arcTo(SkPoint{(bb.x + bb.width - halfLineWidth), (bb.y + halfLineWidth)}, SkPoint{(bb.x + bb.width - halfLineWidth), (bb.y + config->cornerRadius.topRight + halfLineWidth)}, config->cornerRadius.topRight);
                    canvas->drawPath(path.detach(), p);
                }
                // Right border
                if (config->width.right > 0.0f) {
                    SkPathBuilder path;
                    float lineWidth = config->width.right;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + bb.width - halfLineWidth), (bb.y + config->cornerRadius.topRight + halfLineWidth));
                    path.lineTo((bb.x + bb.width - halfLineWidth), (bb.y + bb.height - config->cornerRadius.bottomRight - halfLineWidth));
                    canvas->drawPath(path.detach(), p);
                }
                // Bottom Right Corner
                if (config->cornerRadius.bottomRight > 0.0f) {
                    SkPathBuilder path;
                    float lineWidth = config->width.bottom;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + bb.width - halfLineWidth), (bb.y + bb.height - config->cornerRadius.bottomRight - halfLineWidth));
                    path.arcTo(SkPoint{(bb.x + bb.width - halfLineWidth), (bb.y + bb.height - halfLineWidth)}, SkPoint{(bb.x + bb.width - config->cornerRadius.bottomRight - halfLineWidth), (bb.y + bb.height - halfLineWidth)}, config->cornerRadius.bottomRight);
                    canvas->drawPath(path.detach(), p);
                }
                // Bottom Border
                if (config->width.bottom > 0.0f) {
                    SkPathBuilder path;
                    float lineWidth = config->width.bottom;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + config->cornerRadius.bottomLeft + halfLineWidth), (bb.y + bb.height - halfLineWidth));
                    path.lineTo((bb.x + bb.width - config->cornerRadius.bottomRight - halfLineWidth), (bb.y + bb.height - halfLineWidth));
                    canvas->drawPath(path.detach(), p);
                }
                // Bottom Left Corner
                if (config->cornerRadius.bottomLeft > 0.0f) {
                    SkPathBuilder path;
                    float lineWidth = config->width.bottom;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + config->cornerRadius.bottomLeft + halfLineWidth), (bb.y + bb.height - halfLineWidth));
                    path.arcTo(SkPoint{(bb.x + halfLineWidth), (bb.y + bb.height - halfLineWidth)}, SkPoint{(bb.x + halfLineWidth), (bb.y + bb.height - config->cornerRadius.bottomLeft - halfLineWidth)}, config->cornerRadius.bottomLeft);
                    canvas->drawPath(path.detach(), p);
                }
                // Left Border
                if (config->width.left > 0.0f) {
                    SkPathBuilder path;
                    float lineWidth = config->width.left;
                    p.setStrokeWidth(lineWidth);
                    path.moveTo((bb.x + halfLineWidth), (bb.y + bb.height - config->cornerRadius.bottomLeft - halfLineWidth));
                    path.lineTo((bb.x + halfLineWidth), (bb.y + config->cornerRadius.topRight + halfLineWidth));
                    canvas->drawPath(path.detach(), p);
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                Clay_ClipRenderData* clip = &command->renderData.clip;
                if(!clip->horizontal) {
                    bb.x = 0.0f;
                    bb.width = std::numeric_limits<float>::max();
                }
                if(!clip->vertical) {
                    bb.y = 0.0f;
                    bb.height = std::numeric_limits<float>::max();
                }

                canvas->save();
                SkRect clipRect = SkRect::MakeXYWH(bb.x - 1.0f, bb.y - 1.0f, bb.width + 1.0f, bb.height + 1.0f);
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
    CLAY_AUTO_ID({.layout = { 
            .sizing = {.width = x, .height = y },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
    },
    .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
    .cornerRadius = CLAY_CORNER_RADIUS(10),
    .floating = { .attachTo = CLAY_ATTACH_TO_PARENT },
    .border = {.color = convert_vec4<Clay_Color>(io->theme->fillColor2), .width = CLAY_BORDER_OUTSIDE(1)}
    }) {
        if(Clay_Hovered())
            io->hoverObstructed = true;
        elemUpdate();
    }
}

void GUIManager::left_to_right_layout(Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& elemUpdate) {
    CLAY_AUTO_ID({.layout = { 
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

void GUIManager::left_to_right_line_centered_layout(const std::function<void()>& elemUpdate) {
    CLAY_AUTO_ID({.layout = { 
          .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
          .childGap = io->theme->childGap1,
          .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
          .layoutDirection = CLAY_LEFT_TO_RIGHT,
    }
    }) {
        elemUpdate();
    }
}

void GUIManager::text_label_size(std::string_view val, float modifier) {
    CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io->theme->frontColor1), .fontSize = static_cast<uint16_t>(io->fontSize * modifier)}));
}

void GUIManager::text_label_color(std::string_view val, const SkColor4f& color) {
    CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(color), .fontSize = io->fontSize }));
}

void GUIManager::text_label_light(std::string_view val) {
    CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io->theme->frontColor2), .fontSize = io->fontSize }));
}

void GUIManager::text_label(std::string_view val) {
    CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io->theme->frontColor1), .fontSize = io->fontSize }));
}

void GUIManager::text_label_centered(std::string_view val) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        }
    }) {
        CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io->theme->frontColor1), .fontSize = io->fontSize, .textAlignment = CLAY_TEXT_ALIGN_CENTER}));
    }
}

void GUIManager::text_label_light_centered(std::string_view val) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        }
    }) {
        CLAY_TEXT(strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io->theme->frontColor2), .fontSize = io->fontSize, .textAlignment = CLAY_TEXT_ALIGN_CENTER}));
    }
}

bool GUIManager::radio_button(const char* id, bool val, const std::function<void()>& elemUpdate) {
    push_id(id);
    RadioButton* e = insert_element<RadioButton>(); 
    e->update(*io, val, elemUpdate);
    pop_id();
    return e->selection.clicked;
}

void GUIManager::checkbox(const char* id, bool* val, const std::function<void()>& elemUpdate) {
    push_id(id);
    CheckBox* e = insert_element<CheckBox>(); 
    e->update(*io, *val, elemUpdate);
    if(e->selection.clicked)
        *val = !(*val);
    pop_id();
}

bool GUIManager::input_text_field(const char* id, std::string_view name, std::string* val, bool updateEveryEdit, const std::function<void(SelectionHelper&)>& elemUpdate) {
    bool toRet = false;
    left_to_right_line_layout([&]() {
        text_label(name);
        toRet = input_text(id, val, updateEveryEdit, elemUpdate);
    });
    return toRet;
}

void GUIManager::input_path_field(const char* id, std::string_view name, std::filesystem::path* val, std::filesystem::file_type fileTypeRestriction, const std::function<void(SelectionHelper&)>& elemUpdate) {
    left_to_right_line_layout([&]() {
        text_label(name);
        input_path(id, val, fileTypeRestriction, elemUpdate);
    });
}

void GUIManager::checkbox_field(const char* id, std::string_view name, bool* val, const std::function<void()>& elemUpdate) {
    left_to_right_line_layout([&]() {
        checkbox(id, val, elemUpdate);
        text_label(name);
    });
}

bool GUIManager::radio_button_field(const char* id, std::string_view name, bool val, const std::function<void()>& elemUpdate) {
    bool toRet;
    left_to_right_line_layout([&]() {
        toRet = radio_button(id, val, elemUpdate);
        text_label(name);
    });
    return toRet;
}

bool GUIManager::input_color_component_255(const char* id, float* val, const std::function<void(SelectionHelper&)>& elemUpdate) {
    push_id(id);
    bool isUpdating = insert_element<TextBox<float>>()->update(*io, val,
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
    return isUpdating;
}

bool GUIManager::input_text(const char* id, std::string* val, bool updateEveryEdit, const std::function<void(SelectionHelper&)>& elemUpdate) {
    push_id(id);
    bool toRet = insert_element<TextBox<std::string>>()->update(*io, val,
        [&](const std::string& str) {
            return str;
        },
        [&](const std::string& str) {
            return str;
        }, true, updateEveryEdit, elemUpdate);
    pop_id();
    return toRet;
}

bool GUIManager::selectable_button(const char* id, const std::function<void(SelectionHelper&, bool)>& elemUpdate, GUIStuff::SelectableButton::DrawType drawType, bool isSelected) {
    push_id(id);
    SelectableButton* e = insert_element<SelectableButton>();
    e->update(*io, drawType, elemUpdate, isSelected);
    bool toRet = e->selection.clicked;
    pop_id();
    return toRet;
}

void GUIManager::svg_icon(const char* id, const std::string& svgPath, bool isHighlighted, const std::function<void()>& elemUpdate) {
    push_id(id);
    insert_element<SVGIcon>()->update(*io, svgPath, isHighlighted, elemUpdate);
    pop_id();
}

bool GUIManager::svg_icon_button(const char* id, const std::string& svgPath, bool isSelected, float size, bool hasBorder, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    push_id(id);
    CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size) } } }) {
        toRet = selectable_button("0", [&](SelectionHelper& s, bool is) {
            svg_icon("1", svgPath, is || s.held || s.hovered);
            if(elemUpdate)
                elemUpdate();
        }, SelectableButton::DrawType::FILLED, isSelected);
    }
    pop_id();
    return toRet;
}

bool GUIManager::svg_icon_button_transparent(const char* id, const std::string& svgPath, bool isSelected, float size, bool hasBorder, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    push_id(id);
    CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size) } } }) {
        toRet = selectable_button("0", [&](SelectionHelper& s, bool is) {
            svg_icon("1", svgPath, is || s.held || s.hovered);
            if(elemUpdate)
                elemUpdate();
        }, SelectableButton::DrawType::TRANSPARENT_ALL, isSelected);
    }
    pop_id();
    return toRet;
}

bool GUIManager::text_button_sized(const char* id, std::string_view text, Clay_SizingAxis x, Clay_SizingAxis y, bool isSelected, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    CLAY_AUTO_ID({.layout = {.sizing = {.width = x, .height = y } } }) {
        toRet = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            text_label(text);
            if(elemUpdate)
                elemUpdate();
        }, SelectableButton::DrawType::FILLED, isSelected);
    }
    return toRet;
}

bool GUIManager::text_button(const char* id, std::string_view text, bool isSelected, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) } } }) {
        toRet = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            text_label(text);
            if(elemUpdate)
                elemUpdate();
        }, SelectableButton::DrawType::FILLED, isSelected);
    }
    return toRet;
}

bool GUIManager::text_button_wide(const char* id, std::string_view text, bool isSelected, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        }}) {
        toRet = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            text_label(text);
            if(elemUpdate)
                elemUpdate();
        }, SelectableButton::DrawType::FILLED, isSelected);
    }
    return toRet;
}

bool GUIManager::text_button_left_transparent(const char* id, std::string_view text, bool isSelected, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
        }}) {
        toRet = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                }}) {
                text_label(text);
                if(elemUpdate)
                    elemUpdate();
            }
        }, SelectableButton::DrawType::TRANSPARENT_ALL, isSelected);
    }
    return toRet;
}

bool GUIManager::input_scalar(const char* id, uint8_t* val, uint8_t min, uint8_t max, int decimalPrecision, const std::function<void(SelectionHelper&)>& elemUpdate) {
    bool isUpdating = false;
    push_id(id);
    isUpdating = insert_element<TextBox<uint8_t>>()->update(*io, val, 
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
    return isUpdating;
}

void GUIManager::input_path(const char* id, std::filesystem::path* val, std::filesystem::file_type fileTypeRestriction, const std::function<void(SelectionHelper&)>& elemUpdate) {
    push_id(id);
    insert_element<TextBox<std::filesystem::path>>()->update(*io, val, 
        [&](const std::string& a) {
            std::filesystem::path toRet = std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(a.c_str()), a.length()));
            if(fileTypeRestriction != std::filesystem::file_type::none && std::filesystem::status(toRet).type() != fileTypeRestriction)
                return std::optional<std::filesystem::path>(std::nullopt);
            return std::optional<std::filesystem::path>(toRet);
        },
        [&](const std::filesystem::path& a) {
            return a.string();
        }, true, false, elemUpdate);
    pop_id();
}

void GUIManager::tab_list(const char* id, const std::vector<std::pair<std::string, std::string>>& tabNames, size_t& selectedTab, std::optional<size_t>& closedTab, const std::function<void()>& elemUpdate) {
    push_id(id);
    insert_element<MovableTabList>()->update(*io, tabNames, selectedTab, closedTab, elemUpdate);
    pop_id();
}

bool GUIManager::font_picker(const char* id, std::string* fontName) {
    push_id(id);
    bool toRet = insert_element<FontPicker>()->update(*io, fontName, this);
    pop_id();
    return toRet;
}

void GUIManager::dropdown_select(const char* id, size_t* val, const std::vector<std::string>& selections, float width, const std::function<void()>& hoverboxElemUpdate) {
    push_id(id);
    struct DropdownData {
        bool isOpen = false;
    };
    DropdownData& dropdownData = insert_any_with_id<DropdownData>(0, {});
    left_to_right_layout(CLAY_SIZING_FIXED(width), CLAY_SIZING_FIT(0), [&]() {
        bool click = selectable_button(id, [&](SelectionHelper& s, bool iS) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                    .padding = {.left = 4, .right = 4},
                    .childGap = io->theme->childGap1,
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }
            }) {
                text_label(selections[*val]);
                CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIT(SMALL_BUTTON_SIZE)}
                    }
                }) {
                    svg_icon("dropico", "data/icons/droparrow.svg", dropdownData.isOpen);
                }
            }
        }, SelectableButton::DrawType::FILLED, dropdownData.isOpen);
        if(dropdownData.isOpen) {
            Clay_ElementId localID = CLAY_ID_LOCAL("DROPDOWN");
            Clay_ElementData dropdownElemData = Clay_GetElementData(localID);
            float calculatedDropdownMaxHeight = 0.0f;
            if(dropdownElemData.found)
                calculatedDropdownMaxHeight = std::max(windowSize.y() - dropdownElemData.boundingBox.y - 2.0f, 0.0f);
            CLAY(localID, {
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(width), .height = CLAY_SIZING_FIT(0, calculatedDropdownMaxHeight)},
                    .childGap = 0
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
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
                    .color = convert_vec4<Clay_Color>(io->theme->fillColor2),
                    .width = CLAY_BORDER_OUTSIDE(1)
                }
            }) {
                obstructing_window();
                scroll_bar_many_entries_area("dropdown scroll area", 18.0f, selections.size(), true, [&](size_t i, bool) {
                    bool selectedEntry = *val == i;

                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_FIXED(18)},
                            .padding = CLAY_PADDING_ALL(0),
                            .childGap = 0,
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        }
                    }) {
                        SkColor4f entryColor;
                        if(selectedEntry)
                            entryColor = io->theme->backColor2;
                        else if(Clay_Hovered())
                            entryColor = io->theme->backColor2;
                        else
                            entryColor = io->theme->backColor1;
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                .padding = {.left = 2, .right = 2, .top = 0, .bottom = 0},
                                .childGap = 0,
                                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                            },
                            .backgroundColor = convert_vec4<Clay_Color>(entryColor)
                        }) {
                            text_label(selections[i]);
                            if(io->mouse.leftClick && Clay_Hovered()) {
                                *val = i;
                                dropdownData.isOpen = false;
                            }
                        }
                    }
                });
                if(hoverboxElemUpdate)
                    hoverboxElemUpdate();
            }
        }
        if(click)
            dropdownData.isOpen = !dropdownData.isOpen;
    });
    pop_id();
}

void GUIManager::scroll_bar_area(const char* id, bool clipHorizontal, const std::function<void(float, float, float&)>& elemUpdate, bool maxScrollBugWorkaround) {
    push_id(id);
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        }
    }) {
        struct ScrollAreaData {
            float currentScrollPos = 0.0f;
            bool isMoving = false;

            float scrollerStartPos;
            float mouseStartPos;

            float contentDimensions = 100.0f;
            float containerDimensions = 100.0f;
        };

        ScrollAreaData& sD = insert_any_with_id<ScrollAreaData>(0, {});

        Clay_ElementId localID = CLAY_ID_LOCAL("SCROLL_AREA");

        CLAY(localID, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .clip = {.horizontal = clipHorizontal, .vertical = true, .childOffset = {.x = 0, .y = Clay_GetScrollOffset().y}}
        }) {
            push_id(1);
            elemUpdate(sD.contentDimensions, sD.containerDimensions, sD.currentScrollPos);
            pop_id();
        }

        // Make sure you are accessing this data after the element has been created
        Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(localID);
        Clay_BoundingBox scrollAreaBB = Clay_GetElementData(localID).boundingBox;

        sD.contentDimensions = scrollData.contentDimensions.height;
        sD.containerDimensions = scrollData.scrollContainerDimensions.height;

        if(scrollData.contentDimensions.height > scrollData.scrollContainerDimensions.height) {
            float sAreaDim = scrollData.scrollContainerDimensions.height;
            float contDim = scrollData.contentDimensions.height;
            float scrollerSize = (sAreaDim / contDim) * sAreaDim;
            float scrollPosMax = (contDim - sAreaDim) + (maxScrollBugWorkaround ? 3.0f : 0.0f); //NOTE: Adding + 3.0f to prevent bug with scroller that pushes everything under it when its at the bottom of the scroll area. Only happens with some scroll areas
            float scrollerPos = std::fabs(scrollData.scrollPosition->y / scrollPosMax);
            float areaAboveScrollerSize = scrollerPos * (sAreaDim - scrollerSize);

            sD.currentScrollPos = scrollData.scrollPosition->y;

            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(12), .height = CLAY_SIZING_GROW(0)},
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor2)
            }) {
                SkColor4f scrollerColor;
                if(sD.isMoving)
                    scrollerColor = io->theme->fillColor1;
                else if(Clay_Hovered())
                    scrollerColor = io->theme->fillColor1;
                else
                    scrollerColor = io->theme->fillColor2;

                bool isHoveringOverScroller = false;
                CLAY_AUTO_ID({ 
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(areaAboveScrollerSize)}
                    }
                }) {}
                CLAY_AUTO_ID({
                    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(scrollerSize)}},
                    .backgroundColor = convert_vec4<Clay_Color>(scrollerColor),
                    .cornerRadius = CLAY_CORNER_RADIUS(3),
                }) {
                    if(Clay_Hovered())
                        isHoveringOverScroller = true;
                }
                CLAY_AUTO_ID({ .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                if(Clay_Hovered() && io->mouse.leftClick) {
                    sD.isMoving = true;
                    if(isHoveringOverScroller)
                        sD.scrollerStartPos = scrollAreaBB.y + areaAboveScrollerSize + scrollerSize * 0.5f;
                    else
                        sD.scrollerStartPos = std::clamp(io->mouse.pos.y(), scrollAreaBB.y + scrollerSize * 0.5f, scrollAreaBB.y + scrollAreaBB.height - scrollerSize * 0.5f);
                    sD.mouseStartPos = io->mouse.pos.y();
                }
                if(!io->mouse.leftHeld)
                    sD.isMoving = false;
                if(sD.isMoving) {
                    float newScrollPosFrac;
                    newScrollPosFrac = std::clamp((sD.scrollerStartPos - (sD.mouseStartPos - io->mouse.pos.y()) - scrollAreaBB.y - scrollerSize * 0.5f) / (scrollAreaBB.height - scrollerSize), 0.0f, 1.0f);
                    sD.currentScrollPos = newScrollPosFrac * (-scrollPosMax);
                }
                sD.currentScrollPos = std::clamp(sD.currentScrollPos, -scrollPosMax, 0.0f);
                scrollData.scrollPosition->y = sD.currentScrollPos;
            }
        }
        else
            sD.currentScrollPos = 0.0f;
    }
    pop_id();
}


void GUIManager::scroll_bar_many_entries_area(const char* id, float entryHeight, size_t entryCount, bool clipHorizontal, const std::function<void(size_t, bool)>& entryUpdate, const std::function<void(float, float, float&)>& elemUpdate) {
    push_id(id);
    scroll_bar_area(id, clipHorizontal, [&](float scrollContentHeight, float containerHeight, float& scrollAmount) {
        if(elemUpdate)
            elemUpdate(scrollContentHeight, containerHeight, scrollAmount);

        bool listHovered = Clay_Hovered();

        float absScrollAmount = std::fabs(scrollAmount);
        size_t startPoint = absScrollAmount / entryHeight;
        size_t elementsContainable = (containerHeight / entryHeight) + 2; // Displaying 2 more elements ensures that there isn't any empty space in the container
        size_t endPoint = std::min(entryCount, startPoint + elementsContainable);

        if(startPoint != 0) {
            CLAY_AUTO_ID({
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
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED((entryCount - endPoint) * entryHeight)},
                }
            }) { }
        }
    });
    pop_id();
}

bool GUIManager::rotate_wheel(const char* id, double* angle, float size, const std::function<void()>& elemUpdate) {
    bool toRet = false;
    push_id(id);
    CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size) } } }) {
        RotateWheel* e = insert_element<RotateWheel>();
        e->update(*io, angle, elemUpdate);
        toRet = e->selection.clicked;
    }
    pop_id();
    return toRet;
}

void GUIManager::text_paragraph(const char* id, std::unique_ptr<skia::textlayout::Paragraph> paragraph, float width, const std::function<void()>& elemUpdate) {
    push_id(id);
    TextParagraph* e = insert_element<TextParagraph>();
    e->update(*io, std::move(paragraph), width, elemUpdate);
    pop_id();
}

void GUIManager::paint_circle_popup_menu(const char* id, const Vector2f& centerPos, const PaintCircleMenu::Data& val, const std::function<void()>& elemUpdate) {
    float size = 300.0f;
    push_id(id);
    CLAY_AUTO_ID({.layout = { 
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

void GUIManager::list_popup_menu(const char* id, Vector2f popupPos, const std::function<void()>& elemUpdate) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        }
    }) {
        push_id(id);

        Clay_ElementId localID = CLAY_ID_LOCAL("LIST_POPUP");

        Clay_ElementData actionListElemData = Clay_GetElementData(localID);

        if(actionListElemData.found) {
            if((popupPos.y() + actionListElemData.boundingBox.height) > windowSize.y()) {
                popupPos.y() -= actionListElemData.boundingBox.height;
            }
        }

        CLAY(localID, {
            .layout = { 
                .sizing = {.width = CLAY_SIZING_FIT(100), .height = CLAY_SIZING_FIT(0)},
                .padding = CLAY_PADDING_ALL(io->theme->padding1),
                .childGap = 1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
            .floating = {
                .offset = {popupPos.x(), popupPos.y()},
                .zIndex = -1,
                .attachTo = CLAY_ATTACH_TO_ROOT
            }
        }) {
            obstructing_window();
            push_id(1);
            elemUpdate();
            pop_id();
        }
        pop_id();
    }
}

void GUIManager::tree_listing(const char* id, NetworkingObjects::NetObjID rootObjID, const TreeListing::DisplayData& displayData, TreeListing::SelectionData& selectionData) {
    push_id(id);
    insert_element<TreeListing>()->update(*io, *this, rootObjID, displayData, selectionData);
    pop_id();
}

void GUIManager::obstructing_window() {
    if(Clay_Hovered())
        io->hoverObstructed = true;
}

void GUIManager::clay_error_handler(Clay_ErrorData errorData) {
    Logger::get().log("INFO", "[Clay Error] " + std::string(errorData.errorText.chars));
}

void GUIManager::push_id(int64_t id) {
    idStack.emplace_back(id);
}

void GUIManager::push_id(const char* id) {
    idStack.emplace_back(id);
}

void GUIManager::pop_id() {
    idStack.pop_back();
}

}
