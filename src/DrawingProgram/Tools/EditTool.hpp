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
#include "DrawingProgramToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <any>
#include "EditTools/DrawingProgramEditToolBase.hpp"

class DrawingProgram;

class EditTool : public DrawingProgramToolBase {
    public:
        struct HandleData {
            Vector2f* p;
            Vector2f* min;
            Vector2f* max;
            float minimumDistanceBetweenMinAndPoint = MINIMUM_DISTANCE_BETWEEN_BOUNDS;
            float minimumDistanceBetweenMaxAndPoint = MINIMUM_DISTANCE_BETWEEN_BOUNDS;
            Affine2f coordMatrix = Affine2f::Identity();
        };

        EditTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox(Toolbar& t) override;
        virtual void gui_phone_toolbox(PhoneDrawingProgramScreen& t) override;
        virtual void right_click_popup_gui(Toolbar& t, Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual Vector4f* color_picker_color(Vector4f* oldColor) override;
        virtual bool phone_gui_tool_specific_bottom_toolbar_exists() override;
        virtual void phone_gui_tool_specific_bottom_toolbar(PhoneDrawingProgramScreen& t) override;
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste) override;
        virtual void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text) override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info() override;
        ~EditTool();

        void add_point_handle(const HandleData& handle);
        void edit_start(CanvasComponentContainer::ObjInfo* comp, bool initUndoAfterEditDone = true);
        bool is_editable(CanvasComponentContainer::ObjInfo* comp);

        std::unique_ptr<DrawingProgramEditToolBase> compEditTool;
        std::vector<HandleData> pointHandles;
        CanvasComponentContainer::ObjInfo* objInfoBeingEdited = nullptr;
        HandleData* pointDragging = nullptr;
        std::any prevData;
        bool undoAfterEditDone;

        std::unique_ptr<CanvasComponent> oldData;
};
