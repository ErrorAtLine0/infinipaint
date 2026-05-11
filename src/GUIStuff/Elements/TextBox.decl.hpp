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
#include "Element.hpp"
#include "../../RichText/TextBox.hpp"
#include <limits>
#include <modules/skparagraph/include/TextStyle.h>
#include "../../FontData.hpp"
#include "../../InputManager.hpp"
#include "../../RichTextUserInput.hpp"

namespace GUIStuff {

template <typename T> struct TextBoxData {
    T* data = nullptr;
    std::function<std::optional<T>(const std::string&)> fromStr;
    std::function<std::string(const T&)> toStr;
    bool singleLine = true;
    bool immutable = false;
    InputManager::TextInputProperties textInputProps;
    std::string emptyText;
    std::function<void()> onEdit;
    std::function<void()> onEnter;
    std::function<void()> onSelect;
    std::function<void()> onDeselect;
};

template <typename T> class TextBox : public Element {
    public:
        TextBox(GUIManager& gui);

        void layout(const Clay_ElementId& id, const TextBoxData<T>& userInfo);
        void select();
        virtual void update() override;
        virtual void deselect() override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste) override;
        virtual void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) override;
        virtual void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) override;
        virtual std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info() override;
        ~TextBox();
    private:
        void populate_empty_textbox();
        void after_text_input_callback();
        void init_textbox(UpdateInputData& io);
        void reset_textbox_text();
        bool update_data();
        bool is_selected() const;

        struct OldDisplayData {
            bool isSelected = false;
            std::string textboxStr;
            RichText::TextBox::Cursor cur;
        } oldDD;

        bool isEmptyText = false;
        bool isHeld = false;

        std::unique_ptr<RichTextUserInput> edit;

        std::optional<T> oldData;
        std::string oldEmptyText;
        TextBoxData<T> userInfo;
        std::shared_ptr<RichText::TextBox> textbox;
        std::shared_ptr<RichText::TextBox::Cursor> cur;
};

}
