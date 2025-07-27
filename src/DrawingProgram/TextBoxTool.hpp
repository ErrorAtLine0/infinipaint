#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../DrawComponents/DrawTextBox.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class TextBoxTool {
    public:
        TextBoxTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void reset_tool();

        void edit_start(const std::shared_ptr<DrawTextBox>& a, std::any& prevData);
        void commit_edit_updates(const std::shared_ptr<DrawTextBox>& a, std::any& prevData);
        bool edit_update(const std::shared_ptr<DrawTextBox>& a);
        bool edit_gui(const std::shared_ptr<DrawTextBox>& a);

        bool prevent_undo_or_redo();
    private:
        void edit_text(std::function<void()> toRun, const std::shared_ptr<DrawTextBox>& textBox);
        void update_textbox_network(const std::shared_ptr<DrawTextBox>& textBox);
        void commit();

        struct TextBoxControls {
            float textRelativeSize = 20.0f;
            Vector2f startAt;
            Vector2f endAt;
            int drawStage = 0;
            std::shared_ptr<DrawTextBox> intermediateItem;
        } controls;

        DrawingProgram& drawP;
};
