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

import android.content.Context;
import android.graphics.Insets;
import android.os.Build;
import android.util.Log;
import android.view.View;
import android.view.WindowInsets;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLSurface;

public class InfiniPaintSurface extends SDLSurface {
    protected InfiniPaintSurface(Context context) {
        super(context);
    }

    public static native void nativeInfiniPaintUpdateIMESafeArea(int top, int bottom, int left, int right);

    // Window inset calculations are different from SDL defaults
    @Override
    public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
        if (Build.VERSION.SDK_INT >= 30 /* Android 11 (R) */) {
            Insets combined = insets.getInsets(WindowInsets.Type.systemBars() |
                    //WindowInsets.Type.systemGestures() |
                    //WindowInsets.Type.mandatorySystemGestures() |
                    WindowInsets.Type.tappableElement() |
                    WindowInsets.Type.displayCutout());

            Insets imeInsets = insets.getInsets(WindowInsets.Type.systemBars() |
                    //WindowInsets.Type.systemGestures() |
                    //WindowInsets.Type.mandatorySystemGestures() |
                    WindowInsets.Type.tappableElement() |
                    WindowInsets.Type.displayCutout() |
                    WindowInsets.Type.ime());

            nativeInfiniPaintUpdateIMESafeArea(imeInsets.top, imeInsets.bottom, imeInsets.left, imeInsets.right);
            SDLActivity.onNativeInsetsChanged(combined.left, combined.right, combined.top, combined.bottom);

            if (insets.isVisible(WindowInsets.Type.ime())) {
                if (!mKeyboardVisible) {
                    mKeyboardVisible = true;
                    SDLActivity.onNativeScreenKeyboardShown();
                }
            } else {
                if (mKeyboardVisible) {
                    mKeyboardVisible = false;
                    SDLActivity.onNativeScreenKeyboardHidden();
                }
            }
        }

        // Pass these to any child views in case they need them
        return insets;
    }
}
