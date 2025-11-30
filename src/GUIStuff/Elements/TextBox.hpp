#pragma once
#include "Element.hpp"
#include "../../RichText/TextBox.hpp"
#include <limits>
#include <modules/skparagraph/include/TextStyle.h>
#include "../../FontData.hpp"

namespace GUIStuff {

template <typename T> class TextBox : public Element {
    public:
        bool update(UpdateInputData& io, T* newData, const std::function<std::optional<T>(const std::string&)>& newFromStr, const std::function<std::string(const T&)> newToStr, bool newSingleLine, bool updateEveryEdit, const std::function<void(SelectionHelper&)>& elemUpdate) {
            bool isUpdating = false;

            if(!textbox || !cur)
                force_update_textbox(io, true);

            data = newData;
            fromStr = newFromStr;
            toStr = newToStr;
            force_update_textbox(io, false);

            textbox->set_width(std::numeric_limits<float>::max());
            //textbox->set_text_direction_between(0, 0, textbox->get_suggested_direction(0));

            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(static_cast<float>(io.fontSize * 2)), .height = CLAY_SIZING_FIXED(static_cast<float>(io.fontSize * 1.25f))}
                },
                .custom = { .customData = this }
            }) {
                selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
                if(data && selection.selected) {
                    io.richTextBoxEdit(textbox, cur);
                    textbox->process_mouse_left_button(*cur, io.mouse.pos - bb.min, io.mouse.leftClick, io.mouse.leftHeld, io.key.leftShift);
                    if((updateEveryEdit && textbox->inputChangedTextBox) || io.key.enter) {
                        update_on_edit();
                        isUpdating = true;
                    }
                }
                if(data && selection.justUnselected) {
                    std::optional<T> dataToAssign = fromStr(textbox->get_string());
                    if(dataToAssign.has_value())
                        *data = dataToAssign.value();
                    force_update_textbox(io, true);
                    isUpdating = true;
                }
                if(elemUpdate)
                    elemUpdate(selection);
            }

            if(textbox)
                textbox->inputChangedTextBox = false;

            return isUpdating;
        }

        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override {
            if(!data || !textbox || !cur)
                return;

            bb = get_bb(command);

            canvas->save();
            SkRect r = SkRect::MakeXYWH(bb.min.x(), bb.min.y(), bb.width(), bb.height());

            canvas->drawRect(r, SkPaint(io.theme->backColor2));
            SkPaint outline(selection.selected ? io.theme->fillColor1 : io.theme->backColor2);
            outline.setStroke(true);
            outline.setStrokeWidth(2.0f);
            canvas->drawRoundRect(r, 2.0f, 2.0f, outline);

            canvas->clipRect(SkRect::MakeXYWH(bb.min.x(), bb.min.y(), bb.width(), bb.height()));

            float yOffset = bb.height() * 0.5f;
            yOffset -= textbox->get_height() * 0.5f;

            RichText::TextBox::PaintOpts paintOpts;
            if(selection.selected) {
                paintOpts.cursor = *cur;
                SkRect cursorRect = textbox->get_cursor_rect(cur->pos);
                canvas->translate(bb.min.x() - std::max(cursorRect.fRight - bb.width(), -2.0f), bb.min.y() + yOffset);
            }
            else
                canvas->translate(bb.min.x() + 2.0f, bb.min.y() + yOffset);
            paintOpts.cursorColor = {io.theme->fillColor1.fR, io.theme->fillColor1.fG, io.theme->fillColor1.fB};

            skia::textlayout::TextStyle tStyle;
            tStyle.setFontFamilies(io.fonts->get_default_font_families());
            tStyle.setFontSize(io.fontSize);
            tStyle.setForegroundPaint(SkPaint{io.theme->frontColor1});
            textbox->set_initial_text_style(tStyle);

            textbox->paint(canvas, paintOpts);

            canvas->restore();
        }

        SelectionHelper selection;
    private:
        void force_update_textbox(UpdateInputData& io, bool reallyForce) {
            if(data && oldData.has_value() && (*data == oldData.value()) && !reallyForce && textbox && cur)
                return;

            textbox = std::make_shared<RichText::TextBox>();
            textbox->set_font_data(io.fonts);
            textbox->set_allow_newlines(false);

            cur = std::make_shared<RichText::TextBox::Cursor>();

            if(data) {
                cur->pos = cur->selectionBeginPos = cur->selectionEndPos = textbox->insert({0, 0}, toStr(*data));
                oldData = *data;
            }
        }

        void update_on_edit() {
            std::optional<T> dataToAssign = fromStr(textbox->get_string());
            if(dataToAssign.has_value()) {
                *data = dataToAssign.value();
                oldData = *data;
            }
        }

        T* data = nullptr;
        std::optional<T> oldData;

        std::function<std::optional<T>(const std::string&)> fromStr;
        std::function<std::string(const T&)> toStr;

        std::shared_ptr<RichText::TextBox> textbox;
        std::shared_ptr<RichText::TextBox::Cursor> cur;

        SCollision::AABB<float> bb;
};

}
