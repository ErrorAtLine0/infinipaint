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

#pragma once
#include "CustomEvents.hpp"
#include "RichText/TextBox.hpp"
#include "UndoManager.hpp"
#include "InputManager.hpp"

class RichTextUserInput {
    public:
        RichTextUserInput(CustomEvents::InputTextBoxID initId, const std::shared_ptr<RichText::TextBox>& initTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& initCursor, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& initModMap);
        struct Changes {
            bool textEdited = false;
            bool cursorChanged = false;
        };
        Changes input_key_callback(InputManager& input, const InputManager::KeyCallbackArgs& key);
        bool input_paste_callback(const CustomEvents::PasteEvent& paste);
        Changes input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput);
        void add_text_to_textbox(const std::string& inputText);
        void do_textbox_operation_with_undo(const std::function<void()>& func);
        void add_textbox_undo(const RichText::TextBox::Cursor& prevCursor, const RichText::TextData& prevRichText);
        void process_mouse_left_button(const Vector2f& pos, int clickCount, bool held, bool shift);
        void input_finger_touch_down(const Vector2f& pos);
        void input_finger_held_motion(const Vector2f& pos);
        void android_force_update_modmap();
        void android_force_update_textbox_and_cursor();
        void android_force_update_cursor();
        const CustomEvents::InputTextBoxID id;
    private:
        std::shared_ptr<RichText::TextBox> textBox;
        std::shared_ptr<RichText::TextBox::Cursor> cursor;
        std::shared_ptr<RichText::TextStyleModifier::ModifierMap> modMap;
        UndoManager textboxUndo;
};
