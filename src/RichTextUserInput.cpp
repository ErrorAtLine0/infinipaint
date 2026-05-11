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

#include "RichTextUserInput.hpp"
#include "CustomEvents.hpp"
#include "AndroidJNICalls.hpp"
#include "RichText/TextBox.hpp"

template <typename T> std::optional<T> shared_ptr_to_opt(const std::shared_ptr<T> o) {
    if(o)
        return *o;
    else
        return std::nullopt;
}

RichTextUserInput::RichTextUserInput(CustomEvents::InputTextBoxID initId, const std::shared_ptr<RichText::TextBox>& initTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& initCursor, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& initModMap):
    id(initId),
    textBox(initTextBox),
    cursor(initCursor),
    modMap(initModMap)
{}

void RichTextUserInput::android_force_update_modmap() {
#ifdef __ANDROID__
    AndroidJNICalls::updateModMap(id, modMap);
#endif
}

void RichTextUserInput::android_force_update_textbox_and_cursor() {
#ifdef __ANDROID__
    AndroidJNICalls::updateTextboxAndCursorPos(id, textBox, cursor);
#endif
    android_force_update_modmap();
}

void RichTextUserInput::android_force_update_cursor() {
#ifdef __ANDROID__
    AndroidJNICalls::updateCursorPos(id, cursor);
#endif
    android_force_update_modmap();
}

bool RichTextUserInput::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    if(paste.type == CustomEvents::PasteEvent::DataType::TEXT) {
        do_textbox_operation_with_undo([&]() {
            if(paste.richText.has_value())
                textBox->process_rich_text_input(*cursor, paste.richText.value());
            else
                textBox->process_text_input(*cursor, paste.data, shared_ptr_to_opt(modMap));
        });
        android_force_update_textbox_and_cursor();
        return true;
    }
    return false;
}

RichTextUserInput::Changes RichTextUserInput::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {
    RichTextUserInput::Changes toRet{false, false};
    switch(textboxInput.command) {
        case CustomEvents::AndroidTextBoxInputEvent::CommandType::COMMIT_ALL: {
            #ifdef __ANDROID__
                std::pair<bool, bool> c = AndroidJNICalls::getChangesToCommit(id);
                toRet.textEdited = c.first;
                toRet.cursorChanged = c.second;
                if(toRet.textEdited)
                    do_textbox_operation_with_undo([&]() {
                        AndroidJNICalls::commitAll(id, textBox, cursor);
                    });
                else 
                    AndroidJNICalls::commitAll(id, textBox, cursor);
            #else
                toRet.textEdited = toRet.cursorChanged = false;
            #endif
            break;
        }
    }
    return toRet;
}

void RichTextUserInput::add_text_to_textbox(const std::string& inputText) {
    do_textbox_operation_with_undo([&]() {
        textBox->process_text_input(*cursor, inputText, shared_ptr_to_opt(modMap));
    });
}

void RichTextUserInput::add_textbox_undo(const RichText::TextBox::Cursor& prevCursor, const RichText::TextData& prevRichText) {
    textboxUndo.push({[&, prevCursor = prevCursor, prevRichText = prevRichText]() {
        textBox->set_rich_text_data_for_undo_redo(prevRichText);
        cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = std::max(prevCursor.selectionEndPos, prevCursor.selectionBeginPos);
        cursor->previousX = std::nullopt;
        return true;
    },
    [&, currentCursor = *cursor, currentRichText = textBox->get_rich_text_data()]() {
        textBox->set_rich_text_data_for_undo_redo(currentRichText);
        cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = std::max(currentCursor.selectionEndPos, currentCursor.selectionBeginPos);
        cursor->previousX = std::nullopt;
        return true;
    }});
}

void RichTextUserInput::do_textbox_operation_with_undo(const std::function<void()>& func) {
    auto prevRichText = textBox->get_rich_text_data();
    auto prevCursor = *cursor;
    func();
    add_textbox_undo(prevCursor, prevRichText);
}

void RichTextUserInput::process_mouse_left_button(const Vector2f& pos, int clickCount, bool held, bool shift) {
    RichText::TextBox::Cursor oldCursor = *cursor;
    textBox->process_mouse_left_button(*cursor, pos, clickCount, held, shift);
    if(oldCursor != *cursor)
        android_force_update_cursor();
}

void RichTextUserInput::input_finger_touch_down(const Vector2f& pos) {
    RichText::TextBox::Cursor oldCursor = *cursor;
    cursor->pos = cursor->selectionBeginPos = cursor->selectionEndPos = textBox->get_text_pos_closest_to_point(pos);
    if(oldCursor != *cursor)
        android_force_update_cursor();
}

void RichTextUserInput::input_finger_held_motion(const Vector2f& pos) {
    RichText::TextBox::Cursor oldCursor = *cursor;
    cursor->pos = cursor->selectionEndPos = textBox->get_text_pos_closest_to_point(pos);
    if(oldCursor != *cursor)
        android_force_update_cursor();
}

RichTextUserInput::Changes RichTextUserInput::input_key_callback(InputManager& input, const InputManager::KeyCallbackArgs& key) {
    std::optional<RichText::TextStyleModifier::ModifierMap> modMapOpt = shared_ptr_to_opt(modMap);

    Changes toRet;

    if(key.down) {
        auto oldCursor = *cursor;

        switch(key.key) {
            case InputManager::KEY_GENERIC_UP:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::UP, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_GENERIC_DOWN:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::DOWN, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_GENERIC_LEFT:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::LEFT, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_GENERIC_RIGHT:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::RIGHT, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_TEXT_BACKSPACE:
                do_textbox_operation_with_undo([&] {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::BACKSPACE, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                });
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_DELETE:
                do_textbox_operation_with_undo([&] {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::DEL, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                });
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_HOME:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::HOME, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_TEXT_END:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::END, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_TEXT_COPY:
                input.set_clipboard_plain_and_richtext_pair(textBox->process_copy(*cursor));
                break;
            case InputManager::KEY_TEXT_CUT:
                do_textbox_operation_with_undo([&] {
                    input.set_clipboard_plain_and_richtext_pair(textBox->process_cut(*cursor));
                });
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_PASTE:
                input.call_paste(CustomEvents::PasteEvent::DataType::TEXT);
                break;
            case InputManager::KEY_TEXT_SELECTALL:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::SELECT_ALL, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_TEXT_UNDO:
                textboxUndo.undo();
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_REDO:
                textboxUndo.redo();
                toRet.textEdited = true;
                break;
            case InputManager::KEY_GENERIC_ENTER:
                do_textbox_operation_with_undo([&] {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::ENTER, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                });
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_TAB:
                do_textbox_operation_with_undo([&] {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::TAB, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                });
                toRet.textEdited = true;
                break;
            default:
                return toRet;
        }
        toRet.cursorChanged = oldCursor != *cursor;
        if(toRet.textEdited)
            android_force_update_textbox_and_cursor();
        else if(toRet.cursorChanged)
            android_force_update_cursor();
    }
    return toRet;
}
