#pragma once
#include "Element.hpp"
#include "../../RichText/TextBox.hpp"
#include <limits>
#include <modules/skparagraph/include/TextStyle.h>
#include "../../FontData.hpp"
#include "../../InputManager.hpp"

namespace GUIStuff {

template <typename T> struct TextBoxData {
    T* data = nullptr;
    std::function<std::optional<T>(const std::string&)> fromStr;
    std::function<std::string(const T&)> toStr;
    bool singleLine = true;
    bool updateEveryEdit = false;
    bool immutable = false;
    InputManager::TextInputProperties textInputProps;
    std::function<void()> onEdit;
    std::function<void()> onEnter;
};

template <typename T> class TextBox : public Element {
    public:
        TextBox(GUIManager& gui);

        void layout(const TextBoxData<T>& userInfo);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual bool input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) override;
        virtual bool input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
        ~TextBox();
    private:
        void init_textbox(UpdateInputData& io);
        bool update_data();

        bool isSelected = false;
        TextBoxData<T> userInfo;
        std::shared_ptr<RichText::TextBox> textbox;
        std::shared_ptr<RichText::TextBox::Cursor> cur;
        std::shared_ptr<SCollision::AABB<float>> rect;
};

}
