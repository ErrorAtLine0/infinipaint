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
#include "../../../CanvasComponents/TextBoxCanvasComponent.hpp"
#include "DrawingProgramEditToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <any>
#include "../../../RichTextUserInput.hpp"

class DrawingProgram;

class TextBoxEditTool : public DrawingProgramEditToolBase {
    public:
        TextBoxEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp);

        virtual void edit_start(EditTool& editTool, std::any& prevData) override;
        virtual void commit_edit_updates(std::any& prevData) override;
        virtual void edit_update() override;
        virtual void edit_gui(Toolbar& t) override;
        virtual void gui_phone_toolbox(PhoneDrawingProgramScreen& t) override;
        virtual void right_click_popup_gui(Toolbar& t, Vector2f popupPos) override;
        virtual Vector4f* color_picker_color(Vector4f* oldColor) override;
        virtual bool phone_gui_tool_specific_bottom_toolbar_exists() override;
        virtual void phone_gui_tool_specific_bottom_toolbar(PhoneDrawingProgramScreen& t) override;

        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste) override;
        virtual void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text) override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint) override;
        virtual std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info() override;
    private:
        struct PhoneGUIData {
            enum class PopupType {
                NONE,
                TEXT_COLOR,
                HIGHLIGHT_COLOR
            } popupType = PopupType::NONE;
            PhoneDrawingProgramScreen::ColorSelectorData colorSelectorData;
        } phoneGUIData;

        std::unique_ptr<RichTextUserInput> userInput;

        struct TextBoxEditToolAllData {
            TextBoxCanvasComponent::Data textboxData;
            RichText::TextData richText;
        };

        bool fontPickerIsOpen = false;
        uint32_t newFontSize = 15;
        Vector4f newTextColor{1.0f, 1.0f, 1.0f, 1.0f};
        Vector4f newHighlightColor{1.0f, 1.0f, 1.0f, 0.0f};
        bool newIsBold = false;
        bool newIsItalic = false;
        bool newIsUnderlined = false;
        bool newIsLinethrough = false;
        bool newIsOverline = false;
        std::string newFontName;
        RichText::ParagraphStyleData currentPStyle;

        void phone_bottom_toolbar_gui(PhoneDrawingProgramScreen& t);

        uint8_t get_new_decoration_value();
        void set_styles_at_selection(TextBoxCanvasComponent& a);
        void add_undo_if_selecting_area(TextBoxCanvasComponent& a, const std::function<void()>& func);
        void add_undo(const std::function<void()>& func);

        void hold_undo_data(const std::string& undoName, TextBoxCanvasComponent& a);
        void release_undo_data(const std::string& undoName);

        void commit_update_func_no_android_update();
        void commit_update_and_layout_func_no_android_update();
        void commit_update_func_and_android_update();
        void commit_update_and_layout_func_and_android_update();

        std::unordered_map<std::string, std::pair<RichText::TextBox::Cursor, RichText::TextData>> undoHeldData;

        std::shared_ptr<RichText::TextStyleModifier::ModifierMap> currentModsPtr = std::make_shared<RichText::TextStyleModifier::ModifierMap>();

        TextBoxEditToolAllData get_all_data(const TextBoxCanvasComponent& a);
};
