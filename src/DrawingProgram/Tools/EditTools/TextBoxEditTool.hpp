#pragma once
#include "../../../CanvasComponents/TextBoxCanvasComponent.hpp"
#include "DrawingProgramEditToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class TextBoxEditTool : public DrawingProgramEditToolBase {
    public:
        TextBoxEditTool(DrawingProgram& initDrawP);

        virtual void edit_start(EditTool& editTool,CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual void commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual bool edit_update(CanvasComponentContainer::ObjInfo* comp) override;
        virtual bool edit_gui(CanvasComponentContainer::ObjInfo* comp) override;
        virtual bool right_click_popup_gui(CanvasComponentContainer::ObjInfo* comp, Vector2f popupPos) override;
    private:
        struct TextBoxEditToolAllData {
            TextBoxCanvasComponent::Data textboxData;
            RichText::TextData richText;
        };

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

        uint8_t get_new_decoration_value();
        void set_styles_at_selection(TextBoxCanvasComponent& a);
        void add_undo_if_selecting_area(TextBoxCanvasComponent& a, const std::function<void()>& func);
        void add_undo(const std::function<void()>& func);

        void hold_undo_data(const std::string& undoName, TextBoxCanvasComponent& a);
        void release_undo_data(const std::string& undoName);

        std::unordered_map<std::string, std::pair<RichText::TextBox::Cursor, RichText::TextData>> undoHeldData;

        RichText::TextStyleModifier::ModifierMap currentMods;

        std::vector<std::string> sortedFontList;

        TextBoxEditToolAllData get_all_data(const TextBoxCanvasComponent& a);
};
