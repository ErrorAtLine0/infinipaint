/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

package com.erroratline0.infinipaint;

import static android.text.TextUtils.CAP_MODE_SENTENCES;

import android.content.*;
import android.graphics.RectF;
import android.os.Build;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.Handler;
import android.text.Editable;
import android.text.Selection;
import android.text.SpannableStringBuilder;
import android.util.Log;
import android.view.*;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.CorrectionInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.HandwritingGesture;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputContentInfo;
import android.view.inputmethod.PreviewableHandwritingGesture;
import android.view.inputmethod.SurroundingText;
import android.view.inputmethod.TextAttribute;
import android.view.inputmethod.TextBoundsInfoResult;
import android.view.inputmethod.TextSnapshot;

import java.util.concurrent.Executor;
import java.util.function.Consumer;
import java.util.function.IntConsumer;

import android.content.*;
import android.os.Build;
import android.text.Editable;
import android.view.*;
import android.view.inputmethod.BaseInputConnection;
import android.widget.EditText;

import org.libsdl.app.SDL;
import org.libsdl.app.SDLActivity;

public class InfiniPaintTextBoxInputConnection extends BaseInputConnection {
    protected InfiniPaintTextBoxEditable mEditText;
    protected long mTextBoxID;

    public static native int[] nativeGetSelection(long mTextBoxID);
    public static native void nativeShiftSelection(long mTextBoxID, int amount);
    public static native void nativeSetSelection(long mTextBoxID, int start, int end);

    InfiniPaintTextBoxInputConnection(View targetView, boolean fullEditor) {
        super(targetView, fullEditor);
        mEditText = new InfiniPaintTextBoxEditable();
    }

    public void setInitialData(long textboxID, String initialText, int begin, int end) {
        mTextBoxID = textboxID;
        mEditText.mTextBoxID = textboxID;
        Selection.setSelection(mEditText, begin, end);
    }

    public void updateSelection(int start, int end) {
        Selection.setSelection(mEditText, start, end);
    }

    public void clearAndSetNewTextAndSelection(String str, int start, int end) {
        finishComposingText();
        mEditText.clear();
        mEditText.replace(0, 0, str);
        Selection.setSelection(mEditText, start, end);
    }

    @Override
    public Editable getEditable() {
        return mEditText;
    }

    @Override
    public boolean sendKeyEvent(KeyEvent event) {
        /*
         * This used to handle the keycodes from soft keyboard (and IME-translated input from hardkeyboard)
         * However, as of Ice Cream Sandwich and later, almost all soft keyboard doesn't generate key presses
         * and so we need to generate them ourselves in commitText.  To avoid duplicates on the handful of keys
         * that still do, we empty this out.
         */

        /*
         * Return DOES still generate a key event, however. So rather than using it as the 'click a button' key
         * as we do with physical keyboards, let's just use it to hide the keyboard.
         */

        if (event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
            if (SDLActivity.onNativeSoftReturnKey()) {
                return true;
            }
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_DEL && event.getAction() == KeyEvent.ACTION_DOWN) {
            int start = Selection.getSelectionStart(mEditText);
            int end = Selection.getSelectionEnd(mEditText);
            if(start != end)
                commitText("", 0);
            else {
                nativeShiftSelection(mTextBoxID, -1);
                int[] p = nativeGetSelection(mTextBoxID);
                if(p[0] != start)
                    deleteSurroundingText(start - p[0], 0);
            }
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_DPAD_LEFT && event.getAction() == KeyEvent.ACTION_DOWN) {
            leftKey();
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_DPAD_RIGHT && event.getAction() == KeyEvent.ACTION_DOWN) {
            rightKey();
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_0 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("0", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_1 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("1", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_2 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("2", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_3 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("3", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_4 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("4", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_5 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("5", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_6 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("6", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_7 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("7", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_8 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("8", 1);
            return true;
        }
        else if(event.getKeyCode() == KeyEvent.KEYCODE_9 && event.getAction() == KeyEvent.ACTION_DOWN) {
            finishComposingText();
            commitText("9", 1);
            return true;
        }

        return super.sendKeyEvent(event);
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {
        // Manually do commitText
        int composingTextBegin = getComposingSpanStart(mEditText);
        int composingTextEnd = getComposingSpanEnd(mEditText);
        int start, end;

        if(composingTextBegin != -1 && composingTextEnd != -1) {
            start = Math.min(composingTextBegin, composingTextEnd);
            end = Math.max(composingTextBegin, composingTextEnd);
        }
        else {
            start = Math.min(Selection.getSelectionStart(mEditText), Selection.getSelectionEnd(mEditText));
            end = Math.max(Selection.getSelectionStart(mEditText), Selection.getSelectionEnd(mEditText));
        }

        mEditText.replace(start, end, text);
        if(newCursorPosition <= 0)
            setSelection(start + newCursorPosition, start + newCursorPosition);
        else
            setSelection(start + text.length() + newCursorPosition - 1, start + text.length() + newCursorPosition - 1);
        return true;
    }

    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {
        // Manually do setComposingText
        int composingTextBegin = getComposingSpanStart(mEditText);
        int composingTextEnd = getComposingSpanEnd(mEditText);
        int start, end;

        if(composingTextBegin != -1 && composingTextEnd != -1) {
            start = Math.min(composingTextBegin, composingTextEnd);
            end = Math.max(composingTextBegin, composingTextEnd);
        }
        else {
            start = Math.min(Selection.getSelectionStart(mEditText), Selection.getSelectionEnd(mEditText));
            end = Math.max(Selection.getSelectionStart(mEditText), Selection.getSelectionEnd(mEditText));
        }
        mEditText.replace(start, end, text);
        if(newCursorPosition <= 0)
            setSelection(start + newCursorPosition, start + newCursorPosition);
        else
            setSelection(start + text.length() + newCursorPosition - 1, start + text.length() + newCursorPosition - 1);

        setComposingRegion(start, start + text.length());

        return true;
    }

    @Override
    public boolean deleteSurroundingText(int beforeLength, int afterLength) {
        int start = Math.min(Selection.getSelectionStart(mEditText), Selection.getSelectionEnd(mEditText));
        if (!super.deleteSurroundingText(beforeLength, afterLength))
            return false;
        int newCursorPos = Math.max(start - beforeLength, 0);
        setSelection(newCursorPos, newCursorPos);
        return true;
    }

    @Override
    public boolean deleteSurroundingTextInCodePoints(int beforeLength, int afterLength) {
        if (!super.deleteSurroundingTextInCodePoints(beforeLength, afterLength))
            return false;
        int[] p = nativeGetSelection(mTextBoxID);
        Selection.setSelection(mEditText, p[0], p[1]);
        return true;
    }

    @Override
    public boolean setSelection(int start, int end) {
        if(!super.setSelection(start, end))
            return false;
        nativeSetSelection(mTextBoxID, start, end);
        int[] p = nativeGetSelection(mTextBoxID);
        Selection.setSelection(mEditText, p[0], p[1]);
        return true;
    }

    @Override
    public boolean replaceText(int start, int end, CharSequence text, int newCursorPosition, TextAttribute textAttribute) {
        finishComposingText();
        setSelection(start, end);
        commitText(text, newCursorPosition);
        return true;
    }

    public void leftKey() {
        nativeShiftSelection(mTextBoxID, -1);
        int[] p = nativeGetSelection(mTextBoxID);
        Selection.setSelection(mEditText, p[0], p[1]);
    }

    public void rightKey() {
        nativeShiftSelection(mTextBoxID, 1);
        int[] p = nativeGetSelection(mTextBoxID);
        Selection.setSelection(mEditText, p[0], p[1]);
    }
}

