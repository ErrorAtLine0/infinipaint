#pragma once
#include "../DrawComponents/DrawTextBox.hpp"
#include "DrawingProgramEditToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class TextBoxEditTool : public DrawingProgramEditToolBase {
    public:
        TextBoxEditTool(DrawingProgram& initDrawP);

        virtual void edit_start(EditTool& editTool, const std::shared_ptr<DrawComponent>& a, std::any& prevData) override;
        virtual void commit_edit_updates(const std::shared_ptr<DrawComponent>& a, std::any& prevData) override;
        virtual bool edit_update(const std::shared_ptr<DrawComponent>& a) override;
        virtual bool edit_gui(const std::shared_ptr<DrawComponent>& a) override;
    private:
        struct TextBoxEditToolAllData {
            DrawTextBox::Data textboxData;
            RichTextBox::RichTextData richText;
        };

        TextStyleModifier::ModifierMap modsAtStartOfSelection;

        TextBoxEditToolAllData get_all_data(const std::shared_ptr<DrawTextBox>& a);
};
