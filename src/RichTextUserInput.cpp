#include "RichTextUserInput.hpp"
#include "CustomEvents.hpp"

template <typename T> std::optional<T> shared_ptr_to_opt(const std::shared_ptr<T> o) {
    if(o)
        return *o;
    else
        return std::nullopt;
}

RichTextUserInput::RichTextUserInput(const std::shared_ptr<RichText::TextBox>& initTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& initCursor, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& initModMap):
    textBox(initTextBox),
    cursor(initCursor),
    modMap(initModMap)
{}

bool RichTextUserInput::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    if(paste.type == CustomEvents::PasteEvent::DataType::TEXT) {
        do_textbox_operation_with_undo([&]() {
            if(paste.richText.has_value())
                textBox->process_rich_text_input(*cursor, paste.richText.value());
            else
                textBox->process_text_input(*cursor, paste.data, shared_ptr_to_opt(modMap));
        });
        return true;
    }
    return false;
}

void RichTextUserInput::add_text_to_textbox(const std::string& inputText) {
    do_textbox_operation_with_undo([&]() {
        textBox->process_text_input(*cursor, inputText, shared_ptr_to_opt(modMap));
    });
}

void RichTextUserInput::add_textbox_undo(const RichText::TextBox::Cursor& prevCursor, const RichText::TextData& prevRichText) {
    textboxUndo.push({[&, prevCursor = prevCursor, prevRichText = prevRichText]() {
        textBox->set_rich_text_data_for_undo_redo(prevRichText);
        cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = std::max(prevCursor.selectionEndPos, prevCursor.selectionBeginPos);
        cursor->previousX = std::nullopt;
        return true;
    },
    [&, currentCursor = *cursor, currentRichText = textBox->get_rich_text_data()]() {
        textBox->set_rich_text_data_for_undo_redo(currentRichText);
        cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = std::max(currentCursor.selectionEndPos, currentCursor.selectionBeginPos);
        cursor->previousX = std::nullopt;
        return true;
    }});
}

void RichTextUserInput::do_textbox_operation_with_undo(const std::function<void()>& func) {
    auto prevRichText = textBox->get_rich_text_data();
    auto prevCursor = *cursor;
    func();
    add_textbox_undo(prevCursor, prevRichText);
}

RichTextUserInput::Changes RichTextUserInput::input_key_callback(InputManager& input, const InputManager::KeyCallbackArgs& key) {
    std::optional<RichText::TextStyleModifier::ModifierMap> modMapOpt = shared_ptr_to_opt(modMap);

    Changes toRet;

    if(key.down) {
        auto oldCursor = *cursor;

        switch(key.key) {
            case InputManager::KEY_GENERIC_UP:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::UP, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_GENERIC_DOWN:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::DOWN, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_GENERIC_LEFT:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::LEFT, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_GENERIC_RIGHT:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::RIGHT, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_TEXT_BACKSPACE:
                do_textbox_operation_with_undo([&] {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::BACKSPACE, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                });
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_DELETE:
                do_textbox_operation_with_undo([&] {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::DEL, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                });
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_HOME:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::HOME, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_TEXT_END:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::END, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_TEXT_COPY:
                input.set_clipboard_plain_and_richtext_pair(textBox->process_copy(*cursor));
                break;
            case InputManager::KEY_TEXT_CUT:
                do_textbox_operation_with_undo([&] {
                    input.set_clipboard_plain_and_richtext_pair(textBox->process_cut(*cursor));
                });
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_PASTE:
                input.call_paste(CustomEvents::PasteEvent::DataType::TEXT);
                break;
            case InputManager::KEY_TEXT_SELECTALL:
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::SELECT_ALL, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                break;
            case InputManager::KEY_TEXT_UNDO:
                textboxUndo.undo();
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_REDO:
                textboxUndo.redo();
                toRet.textEdited = true;
                break;
            case InputManager::KEY_GENERIC_ENTER:
                do_textbox_operation_with_undo([&] {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::ENTER, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                });
                toRet.textEdited = true;
                break;
            case InputManager::KEY_TEXT_TAB:
                do_textbox_operation_with_undo([&] {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::TAB, input.ctrl_or_meta_held(), input.key(InputManager::KEY_GENERIC_LSHIFT).held, modMapOpt);
                });
                toRet.textEdited = true;
                break;
            default:
                return toRet;
        }
        toRet.cursorChanged = oldCursor != *cursor;
    }
    return toRet;
}
