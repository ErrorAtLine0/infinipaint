#pragma once
#include "RichText/TextBox.hpp"
#include "UndoManager.hpp"
#include "InputManager.hpp"

class RichTextUserInput {
    public:
        RichTextUserInput(const std::shared_ptr<RichText::TextBox>& initTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& initCursor, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& initModMap, const std::function<void()>& initOnTextEdit);
        void input_key_callback(InputManager& input, const InputManager::KeyCallbackArgs& key);
        void add_text_to_textbox(const std::string& inputText);
    private:
        void do_textbox_operation_with_undo(const std::function<void()>& func);
        void add_textbox_undo(const RichText::TextBox::Cursor& prevCursor, const RichText::TextData& prevRichText);
        std::shared_ptr<RichText::TextBox> textBox;
        std::shared_ptr<RichText::TextBox::Cursor> cursor;
        std::shared_ptr<RichText::TextStyleModifier::ModifierMap> modMap;
        UndoManager textboxUndo;
        std::function<void()> onTextEdit;
};
