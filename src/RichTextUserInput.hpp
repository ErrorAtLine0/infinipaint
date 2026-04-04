#pragma once
#include "CustomEvents.hpp"
#include "RichText/TextBox.hpp"
#include "UndoManager.hpp"
#include "InputManager.hpp"

class RichTextUserInput {
    public:
        RichTextUserInput(const std::shared_ptr<RichText::TextBox>& initTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& initCursor, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& initModMap);
        struct Changes {
            bool textEdited = false;
            bool cursorChanged = false;
        };
        Changes input_key_callback(InputManager& input, const InputManager::KeyCallbackArgs& key);
        bool input_paste_callback(const CustomEvents::PasteEventData& paste);
        void add_text_to_textbox(const std::string& inputText);
        void do_textbox_operation_with_undo(const std::function<void()>& func);
        void add_textbox_undo(const RichText::TextBox::Cursor& prevCursor, const RichText::TextData& prevRichText);
    private:
        std::shared_ptr<RichText::TextBox> textBox;
        std::shared_ptr<RichText::TextBox::Cursor> cursor;
        std::shared_ptr<RichText::TextStyleModifier::ModifierMap> modMap;
        UndoManager textboxUndo;
};
