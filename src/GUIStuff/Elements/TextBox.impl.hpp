#include "TextBox.decl.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

template <typename T> TextBox<T>::TextBox(GUIManager& gui): Element(gui) {}

template <typename T> void TextBox<T>::layout(const TextBoxData<T>& userInfo) {
    auto& io = *gui.io;
    this->userInfo = userInfo;
    textbox->onUserTextEdit = [&] {
        if(userInfo.immutable)
            cur->pos = cur->selectionBeginPos = cur->selectionEndPos = textbox->insert({0, 0}, userInfo.toStr(*userInfo.data));
        else {
            if(update_data() && userInfo.onEdit)
                gui.set_post_callback_func(userInfo.onEdit);
        }
    };
    init_textbox(io);
    CLAY_AUTO_ID({
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
    SkPaint outline(isSelected ? io.theme->fillColor1 : io.theme->backColor2);
    outline.setStroke(true);
    outline.setStrokeWidth(2.0f);
    outline.setAntiAlias(skiaAA);
    canvas->drawRoundRect(r, 2.0f, 2.0f, outline);

    canvas->clipRect(SkRect::MakeXYWH(bb.min.x(), bb.min.y(), bb.width(), bb.height()));

    float yOffset = bb.height() * 0.5f;
    yOffset -= textbox->get_height() * 0.5f;

    RichText::TextBox::PaintOpts paintOpts;
    if(isSelected) {
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

template <typename T> bool TextBox<T>::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
    auto& io = *gui.io;

    if(button.button == InputManager::MouseButton::LEFT && boundingBox.has_value()) {
        if(button.down) {
            if(mouseHovering) {
                *rect = {(boundingBox.value().min + io.windowPos) * io.guiScaleMultiplier, (boundingBox.value().max + io.windowPos) * io.guiScaleMultiplier};
                gui.io->input->set_rich_text_box_input_front(textbox, cur, false, rect, userInfo.textInputProps, nullptr);
                textbox->process_mouse_left_button(*cur, button.pos - boundingBox.value().min, button.clicks, true, gui.io->input->key(InputManager::KEY_TEXT_SHIFT).held);
                isSelected = true;
            }
            else {
                gui.io->input->remove_rich_text_box_input(textbox);
                if(update_data())
                    gui.set_post_callback_func(userInfo.onEdit);
                init_textbox(*gui.io);
                isSelected = false;
            }
        }
        else {
            textbox->process_mouse_left_button(*cur, button.pos - boundingBox.value().min, 0, false, gui.io->input->key(InputManager::KEY_TEXT_SHIFT).held);
        }
    }
    return Element::input_mouse_button_callback(button, mouseHovering);
}

template <typename T> bool TextBox<T>::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) {
    if(boundingBox.has_value())
        textbox->process_mouse_left_button(*cur, motion.pos - boundingBox.value().min, 0, gui.io->input->mouse.leftDown, gui.io->input->key(InputManager::KEY_TEXT_SHIFT).held);
    return Element::input_mouse_motion_callback(motion, mouseHovering);
}

template <typename T> void TextBox<T>::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(key.key == InputManager::KEY_TEXT_ENTER && key.down && userInfo.onEnter) {
        bool success = update_data();
        gui.set_post_callback_func([&, success] {
            if(success && userInfo.onEdit) userInfo.onEdit();
            userInfo.onEnter();
        });
    }
}

template <typename T> void TextBox<T>::init_textbox(UpdateInputData& io) {
    textbox = std::make_shared<RichText::TextBox>();
    textbox->set_width(std::numeric_limits<float>::max());
    textbox->set_font_data(io.fonts);
    textbox->set_allow_newlines(false);

    cur = std::make_shared<RichText::TextBox::Cursor>();
    rect = std::make_shared<SCollision::AABB<float>>();

    cur->pos = cur->selectionBeginPos = cur->selectionEndPos = textbox->insert({0, 0}, userInfo.toStr(*userInfo.data));
}

template <typename T> bool TextBox<T>::update_data() {
    std::optional<T> dataToAssign = userInfo.fromStr(textbox->get_string());
    if(dataToAssign.has_value()) {
        *userInfo.data = dataToAssign.value();
        return true;
    }
    return false;
}

template <typename T> TextBox<T>::~TextBox() {
    gui.io->input->remove_rich_text_box_input(textbox);
}

}
