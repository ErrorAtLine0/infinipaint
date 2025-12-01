#pragma once
#include "../DrawComponents/DrawTextBox.hpp"
#include "DrawingProgramToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class TextBoxTool : public DrawingProgramToolBase {
    public:
        TextBoxTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual bool right_click_popup_gui(Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual bool prevent_undo_or_redo() override;
    private:
        void edit_text(std::function<void()> toRun, const std::shared_ptr<DrawTextBox>& textBox);
        void update_textbox_network(const std::shared_ptr<DrawTextBox>& textBox);
        void commit();

        struct TextBoxControls {
            Vector2f startAt;
            Vector2f endAt;
            int drawStage = 0;
            std::shared_ptr<DrawTextBox> intermediateItem;
        } controls;
};
