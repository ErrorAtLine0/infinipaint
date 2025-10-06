#pragma once
#include "Element.hpp"
#include "../../CollabTextBox/CollabTextBox.hpp"

namespace GUIStuff {

template <typename T> class TextBox : public Element {
    public:
        void update(UpdateInputData& io, T* newData, const std::function<std::optional<T>(const std::string&)>& newFromStr, const std::function<std::string(const T&)> newToStr, bool newSingleLine, bool updateEveryEdit, const std::function<void(SelectionHelper&)>& elemUpdate) {
            data = newData;
            fromStr = newFromStr;
            toStr = newToStr;
            singleLine = newSingleLine;
            force_update_textbox(false);

            textbox.setFont(SkFont(io.textTypeface, io.fontSize));
            textbox.setFontMgr(io.textFontMgr);

            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(static_cast<float>(io.fontSize * 2)), .height = CLAY_SIZING_GROW(static_cast<float>(io.fontSize * 1.25f))}
                },
                .custom = { .customData = this }
            }) {
                selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
                if(data && selection.selected) {
                    bool moved = io.key.left || io.key.right || io.key.up || io.key.down || io.key.home;
                    bool movedHeld = false;

                    if(io.mouse.leftClick) {
                        Vector2f textSelectPos = io.mouse.pos - bb.min;
                        SkIPoint p = convert_vec2<SkIPoint>(textSelectPos.cast<int32_t>());
                        cur.pos = cur.selectionBeginPos = textbox.getPosition(p);
                        moved = true;
                    }
                    else if(io.mouse.leftHeld) {
                        Vector2f textSelectPos = io.mouse.pos - bb.min;
                        SkIPoint p = convert_vec2<SkIPoint>(textSelectPos.cast<int32_t>());
                        cur.pos = cur.selectionBeginPos = textbox.getPosition(p);
                        movedHeld = true;
                    }

                    if(io.key.left)
                        cur.pos = cur.selectionBeginPos = textbox.move(io.key.leftCtrl ? CollabTextBox::Movement::kWordLeft : CollabTextBox::Movement::kLeft, cur.selectionBeginPos);
                    if(io.key.right)
                        cur.pos = cur.selectionBeginPos = textbox.move(io.key.leftCtrl ? CollabTextBox::Movement::kWordRight : CollabTextBox::Movement::kRight, cur.selectionBeginPos);
                    if(io.key.up && !singleLine)
                        cur.pos = cur.selectionBeginPos = textbox.move(CollabTextBox::Movement::kUp, cur.selectionBeginPos);
                    if(io.key.down && !singleLine)
                        cur.pos = cur.selectionBeginPos = textbox.move(CollabTextBox::Movement::kDown, cur.selectionBeginPos);
                    if(io.key.home)
                        cur.pos = cur.selectionBeginPos = textbox.move(CollabTextBox::Movement::kHome, cur.selectionBeginPos);

                    if(moved && !io.key.leftShift && !movedHeld)
                        cur.selectionEndPos = cur.selectionBeginPos;

                    if(io.key.backspace) {
                        if(cur.selectionBeginPos != cur.selectionEndPos)
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
                        else
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.pos, textbox.move(CollabTextBox::Movement::kLeft, cur.pos));
                        update_on_edit(updateEveryEdit);
                    }
                    if(io.key.del) {
                        if(cur.selectionBeginPos != cur.selectionEndPos)
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
                        else
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.pos, textbox.move(CollabTextBox::Movement::kRight, cur.pos));
                        update_on_edit(updateEveryEdit);
                    }
                    if(io.key.paste) {
                        if(cur.selectionBeginPos != cur.selectionEndPos)
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
                        if(singleLine) {
                            std::string removedNewlines = io.clipboard.textInFunc();
                            std::erase(removedNewlines, '\n');
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.insert(cur.pos, removedNewlines);
                        }
                        else
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.insert(cur.pos, io.clipboard.textInFunc());
                        update_on_edit(updateEveryEdit);
                    }
                    if(io.key.enter) {
                        if(singleLine) {
                            std::optional<T> dataToAssign = fromStr(textbox.get_string());
                            if(dataToAssign)
                                *data = dataToAssign.value();
                            force_update_textbox(true);
                        }
                        else {
                            if(cur.selectionBeginPos != cur.selectionEndPos)
                                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
                            cur.pos = textbox.insert(cur.pos, "\n");
                            cur.pos.fParagraphIndex++;
                            cur.pos.fTextByteIndex = 0;
                            cur.selectionBeginPos = cur.selectionEndPos = cur.pos;
                            update_on_edit(updateEveryEdit);
                        }
                    }
                    if(io.key.copy) {
                        io.clipboard.textOut = textbox.copy(cur.selectionBeginPos, cur.selectionEndPos);
                    }
                    if(io.key.cut) {
                        io.clipboard.textOut = textbox.copy(cur.selectionBeginPos, cur.selectionEndPos);
                        if(cur.selectionBeginPos != cur.selectionEndPos)
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
                        update_on_edit(updateEveryEdit);
                    }
                    if(io.key.selectAll) {
                        cur.selectionEndPos.fParagraphIndex = textbox.lineCount() == 0 ? 0 : textbox.lineCount() - 1;
                        cur.selectionEndPos.fTextByteIndex = textbox.line(cur.selectionEndPos.fParagraphIndex).size();
                        cur.pos = cur.selectionEndPos;
                        cur.selectionBeginPos.fTextByteIndex = 0;
                        cur.selectionBeginPos.fParagraphIndex = 0;
                    }
                    if(!io.textInput.empty()) {
                        if(cur.selectionBeginPos != cur.selectionEndPos)
                            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
                        cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.insert(cur.pos, io.textInput);
                        update_on_edit(updateEveryEdit);
                    }

                    io.acceptingTextInput = true;
                }
                if(data && selection.justUnselected) {
                    std::optional<T> dataToAssign = fromStr(textbox.get_string());
                    if(dataToAssign)
                        *data = dataToAssign.value();
                    force_update_textbox(true);
                }
                if(elemUpdate)
                    elemUpdate(selection);
            }
        }

        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override {
            if(!data)
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

            if(singleLine && selection.selected) {
                SkRect cursorRect = textbox.getLocation(cur.pos);
                canvas->translate(bb.min.x() - std::max(cursorRect.fRight - bb.width(), 0.0f), bb.min.y());
            }
            else
                canvas->translate(bb.min.x(), bb.min.y());

            textbox.setFont(SkFont(io.textTypeface, io.fontSize));
            textbox.setFontMgr(io.textFontMgr);
            textbox.setWidth(singleLine ? 99999999 : bb.width());

            CollabTextBox::Editor::PaintOpts paintOpts;
            paintOpts.fForegroundColor = io.theme->frontColor1;
            paintOpts.fBackgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};
            paintOpts.cursorColor = io.theme->fillColor5;
            paintOpts.showCursor = selection.selected;
            paintOpts.cursor = cur;

            textbox.paint(canvas, paintOpts);

            canvas->restore();
        }

        SelectionHelper selection;
    private:
        void force_update_textbox(bool reallyForce) {
            if(data && (*data == oldData) && !reallyForce)
                return;

            textbox = CollabTextBox::Editor();
            cur = CollabTextBox::Cursor();
            if(data) {
                cur.pos = cur.selectionBeginPos = cur.selectionEndPos = textbox.insert({0, 0}, toStr(*data));
                oldData = *data;
            }
            else {}
                //oldData = T();
        }

        void update_on_edit(bool updateEveryEdit) {
            if(updateEveryEdit) {
                std::optional<T> dataToAssign = fromStr(textbox.get_string());
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

        CollabTextBox::Editor textbox;
        CollabTextBox::Cursor cur;

        SCollision::AABB<float> bb;
};

}
