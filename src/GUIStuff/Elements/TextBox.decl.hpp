#pragma once
#include "Element.hpp"
#include "../../RichText/TextBox.hpp"
#include <limits>
#include <modules/skparagraph/include/TextStyle.h>
#include "../../FontData.hpp"
#include "../../InputManager.hpp"
#include "../../RichTextUserInput.hpp"

namespace GUIStuff {

template <typename T> struct TextBoxData {
    T* data = nullptr;
    std::function<std::optional<T>(const std::string&)> fromStr;
    std::function<std::string(const T&)> toStr;
    bool singleLine = true;
    bool immutable = false;
    InputManager::TextInputProperties textInputProps;
    std::function<void()> onEdit;
    std::function<void()> onEnter;
    std::function<void()> onSelect;
    std::function<void()> onDeselect;
};


struct TextEditData {
    TextEditData(unsigned initTextboxInputID, const std::shared_ptr<RichText::TextBox>& initTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& initCursor):
        textboxInputID(initTextboxInputID),
        userInput(initTextBox, initCursor, nullptr)
    {}
    unsigned textboxInputID;
    RichTextUserInput userInput;
};

template <typename T> class TextBox : public Element {
    public:
        TextBox(GUIManager& gui);

        void layout(const Clay_ElementId& id, const TextBoxData<T>& userInfo);
        void select();
        virtual void update() override;
        virtual void deselect() override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual void input_paste_callback(const CustomEvents::PasteEventData& paste) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        ~TextBox();
    private:
        void after_text_input_callback();
        void init_textbox(UpdateInputData& io);
        void reset_textbox_text();
        bool update_data();
        bool is_selected() const;

        struct OldDisplayData {
            bool isSelected = false;
            std::string textboxStr;
            RichText::TextBox::Cursor cur;
        } oldDD;

        bool isHeld = false;
        std::optional<TextEditData> edit;

        std::optional<T> oldData;
        TextBoxData<T> userInfo;
        std::shared_ptr<RichText::TextBox> textbox;
        std::shared_ptr<RichText::TextBox::Cursor> cur;
};

}
