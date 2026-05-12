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

#include "TextBox.decl.hpp"
#include "../GUIManager.hpp"
#include <SDL3/SDL_keyboard.h>
#include <Helpers/Logger.hpp>
#include "../../AndroidJNICalls.hpp"

namespace GUIStuff {

template <typename T> TextBox<T>::TextBox(GUIManager& gui): Element(gui) {}

template <typename T> void TextBox<T>::layout(const Clay_ElementId& id, const TextBoxData<T>& userInfo) {
    auto& io = gui.io;
    this->userInfo = userInfo;
    init_textbox(io);
    if(!oldData.has_value() || oldData.value() != *userInfo.data || (oldEmptyText != userInfo.emptyText)) {
        reset_textbox_text();
        populate_empty_textbox();
        gui.invalidate_draw_element(this);
    }

    if(edit) {
        SCollision::AABB<float> rect = {boundingBox.value().min * gui.io.guiScaleMultiplier, boundingBox.value().max * gui.io.guiScaleMultiplier};
        gui.io.input->update_textbox_rectangle(edit->id, rect);
    }

    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(static_cast<float>(io.fontSize * 2)), .height = CLAY_SIZING_FIXED(static_cast<float>(io.fontSize * 1.25f))}
        },
        .custom = { .customData = this }
    }) {
    }
}

template <typename T> void TextBox<T>::update() {
    std::string newTextboxStr = textbox->get_string();
    if(oldDD.isSelected != is_selected() || oldDD.textboxStr != newTextboxStr || oldDD.cur != *cur) {
        oldDD.isSelected = is_selected();
        oldDD.textboxStr = newTextboxStr;
        oldDD.cur = *cur;
        gui.invalidate_draw_element(this);
    }
}

template <typename T> void TextBox<T>::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    SkRect r = SkRect::MakeXYWH(bb.min.x(), bb.min.y(), bb.width(), bb.height());

    if(userInfo.decorations) {
        canvas->drawRect(r, SkPaint(io.theme->backColor2));
        SkPaint outline(is_selected() ? io.theme->fillColor1 : io.theme->backColor2);
        outline.setStroke(true);
        outline.setStrokeWidth(2.0f);
        outline.setAntiAlias(skiaAA);
        canvas->drawRoundRect(r, 2.0f, 2.0f, outline);
    }

    canvas->clipRect(r);

    float yOffset = bb.height() * 0.5f;
    yOffset -= textbox->get_height() * 0.5f;

    RichText::TextBox::PaintOpts paintOpts;
    if(is_selected()) {
        paintOpts.cursor = *cur;
        SkRect cursorRect = textbox->get_cursor_rect(cur->pos);
        canvas->translate(bb.min.x() - std::max(cursorRect.fRight - bb.width(), -2.0f), bb.min.y() + yOffset);
    }
    else
        canvas->translate(bb.min.x() + 2.0f, bb.min.y() + yOffset);
    paintOpts.cursorColor = {io.theme->fillColor1.fR, io.theme->fillColor1.fG, io.theme->fillColor1.fB};
    paintOpts.skiaAA = skiaAA;

    skia::textlayout::TextStyle tStyle;
    tStyle.setFontFamilies(io.fonts->get_default_font_families());
    tStyle.setFontSize(io.fontSize);
    tStyle.setForegroundPaint(SkPaint{io.theme->frontColor1});
    textbox->set_initial_text_style(tStyle);

    textbox->paint(canvas, paintOpts);

    canvas->restore();
}

template <typename T> void TextBox<T>::select() {
    if(!is_selected()) {
        if(isEmptyText)
            textbox->clear_text();
        edit = std::make_unique<RichTextUserInput>(CustomEvents::text_box_get_new_id(), textbox, cur, nullptr);
        CustomEvents::emit_event(CustomEvents::RefreshTextBoxInputEvent{});
    }
}

template <typename T> void TextBox<T>::deselect() {
    if(is_selected()) {
        edit = nullptr;
        if(userInfo.onDeselect) userInfo.onDeselect();
        CustomEvents::emit_event(CustomEvents::RefreshTextBoxInputEvent{});
        reset_textbox_text();
        populate_empty_textbox();
    }
}

template <typename T> bool TextBox<T>::is_selected() const {
    return edit != nullptr;
}

template <typename T> void TextBox<T>::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    if(is_selected() && edit->input_paste_callback(paste))
        after_text_input_callback();
}

template <typename T> void TextBox<T>::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {
    if(is_selected() && edit->input_android_text_box_input_callback(textboxInput).textEdited)
        after_text_input_callback();
}

template <typename T> void TextBox<T>::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(is_selected()) {
        if(key.key == InputManager::KEY_GENERIC_ENTER && key.down) {
            bool success = update_data();
            gui.set_post_callback_func_high_priority([&, success] {
                if(success && userInfo.onEdit) userInfo.onEdit();
                if(userInfo.onEnter) userInfo.onEnter();
                reset_textbox_text();
                if(is_selected()) edit->android_force_update_textbox_and_cursor();
            });
        }
        else {
            if(edit->input_key_callback(*gui.io.input, key).textEdited)
                after_text_input_callback();
        }
    }
}

template <typename T> void TextBox<T>::populate_empty_textbox() {
    if(!is_selected() && isEmptyText && !userInfo.emptyText.empty()) {
        RichText::TextData text;
        RichText::TextData::Paragraph& par = text.paragraphs.emplace_back();
        par.text = userInfo.emptyText;

        RichText::PositionedTextStyleMod& positionedMod = text.tStyleMods.emplace_back();
        positionedMod.pos = {0, 0};
        positionedMod.mods[RichText::TextStyleModifier::ModifierType::SLANT] = std::make_shared<RichText::SlantTextStyleModifier>(SkFontStyle::kItalic_Slant);
        positionedMod.mods[RichText::TextStyleModifier::ModifierType::COLOR] = std::make_shared<RichText::ColorTextStyleModifier>(convert_vec4<Vector4f>(gui.io.theme->frontColor2));

        textbox->set_rich_text_data(text);
    }
}

template <typename T> void TextBox<T>::after_text_input_callback() {
    if(userInfo.immutable)
        cur->pos = cur->selectionBeginPos = cur->selectionEndPos = textbox->insert({0, 0}, userInfo.toStr(*userInfo.data));
    else {
        if(update_data() && userInfo.onEdit)
            gui.set_post_callback_func_high_priority(userInfo.onEdit);
    }
}

template <typename T> void TextBox<T>::input_text_callback(const InputManager::TextCallbackArgs& text) {
    if(is_selected()) {
        edit->add_text_to_textbox(text.str);
        after_text_input_callback();
    }
}

template <typename T> void TextBox<T>::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT && boundingBox.has_value()) {
        if(button.down) {
            if(mouseHovering) {
                if(!is_selected()) {
                    select();
                    gui.set_post_callback_func_high_priority([&] {
                        if(userInfo.onSelect) userInfo.onSelect();
                    });
                }
                isHeld = true;
                edit->process_mouse_left_button(button.pos - boundingBox.value().min, button.clicks, isHeld, gui.io.input->key(InputManager::KEY_GENERIC_LSHIFT).held);
            }
            else if(is_selected()) {
                isHeld = false;
                deselect();
            }
        }
        else {
            isHeld = false;
            if(is_selected())
                edit->process_mouse_left_button(button.pos - boundingBox.value().min, 0, isHeld, gui.io.input->key(InputManager::KEY_GENERIC_LSHIFT).held);
        }
    }
}

template <typename T> void TextBox<T>::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(boundingBox.has_value() && is_selected())
        edit->process_mouse_left_button(motion.pos - boundingBox.value().min, 0, isHeld, gui.io.input->key(InputManager::KEY_GENERIC_LSHIFT).held);
}

template <typename T> void TextBox<T>::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
    if(boundingBox.has_value()) {
        if(touch.down) {
            if(mouseHovering) {
                if(!is_selected()) {
                    select();
                    gui.set_post_callback_func_high_priority([&] {
                        if(userInfo.onSelect) userInfo.onSelect();
                    });
                }
                isHeld = true;
                edit->input_finger_touch_down(touch.pos - boundingBox.value().min);
            }
            else if(is_selected()) {
                isHeld = false;
                deselect();
            }
        }
        else
            isHeld = false;
    }
}

template <typename T> void TextBox<T>::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
    if(boundingBox.has_value() && is_selected() && isHeld)
        edit->input_finger_held_motion(motion.pos - boundingBox.value().min);
}

template <typename T> std::optional<InputManager::TextBoxStartInfo> TextBox<T>::get_text_box_start_info() {
    if(edit) {
        SCollision::AABB<float> rect = {boundingBox.value().min * gui.io.guiScaleMultiplier, boundingBox.value().max * gui.io.guiScaleMultiplier};
        return InputManager::TextBoxStartInfo{
            .id = edit->id,
            .rect = rect,
            .inputProperties = userInfo.textInputProps,
            .textBox = textbox,
            .cursor = cur
        };
    }
    return std::nullopt;
}

template <typename T> void TextBox<T>::init_textbox(UpdateInputData& io) {
    if(!textbox) {
        textbox = std::make_shared<RichText::TextBox>();
        textbox->set_max_width(std::numeric_limits<float>::max());
        textbox->set_font_data(io.fonts);
        textbox->set_allow_newlines(false);
        cur = std::make_shared<RichText::TextBox::Cursor>();
    }
}

template <typename T> void TextBox<T>::reset_textbox_text() {
    std::string newStr = userInfo.toStr(*userInfo.data);
    isEmptyText = newStr.empty();
    textbox->set_string(newStr);
    cur->pos = textbox->move(RichText::TextBox::Movement::NOWHERE, cur->pos);
    cur->selectionBeginPos = textbox->move(RichText::TextBox::Movement::NOWHERE, cur->selectionBeginPos);
    cur->selectionEndPos = textbox->move(RichText::TextBox::Movement::NOWHERE, cur->selectionEndPos);
}

template <typename T> bool TextBox<T>::update_data() {
    std::optional<T> dataToAssign = userInfo.fromStr(textbox->get_string());
    if(dataToAssign.has_value()) {
        *userInfo.data = dataToAssign.value();
        oldData = dataToAssign;
        return true;
    }
    return false;
}

template <typename T> TextBox<T>::~TextBox() {
    if(is_selected()) {
        Logger::get().log("INFO", "Destroy");
        CustomEvents::emit_event(CustomEvents::RefreshTextBoxInputEvent{});
    }
}

}
