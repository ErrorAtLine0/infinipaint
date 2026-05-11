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

import android.os.Build;
import android.text.Editable;
import android.text.InputFilter;
import android.text.SpannableStringBuilder;
import android.widget.EditText;

import java.util.stream.IntStream;

public class InfiniPaintTextBoxEditable extends SpannableStringBuilder {
    public static native void nativeReplace(long mTextBoxID, int st, int en, String str);
    public static native String nativeGetString(long mTextBoxID);

    public long mTextBoxID;

    InfiniPaintTextBoxEditable() {
    }

    @Override
    public SpannableStringBuilder replace(int st, int en, CharSequence source, int start, int end) {
        nativeReplace(mTextBoxID, st, en, source.subSequence(start, end).toString());
        return this;
    }

    @Override
    public SpannableStringBuilder replace(int st, int en, CharSequence text) {
        return replace(st, en, text, 0, text.length());
    }

    @Override
    public SpannableStringBuilder insert(int where, CharSequence text, int start, int end) {
        return replace(where, where, text, start, end);
    }

    @Override
    public SpannableStringBuilder insert(int where, CharSequence text) {
        return replace(where, where, text, 0, text.length());
    }

    @Override
    public SpannableStringBuilder delete(int st, int en) {
        return replace(st, en, "", 0, 0);
    }

    @Override
    public SpannableStringBuilder append(CharSequence text) {
        return replace(length(), length(), text, 0, text.length());
    }

    @Override
    public SpannableStringBuilder append(CharSequence text, int start, int end) {
        return replace(length(), length(), text, start, end);
    }

    @Override
    public SpannableStringBuilder append(char text) {
        return append(String.valueOf(text));
    }

    @Override
    public void clear() {
        replace(0, length(), "", 0, 0);
    }

    @Override
    public int length() {
        return nativeGetString(mTextBoxID).length();
    }

    @Override
    public char charAt(int i) {
        return nativeGetString(mTextBoxID).charAt(i);
    }

    @Override
    public boolean isEmpty() {
        return nativeGetString(mTextBoxID).isEmpty();
    }

    @Override
    public CharSequence subSequence(int i, int i1) {
        return nativeGetString(mTextBoxID).subSequence(i, i1);
    }

    @Override
    public IntStream chars() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            return nativeGetString(mTextBoxID).chars();
        return null;
    }

    @Override
    public IntStream codePoints() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            return nativeGetString(mTextBoxID).codePoints();
        return null;
    }

    @Override
    public void getChars(int i, int i1, char[] chars, int i2) {
        nativeGetString(mTextBoxID).getChars(i, i1, chars, i2);
    }
}
