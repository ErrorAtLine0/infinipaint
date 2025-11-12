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
            RichText::TextData richText;
        };

        uint32_t newFontSize = 15;
        Vector4f newTextColor{1.0f, 1.0f, 1.0f, 1.0f};
        bool newIsBold = false;
        bool newIsItalic = false;
        bool newIsUnderlined = false;
        bool newIsLinethrough = false;
        bool newIsOverline = false;
        std::string newFontName;
        ParagraphStyleData currentPStyle;

        uint8_t get_new_decoration_value();
        void set_styles_at_selection(const std::shared_ptr<DrawTextBox>& a);
        void add_undo_if_selecting_area(const std::shared_ptr<DrawTextBox>& a, const std::function<void()>& func);
        void add_undo(const std::function<void()>& func);

        void hold_undo_data(const std::string& undoName, const std::shared_ptr<DrawTextBox>& a);
        void release_undo_data(const std::string& undoName);

        std::unordered_map<std::string, std::pair<RichText::TextBox::Cursor, RichText::TextData>> undoHeldData;

        RichText::TextStyleModifier::ModifierMap currentMods;

        std::vector<std::string> sortedFontList;

        TextBoxEditToolAllData get_all_data(const std::shared_ptr<DrawTextBox>& a);
};
