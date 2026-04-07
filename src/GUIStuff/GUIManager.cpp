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
    setToLayout = true;
    postCallbackFuncIsHighPriority = false;
}

Clay_Dimensions GUIManager::clay_skia_measure_text(Clay_StringSlice str, Clay_TextElementConfig* config, void* userData) {
    GUIManager* window = static_cast<GUIManager*>(userData);

    skia::textlayout::ParagraphStyle pStyle;
    pStyle.setTextAlign(skia::textlayout::TextAlign::kLeft);
    skia::textlayout::TextStyle tStyle;
    tStyle.setFontSize(config->fontSize);
    tStyle.setFontFamilies(window->io.fonts->get_default_font_families());
    pStyle.setTextStyle(tStyle);

    skia::textlayout::ParagraphBuilderImpl a(pStyle, window->io.fonts->collection, SkUnicodes::ICU::Make());
    a.addText(str.chars, str.length);
    std::unique_ptr<skia::textlayout::Paragraph> p = a.Build();
    p->layout(std::numeric_limits<float>::max());

    return Clay_Dimensions(p->getMaxIntrinsicWidth(), p->getHeight());
}

void GUIManager::draw(SkCanvas* canvas, bool skiaAA) {
    canvas->save();
    canvas->translate(io.windowPos.x(), io.windowPos.y());

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
                paint.setAntiAlias(skiaAA);
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
                tStyle.setFontFamilies(io.fonts->get_default_font_families());
                tStyle.setColor(convert_vec4<SkColor4f>(config->textColor).toSkColor());
                pStyle.setTextStyle(tStyle);

                skia::textlayout::ParagraphBuilderImpl a(pStyle, io.fonts->collection, SkUnicodes::ICU::Make());
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
                p.setAntiAlias(skiaAA);

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
                Element* customElement = static_cast<Element*>(command->renderData.custom.customData);
                customElement->clay_draw(canvas, io, command, skiaAA);
                break;
            }
            default:
                break;
        }
    }

    canvas->restore();
}

void GUIManager::update_window(const Vector2f& windowPos, const Vector2f& windowSize, float guiScaleMultiplier) {
    if(io.windowPos != windowPos || io.windowSize != windowSize || io.guiScaleMultiplier != guiScaleMultiplier)
        set_to_layout();
    io.windowPos = windowPos;
    io.windowSize = windowSize / guiScaleMultiplier;
    io.guiScaleMultiplier = guiScaleMultiplier;
}

void GUIManager::set_to_layout() {
    setToLayout = true;
}

void GUIManager::layout_if_necessary() {
    if(setToLayout) {
        layout();
        setToLayout = false;
    }
}

void GUIManager::layout() {
    constexpr int LAYOUT_RUN_COUNT = 4;
    for(int i = 0; i < LAYOUT_RUN_COUNT; i++)
        single_layout_run();
}

void GUIManager::layout_begin() {
    for(auto& [id, e] : elements)
        e.isUsedThisFrame = false;

    Clay_SetLayoutDimensions(Clay_Dimensions(io.windowSize.x(), io.windowSize.y()));
    Clay_SetCurrentContext(clayInstance);

    strArena.reset();

    orderedElements.clear();

    Clay_BeginLayout();
}

void GUIManager::layout_end() {
    renderCommands = Clay_EndLayout();
    if(!idStack.empty())
        throw std::runtime_error("[GUIManager::end] ID Stack is not empty on end (push_id and pop_id calls not equal)");
    std::erase_if(elements, [](auto& p) {
        return !p.second.isUsedThisFrame;
    });
    std::stable_sort(orderedElements.begin(), orderedElements.end(), [](ElementContainer* a, ElementContainer* b) {
        return b->elem->zIndex < a->elem->zIndex; // Order from highest to lowest zIndex
    });
}

void GUIManager::single_layout_run() {
    layout_begin();
    io.layoutRun();
    layout_end();
}

void GUIManager::clay_error_handler(Clay_ErrorData errorData) {
    Logger::get().log("INFO", "[Clay Error] " + std::string(errorData.errorText.chars));
}

void GUIManager::new_id(const char* id, const std::function<void()>& f) {
    push_id(id);
    f();
    pop_id();
}

void GUIManager::new_id(int64_t id, const std::function<void()>& f) {
    push_id(id);
    f();
    pop_id();
}

void GUIManager::set_z_index(int16_t z, const std::function<void()>& f) {
    auto oldClippingRegion = clippingRegion;
    clippingRegion = std::nullopt;
    set_z_index_keep_clipping_region(z, f);
    clippingRegion = oldClippingRegion;
}

void GUIManager::set_z_index_keep_clipping_region(int16_t z, const std::function<void()>& f) {
    int16_t oldZIndex = zIndex;
    zIndex = z;
    f();
    zIndex = oldZIndex;
}

int16_t GUIManager::get_z_index() {
    return zIndex;
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

void GUIManager::set_post_callback_func(const std::function<void()>& f) {
    if(!postCallbackFuncIsHighPriority)
        postCallbackFunc = f;
}

void GUIManager::set_post_callback_func_high_priority(const std::function<void()>& f) {
    postCallbackFuncIsHighPriority = true;
    postCallbackFunc = f;
}

void GUIManager::run_post_callback_func() {
    if(postCallbackFunc) {
        postCallbackFunc();
        postCallbackFunc = nullptr;
    }
    postCallbackFuncIsHighPriority = false;
}

bool GUIManager::cursor_obstructed() const {
    return cursorObstructed;
}

void GUIManager::input_paste_callback(const CustomEvents::PasteEventData& paste) {
    for(ElementContainer* e : orderedElements)
        e->elem->input_paste_callback(paste);
}

void GUIManager::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    for(ElementContainer* e : orderedElements)
        e->elem->input_text_key_callback(key);
}

void GUIManager::input_text_callback(const InputManager::TextCallbackArgs& text) {
    for(ElementContainer* e : orderedElements)
        e->elem->input_text_callback(text);
}

void GUIManager::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    for(ElementContainer* e : orderedElements)
        e->elem->input_key_callback(key);
}

void GUIManager::input_mouse_button_callback(InputManager::MouseButtonCallbackArgs button) {
    button.pos /= io.guiScaleMultiplier;
    mouse_callback(button.pos, [&button] (ElementContainer* e) { e->elem->input_mouse_button_callback(button); });
}

void GUIManager::input_mouse_motion_callback(InputManager::MouseMotionCallbackArgs motion) {
    motion.pos /= io.guiScaleMultiplier;
    mouse_callback(motion.pos, [&motion] (ElementContainer* e) { e->elem->input_mouse_motion_callback(motion); });
}

void GUIManager::input_mouse_wheel_callback(InputManager::MouseWheelCallbackArgs wheel) {
    wheel.mousePos /= io.guiScaleMultiplier;
    mouse_callback(wheel.mousePos, [&wheel] (ElementContainer* e) { e->elem->input_mouse_wheel_callback(wheel); });
}

void GUIManager::mouse_callback(const Vector2f& mousePos, const std::function<void(ElementContainer*)>& f) {
    cursorObstructed = false;
    int16_t zIndexObstructed = 0;

    for(ElementContainer* e : orderedElements)
        e->elem->childMouseHovering = false;

    for(ElementContainer* e : orderedElements) {
        if((!cursorObstructed || zIndexObstructed == e->elem->zIndex) && e->elem->collides_with_point(mousePos)) {
            zIndexObstructed = e->elem->zIndex;
            cursorObstructed = true;
            e->elem->mouseHovering = true;
            Element* nextParent = e->elem->parent;
            while(nextParent) {
                nextParent->childMouseHovering |= true;
                nextParent = nextParent->parent;
            }
        }
        else
            e->elem->mouseHovering = false;
    }

    for(ElementContainer* e : orderedElements)
        f(e);
}

}
