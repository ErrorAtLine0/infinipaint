#include "TextBox.decl.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

template <typename T> TextBox<T>::TextBox(GUIManager& gui): Element(gui) {}

template <typename T> void TextBox<T>::layout(const Clay_ElementId& id, const TextBoxData<T>& userInfo) {
    auto& io = gui.io;
    this->userInfo = userInfo;
    init_textbox(io);
    if(!oldData.has_value() || oldData.value() != *userInfo.data)
        reset_textbox_text();

    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(static_cast<float>(io.fontSize * 2)), .height = CLAY_SIZING_FIXED(static_cast<float>(io.fontSize * 1.25f))}
        },
        .custom = { .customData = this }
    }) {
    }
}

template <typename T> void TextBox<T>::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    SkRect r = SkRect::MakeXYWH(bb.min.x(), bb.min.y(), bb.width(), bb.height());

    canvas->drawRect(r, SkPaint(io.theme->backColor2));
    SkPaint outline(is_selected() ? io.theme->fillColor1 : io.theme->backColor2);
    outline.setStroke(true);
    outline.setStrokeWidth(2.0f);
    outline.setAntiAlias(skiaAA);
    canvas->drawRoundRect(r, 2.0f, 2.0f, outline);

    canvas->clipRect(SkRect::MakeXYWH(bb.min.x(), bb.min.y(), bb.width(), bb.height()));

    float yOffset = bb.height() * 0.5f;
    yOffset -= textbox->get_height() * 0.5f;

    RichText::TextBox::PaintOpts paintOpts;
    if(is_selected()) {
        paintOpts.cursor = *cur;
        SkRect cursorRect = textbox->get_cursor_rect(cur->pos);
        canvas->translate(bb.min.x() - std::max(cursorRect.fRight - bb.width(), -2.0f), bb.min.y() + yOffset);
    }
    else
        canvas->translate(bb.min.x() + 2.0f, bb.min.y() + yOffset);
    paintOpts.cursorColor = {io.theme->fillColor1.fR, io.theme->fillColor1.fG, io.theme->fillColor1.fB};
    paintOpts.skiaAA = skiaAA;

    skia::textlayout::TextStyle tStyle;
    tStyle.setFontFamilies(io.fonts->get_default_font_families());
    tStyle.setFontSize(io.fontSize);
    tStyle.setForegroundPaint(SkPaint{io.theme->frontColor1});
    textbox->set_initial_text_style(tStyle);

    textbox->paint(canvas, paintOpts);

    canvas->restore();
}

template <typename T> bool TextBox<T>::is_selected() const {
    return edit.has_value();
}

template <typename T> void TextBox<T>::input_paste_callback(const CustomEvents::PasteEventData& paste) {
    if(is_selected() && edit.value().userInput.input_paste_callback(paste))
        after_text_input_callback();
}

template <typename T> void TextBox<T>::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(is_selected()) {
        if(key.key == InputManager::KEY_GENERIC_ENTER && key.down) {
            bool success = update_data();
            gui.set_post_callback_func_high_priority([&, success] {
                if(success && userInfo.onEdit) userInfo.onEdit();
                if(userInfo.onEnter) userInfo.onEnter();
                reset_textbox_text();
            });
        }
        else {
            if(edit.value().userInput.input_key_callback(*gui.io.input, key).textEdited)
                after_text_input_callback();
        }
    }
}

template <typename T> void TextBox<T>::after_text_input_callback() {
    if(userInfo.immutable)
        cur->pos = cur->selectionBeginPos = cur->selectionEndPos = textbox->insert({0, 0}, userInfo.toStr(*userInfo.data));
    else {
        if(update_data() && userInfo.onEdit)
            gui.set_post_callback_func_high_priority(userInfo.onEdit);
    }
}

template <typename T> void TextBox<T>::input_text_callback(const InputManager::TextCallbackArgs& text) {
    if(is_selected()) {
        edit.value().userInput.add_text_to_textbox(text.str);
        after_text_input_callback();
    }
}

template <typename T> void TextBox<T>::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    auto& io = gui.io;

    isHeld = false;

    if(button.button == InputManager::MouseButton::LEFT && boundingBox.has_value()) {
        if(button.down) {
            if(mouseHovering) {
                if(!is_selected()) {
                    SCollision::AABB<float> rect = {(boundingBox.value().min + io.windowPos) * io.guiScaleMultiplier, (boundingBox.value().max + io.windowPos) * io.guiScaleMultiplier};
                    unsigned id = gui.io.input->set_text_box_front(rect, userInfo.textInputProps);
                    edit = TextEditData(id, textbox, cur);
                    gui.set_post_callback_func_high_priority([&] {
                        if(userInfo.onSelect) userInfo.onSelect();
                    });
                }
                isHeld = true;
                textbox->process_mouse_left_button(*cur, button.pos - boundingBox.value().min, button.clicks, isHeld, gui.io.input->key(InputManager::KEY_GENERIC_LSHIFT).held);
            }
            else if(is_selected()) {
                gui.io.input->remove_text_box(edit.value().textboxInputID);
                gui.set_post_callback_func_high_priority([&] {
                    if(update_data())
                        if(userInfo.onEdit) userInfo.onEdit();
                    if(userInfo.onDeselect) userInfo.onDeselect();
                });
                reset_textbox_text();
                edit = std::nullopt;
            }
        }
        else {
            textbox->process_mouse_left_button(*cur, button.pos - boundingBox.value().min, 0, isHeld, gui.io.input->key(InputManager::KEY_GENERIC_LSHIFT).held);
        }
    }
}

template <typename T> void TextBox<T>::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(boundingBox.has_value())
        textbox->process_mouse_left_button(*cur, motion.pos - boundingBox.value().min, 0, isHeld, gui.io.input->key(InputManager::KEY_GENERIC_LSHIFT).held);
}

template <typename T> void TextBox<T>::init_textbox(UpdateInputData& io) {
    if(!textbox) {
        textbox = std::make_shared<RichText::TextBox>();
        textbox->set_width(std::numeric_limits<float>::max());
        textbox->set_font_data(io.fonts);
        textbox->set_allow_newlines(false);
        cur = std::make_shared<RichText::TextBox::Cursor>();
    }
}

template <typename T> void TextBox<T>::reset_textbox_text() {
    textbox->set_string(userInfo.toStr(*userInfo.data));
    cur->pos = textbox->move(RichText::TextBox::Movement::NOWHERE, cur->pos);
    cur->selectionBeginPos = textbox->move(RichText::TextBox::Movement::NOWHERE, cur->selectionBeginPos);
    cur->selectionEndPos = textbox->move(RichText::TextBox::Movement::NOWHERE, cur->selectionEndPos);
}

template <typename T> bool TextBox<T>::update_data() {
    std::optional<T> dataToAssign = userInfo.fromStr(textbox->get_string());
    if(dataToAssign.has_value()) {
        *userInfo.data = dataToAssign.value();
        oldData = dataToAssign;
        return true;
    }
    return false;
}

template <typename T> TextBox<T>::~TextBox() {
    if(is_selected())
        gui.io.input->remove_text_box(edit.value().textboxInputID);
}

}
