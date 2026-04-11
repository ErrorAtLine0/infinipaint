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
        virtual bool edit_update() override;
        virtual void edit_gui() override;
        virtual void right_click_popup_gui(Vector2f popupPos) override;

        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text) override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint) override;
    private:
        std::unique_ptr<RichTextUserInput> userInput;
        std::optional<unsigned> textboxID;

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

        void commit_update_func();
        void commit_update_and_layout_func();

        std::unordered_map<std::string, std::pair<RichText::TextBox::Cursor, RichText::TextData>> undoHeldData;

        std::shared_ptr<RichText::TextStyleModifier::ModifierMap> currentModsPtr = std::make_shared<RichText::TextStyleModifier::ModifierMap>();

        std::vector<std::string> sortedFontList;

        TextBoxEditToolAllData get_all_data(const TextBoxCanvasComponent& a);
};
