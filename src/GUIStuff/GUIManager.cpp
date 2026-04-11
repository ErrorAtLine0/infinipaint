#include "GUIManager.hpp"
#include <include/core/SkBlendMode.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkPath.h>
#include <include/core/SkPathBuilder.h>
#include "Elements/GUIStuffHelpers.hpp"
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

void GUIManager::calculate_new_clip_rect(std::vector<SCollision::AABB<float>>& clipRectStack, std::optional<SCollision::AABB<float>>& clipRect, bool& clipNoDraw) {
    if(clipRectStack.empty()) {
        clipNoDraw = false;
        clipRect = std::nullopt;
        return;
    }
    SCollision::AABB<float> toRet = clipRectStack.front();
    for(size_t i = 1; i < clipRectStack.size(); i++) {
        if(!SCollision::collide(toRet, clipRectStack[i])) {
            clipNoDraw = true;
            clipRect = std::nullopt;
            return;
        }
        toRet = toRet.get_intersection_between_aabbs(clipRectStack[i]);
    }
    clipNoDraw = false;
    clipRect = toRet;
}

void GUIManager::clip_rect_transform(SkCanvas* canvas, std::vector<SCollision::AABB<float>>& clipRectStack, std::optional<SCollision::AABB<float>>& clipRect, bool& clipNoDraw) {
    canvas->save();
    // Calculating clip rect before transform should resolve the 1 pixel edge bug
    calculate_new_clip_rect(clipRectStack, clipRect, clipNoDraw);
    if(clipRect.has_value()) {
        SCollision::AABB<float> c = clipRect.value();
        c.min += io.windowPos * io.guiScaleMultiplier;
        c.max += io.windowPos * io.guiScaleMultiplier;
        c.min *= io.guiScaleMultiplier;
        c.max *= io.guiScaleMultiplier;
        canvas->clipIRect(c.get_sk_irect());
    }
    canvas->scale(io.guiScaleMultiplier, io.guiScaleMultiplier);
    canvas->translate(io.windowPos.x(), io.windowPos.y());
}

void GUIManager::draw(SkCanvas* c, bool skiaAA) {
    if(io.redrawSurface) {
        oldRenderCommandMap.clear();
        setToUpdateInvalidateDrawAreaFromLayout = false;
    }
    update_invalidated_draw_area_from_layout();
    if(io.redrawSurface || invalidDrawBB.has_value()) {
        SkCanvas* canvas = io.surface->getCanvas();

        std::optional<SCollision::AABB<float>> clipRect;
        std::vector<SCollision::AABB<float>> clipRectStack;
        bool clipNoDraw = false;

        if(!io.redrawSurface)
            clipRectStack.emplace_back(invalidDrawBB.value());

        clip_rect_transform(canvas, clipRectStack, clipRect, clipNoDraw);

        canvas->clear({0.0f, 0.0f, 0.0f, 0.0f});

        for(size_t i = 0; i < static_cast<size_t>(renderCommands.length); i++) {
            Clay_RenderCommand* command = Clay_RenderCommandArray_Get(&renderCommands, i);
            Clay_BoundingBox bb = command->boundingBox;

            if(command->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_START && command->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_END) {
                if(clipNoDraw || (clipRect.has_value() && !SCollision::collide(get_invalid_draw_bb_from_command(command), clipRect.value())))
                    continue;
            }

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
                        bb.width = 30000.0f;
                    }
                    if(!clip->vertical) {
                        bb.y = 0.0f;
                        bb.height = 30000.0f;
                    }

                    bb.x -= 1;
                    bb.y -= 1;
                    bb.width += 1;
                    bb.height += 1;
                    canvas->restore();
                    clipRectStack.emplace_back(clay_bounding_box_to_aabb(bb));
                    clip_rect_transform(canvas, clipRectStack, clipRect, clipNoDraw);
                    break;
                }
                case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                    canvas->restore();
                    clipRectStack.pop_back();
                    clip_rect_transform(canvas, clipRectStack, clipRect, clipNoDraw);
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

    invalidDrawBB = std::nullopt;
    io.redrawSurface = false;

    c->drawImage(io.surface->makeTemporaryImage(), 0, 0, {SkFilterMode::kNearest, SkMipmapMode::kNone}, nullptr);
}

SCollision::AABB<float> GUIManager::get_invalid_draw_bb_from_command(const Clay_RenderCommand* command) {
    BorderData extraPadding;
    if(command->commandType == CLAY_RENDER_COMMAND_TYPE_BORDER) {
        const Clay_BorderRenderData& border = command->renderData.border;
        extraPadding.top = border.width.top;
        extraPadding.bottom = border.width.bottom;
        extraPadding.left = border.width.left;
        extraPadding.right = border.width.right;
    }
    return get_invalid_draw_bb(clay_bounding_box_to_aabb(command->boundingBox), extraPadding);
}

SCollision::AABB<float> GUIManager::get_invalid_draw_bb(SCollision::AABB<float> bb, const BorderData& extraPadding) {
    constexpr float EXTRA_AREA = 3.0f;

    bb.min.y() -= EXTRA_AREA + extraPadding.top;
    bb.max.y() += EXTRA_AREA + extraPadding.bottom;
    bb.min.x() -= EXTRA_AREA + extraPadding.left;
    bb.max.x() += EXTRA_AREA + extraPadding.right;

    return bb;
}

void GUIManager::update_invalidated_draw_area_from_layout() {
    if(setToUpdateInvalidateDrawAreaFromLayout) {
        auto renderCommandMapCopy = oldRenderCommandMap;
        for(int32_t i = 0; i < renderCommands.length; i++) {
            const Clay_RenderCommand& renderCommand = renderCommands.internalArray[i];
            auto oldCommandIt = oldRenderCommandMap.find(renderCommand.id);
            if(oldCommandIt != oldRenderCommandMap.end()) {
                Clay_RenderCommand& oldRenderCommand = oldCommandIt->second;
                if(std::memcmp(&renderCommand, &oldRenderCommand, sizeof(Clay_RenderCommand)) != 0) {
                    invalidate_draw_in_area(get_invalid_draw_bb_from_command(&oldRenderCommand));
                    invalidate_draw_in_area(get_invalid_draw_bb_from_command(&renderCommand));
                    oldRenderCommand = renderCommand;
                }
            }
            else {
                invalidate_draw_in_area(get_invalid_draw_bb_from_command(&renderCommand));
                oldRenderCommandMap.emplace(renderCommand.id, renderCommand);
            }
            renderCommandMapCopy.erase(renderCommand.id);
        }
        for(auto& [id, command] : renderCommandMapCopy) {
            invalidate_draw_in_area(get_invalid_draw_bb_from_command(&command));
            oldRenderCommandMap.erase(id);
        }
        setToUpdateInvalidateDrawAreaFromLayout = false;
    }
}

void GUIManager::update() {
    for(ElementContainer* e : orderedElements)
        e->elem->update();
}

void GUIManager::update_window(const Vector2f& windowPos, const Vector2f& windowSize, float guiScaleMultiplier) {
    if(io.windowPos != windowPos || io.windowSize != windowSize || io.guiScaleMultiplier != guiScaleMultiplier) {
        io.redrawSurface = true;
        set_to_layout();
    }
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
        setToUpdateInvalidateDrawAreaFromLayout = true;
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

void GUIManager::invalidate_draw_element(Element* element, const BorderData& extraPadding) {
    if(element->get_bb().has_value())
        invalidate_draw_in_area(get_invalid_draw_bb(element->get_bb().value(), extraPadding));
}

void GUIManager::invalidate_draw_in_area(const SCollision::AABB<float>& bb) {
    if(invalidDrawBB.has_value())
        invalidDrawBB.value().include_aabb_in_bounds(bb);
    else 
        invalidDrawBB = bb;
}

bool GUIManager::cursor_obstructed() const {
    return cursorObstructed;
}

void GUIManager::deselect_all() {
    for(ElementContainer* e : orderedElements)
        e->elem->deselect();
}

void GUIManager::input_paste_callback(const CustomEvents::PasteEvent& paste) {
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
