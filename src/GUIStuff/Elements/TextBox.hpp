#pragma once
#include "Element.hpp"
#include "../../RichTextBox/RichTextBox.hpp"
#include <limits>

namespace GUIStuff {

template <typename T> class TextBox : public Element {
    public:
        void update(UpdateInputData& io, T* newData, const std::function<std::optional<T>(const std::string&)>& newFromStr, const std::function<std::string(const T&)> newToStr, bool newSingleLine, bool updateEveryEdit, const std::function<void(SelectionHelper&)>& elemUpdate) {
            if(!textbox || !cur)
                force_update_textbox(io, false);

            data = newData;
            fromStr = newFromStr;
            toStr = newToStr;
            singleLine = newSingleLine;
            force_update_textbox(io, false);

            textbox->set_width(singleLine ? std::numeric_limits<float>::max() : bb.width());

            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(static_cast<float>(io.fontSize * 2)), .height = CLAY_SIZING_GROW(static_cast<float>(io.fontSize * 1.25f))}
                },
                .custom = { .customData = this }
            }) {
                selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
                if(data && selection.selected) {
                    io.richTextBoxEdit(textbox, cur);
                    textbox->process_mouse_left_button(*cur, io.mouse.pos - bb.min, io.mouse.leftClick, io.mouse.leftHeld, io.key.leftShift);
                    update_on_edit(updateEveryEdit);
                }
                if(data && selection.justUnselected) {
                    std::optional<T> dataToAssign = fromStr(textbox->get_string());
                    if(dataToAssign)
                        *data = dataToAssign.value();
                    force_update_textbox(io, true);
                }
                if(elemUpdate)
                    elemUpdate(selection);
            }
        }

        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override {
            if(!data || !textbox || !cur)
                return;

            bb = get_bb(command);

            canvas->save();
            SkRect r = SkRect::MakeXYWH(bb.min.x(), bb.min.y(), bb.width(), bb.height());

            canvas->drawRect(r, SkPaint(io.theme->backColor2));
            SkPaint outline(selection.selected ? io.theme->fillColor1 : io.theme->backColor3);
            outline.setStroke(true);
            outline.setStrokeWidth(2.0f);
            canvas->drawRoundRect(r, 2.0f, 2.0f, outline);

            canvas->clipRect(SkRect::MakeXYWH(bb.min.x(), bb.min.y(), bb.width(), bb.height()));

            canvas->translate(bb.min.x(), bb.min.y());

            RichTextBox::PaintOpts paintOpts;
            if(selection.selected)
                paintOpts.cursor = *cur;
            paintOpts.cursorColor = {io.theme->fillColor1.fR, io.theme->fillColor1.fG, io.theme->fillColor1.fB};

            textbox->paint(canvas, paintOpts);

            canvas->restore();
        }

        SelectionHelper selection;
    private:
        void force_update_textbox(UpdateInputData& io, bool reallyForce) {
            if(data && (*data == oldData) && !reallyForce && textbox && cur)
                return;

            textbox = std::make_shared<RichTextBox>();
            textbox->set_font_collection(io.fontCollection);
            textbox->set_allow_newlines(!singleLine);
            cur = std::make_shared<RichTextBox::Cursor>();

            if(data) {
                cur->pos = cur->selectionBeginPos = cur->selectionEndPos = textbox->insert({0, 0}, toStr(*data));
                oldData = *data;
            }
        }

        void update_on_edit(bool updateEveryEdit) {
            if(updateEveryEdit) {
                std::optional<T> dataToAssign = fromStr(textbox->get_string());
                if(dataToAssign) {
                    *data = dataToAssign.value();
                    oldData = *data;
                }
            }
        }

        T* data = nullptr;
        T oldData;

        bool singleLine = true;

        std::function<std::optional<T>(const std::string&)> fromStr;
        std::function<std::string(const T&)> toStr;

        std::shared_ptr<RichTextBox> textbox;
        std::shared_ptr<RichTextBox::Cursor> cur;

        SCollision::AABB<float> bb;
};

}
