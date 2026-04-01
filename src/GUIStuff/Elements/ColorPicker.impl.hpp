#pragma once
#include "ColorPicker.decl.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

template <typename T> ColorPicker<T>::ColorPicker(GUIManager& gui): Element(gui) {}

template <typename T> void ColorPicker<T>::layout(const Clay_ElementId& id, T* data, bool selectAlpha, const std::function<void()>& onChange) {
    this->data = data;
    if(!oldData.has_value() || oldData.value() != *data) {
        savedHsv = rgb_to_hsv<Vector3f, T>(*data);
        oldData = *data;
    }
    this->selectAlpha = selectAlpha;
    this->onChange = onChange;

    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
        },
        .aspectRatio = {1.0f},
        .custom = { .customData = this }
    }) {}
}

template <typename T> void ColorPicker<T>::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());
    float svSelectionAreaSize = get_sv_selection_area_size();
    canvas->scale(svSelectionAreaSize, svSelectionAreaSize);
    canvas->clipRect(SkRect::MakeXYWH(0.0f, 0.0f, 1.0f, 1.0f), skiaAA);

    SkPaint svSelectionAreaPaint;
    svSelectionAreaPaint.setShader(ColorPickerShaders::get_sv_selection_shader(savedHsv.x() / 360.0f));
    canvas->drawPaint(svSelectionAreaPaint);

    SkPaint selectionLinePaint({1.0f, 1.0f, 1.0f, 1.0f});
    selectionLinePaint.setAntiAlias(skiaAA);
    selectionLinePaint.setStrokeWidth(2.0f / svSelectionAreaSize);
    canvas->drawLine(0.0f, 1.0f - savedHsv.z(), 1.0f, 1.0f - savedHsv.z(), selectionLinePaint);
    canvas->drawLine(savedHsv.y(), 0.0f, savedHsv.y(), 1.0f, selectionLinePaint);

    canvas->restore();

    float normalizedHue = savedHsv.x() / 360.0f;

    Vector2f hueBarPos = get_hue_bar_pos();
    Vector2f hueBarDim = get_hue_bar_dim();
    canvas->save();
    canvas->translate(hueBarPos.x(), hueBarPos.y());
    canvas->scale(hueBarDim.x(), hueBarDim.y());
    canvas->clipRect(SkRect::MakeXYWH(0.0f, 0.0f, 1.0f, 1.0f), skiaAA);

    SkPaint hueBarPaint;
    hueBarPaint.setShader(ColorPickerShaders::get_hue_shader());
    canvas->drawPaint(hueBarPaint);
    canvas->drawLine(0.0f, 1.0f - normalizedHue, 1.0f, 1.0f - normalizedHue, selectionLinePaint);

    canvas->restore();

    Vector2f alphaBarPos = get_alpha_bar_pos();
    Vector2f alphaBarDim = get_alpha_bar_dim();
    canvas->save();
    canvas->translate(alphaBarPos.x(), alphaBarPos.y());
    canvas->clipRect(SkRect::MakeXYWH(0.0f, 0.0f, alphaBarDim.x(), alphaBarDim.y()), skiaAA);

    SkPaint alphaBarPaint;
    if(selectAlpha)
        alphaBarPaint.setShader(ColorPickerShaders::get_alpha_bar_shader({(*data)[0], (*data)[1], (*data)[2]}, alphaBarDim.x()));
    else
        alphaBarPaint.setColor4f(SkColor4f{(*data)[0], (*data)[1], (*data)[2], 1.0f});
    canvas->drawPaint(alphaBarPaint);

    if(selectAlpha) {
        canvas->scale(alphaBarDim.x(), alphaBarDim.y());
        canvas->drawLine((*data)[3], 0.0f, (*data)[3], 1.0f, selectionLinePaint);
    }

    canvas->restore();
}

template <typename T> bool ColorPicker<T>::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
    held = HeldBar::NONE;
    if(mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down && boundingBox.has_value()) {
        auto& bb = boundingBox.value();
        float svSelectionAreaSize = get_sv_selection_area_size();
        if(SCollision::collide(SCollision::AABB<float>(bb.min, bb.min + Vector2f{svSelectionAreaSize, svSelectionAreaSize}), button.pos))
            held = HeldBar::SV_HELD;
        else {
            Vector2f hueBarPos = get_hue_bar_pos();
            Vector2f hueBarDim = get_hue_bar_dim();
            if(SCollision::collide(SCollision::AABB<float>(hueBarPos, hueBarPos + hueBarDim), button.pos))
                held = HeldBar::HUE_HELD;
            else if(selectAlpha) {
                Vector2f alphaBarPos = get_alpha_bar_pos();
                Vector2f alphaBarDim = get_alpha_bar_dim();
                if(SCollision::collide(SCollision::AABB<float>(alphaBarPos, alphaBarPos + alphaBarDim), button.pos))
                    held = HeldBar::ALPHA_HELD;
            }
        }
    }
    update_color_picker_pos(button.pos);
    return Element::input_mouse_button_callback(button, mouseHovering);
}

template <typename T> bool ColorPicker<T>::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) {
    update_color_picker_pos(motion.pos);
    return Element::input_mouse_motion_callback(motion, mouseHovering);
}

template <typename T> void ColorPicker<T>::update_color_picker_pos(const Vector2f& p) {
    if(held != HeldBar::NONE && boundingBox.has_value()) {
        gui.set_post_callback_func([&, p]() {
            auto& bb = boundingBox.value();
            switch(held) {
                case HeldBar::NONE:
                    break;
                case HeldBar::SV_HELD: {
                    float svSelectionAreaSize = bb.width() - (BAR_GAP + BAR_WIDTH);
                    Vector2f newSv = cwise_vec_clamp<Vector2f>((p - bb.min) / svSelectionAreaSize, Vector2f{0.0f, 0.0f}, Vector2f{1.0f, 1.0f});
                    savedHsv.y() = newSv.x();
                    savedHsv.z() = 1.0f - newSv.y();
                    set_hsv(savedHsv);
                    if(onChange) onChange();
                    break;
                }
                case HeldBar::HUE_HELD: {
                    Vector2f huePos = get_hue_bar_pos();
                    Vector2f hueDim = get_hue_bar_dim();
                    savedHsv.x() = (1.0f - std::clamp((p.y() - huePos.y()) / hueDim.y(), 0.0f, 1.0f)) * 360.0f;
                    set_hsv(savedHsv);
                    if(onChange) onChange();
                    break;
                }
                case HeldBar::ALPHA_HELD: {
                    Vector2f alphaPos = get_alpha_bar_pos();
                    Vector2f alphaDim = get_alpha_bar_dim();
                    (*data)[3] = std::clamp((p.x() - alphaPos.x()) / alphaDim.x(), 0.0f, 1.0f);
                    oldData = *data;
                    if(onChange) onChange();
                    break;
                }
            }
        });
    }
}

template <typename T> float ColorPicker<T>::get_sv_selection_area_size() {
    return boundingBox.value().width() - (BAR_GAP + BAR_WIDTH);
}

template <typename T> Vector2f ColorPicker<T>::get_hue_bar_pos() {
    return {boundingBox.value().min.x() + get_sv_selection_area_size() + BAR_GAP, boundingBox.value().min.y()};
}

template <typename T> Vector2f ColorPicker<T>::get_hue_bar_dim() {
    return {BAR_WIDTH, get_sv_selection_area_size()};
}

template <typename T> Vector2f ColorPicker<T>::get_alpha_bar_pos() {
    return {boundingBox.value().min.x(), boundingBox.value().min.y() + get_sv_selection_area_size() + BAR_GAP};
}

template <typename T> Vector2f ColorPicker<T>::get_alpha_bar_dim() {
    return {boundingBox.value().width(), BAR_WIDTH};
}

template <typename T> void ColorPicker<T>::set_hsv(const Vector3f& hsv) {
    Vector3f a = hsv_to_rgb<Vector3f>(hsv);
    (*data)[0] = a.x();
    (*data)[1] = a.y();
    (*data)[2] = a.z();
    oldData = *data;
}

}
