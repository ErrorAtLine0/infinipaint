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

import static androidx.core.content.FileProvider.getUriForFile;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.RelativeLayout;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLDummyEdit;
import org.libsdl.app.SDLSurface;

import java.io.File;
import java.util.ArrayList;

public class InfiniPaint extends SDLActivity {
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Window window = this.getWindow();
        window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
        window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
        window.setStatusBarColor(android.graphics.Color.TRANSPARENT);
        window.setNavigationBarColor(android.graphics.Color.TRANSPARENT);
        // Transparent navigation
        //if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
        //    window.setNavigationBarContrastEnforced(false);
    }

    protected SDLSurface createSDLSurface(Context context) {
        return new InfiniPaintSurface(context);
    }

    static class StartInfiniPaintTextInputTask implements Runnable {
        public StartInfiniPaintTextInputTask(long textboxID, String str, int selectionBegin, int selectionEnd, int inputType) {
            mTextBoxID = textboxID;
            mStr = str;
            mSelectionBegin = selectionBegin;
            mSelectionEnd = selectionEnd;
            mInputType = inputType;
        }

        long mTextBoxID;
        String mStr;
        int mSelectionBegin;
        int mSelectionEnd;
        int mInputType;

        @Override
        public void run() {
            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(1, 1);
            //params.leftMargin = 1;
            //params.topMargin = 1;

            if (mTextEdit2 == null) {
                mTextEdit2 = new InfiniPaintTextBoxView(getContext());
                mLayout.addView(mTextEdit2, params);
            }
            else {
                mTextEdit2.setLayoutParams(params);
            }

            mTextEdit2.setInitialData(mTextBoxID, mStr, mSelectionBegin, mSelectionEnd, mInputType);
            mTextEdit2.setVisibility(View.VISIBLE);
            mTextEdit2.requestFocus();
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.showSoftInput(mTextEdit2, 0);

            if (imm.isAcceptingText())
                onNativeScreenKeyboardShown();
        }
    }

    static class InfiniPaintUpdateCursorPosTask implements Runnable {
        public InfiniPaintUpdateCursorPosTask(int selectionBegin, int selectionEnd) {
            mSelectionBegin = selectionBegin;
            mSelectionEnd = selectionEnd;
        }

        int mSelectionBegin;
        int mSelectionEnd;

        @Override
        public void run() {
            if (mTextEdit2 != null) {
                InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                mTextEdit2.ic.updateSelection(mSelectionBegin, mSelectionEnd);
                imm.updateSelection(mTextEdit2, mSelectionBegin, mSelectionEnd, -1, -1);
            }
        }
    }

    static class InfiniPaintUpdateTextboxAndCursorPosTask implements Runnable {
        public InfiniPaintUpdateTextboxAndCursorPosTask(String str, int selectionBegin, int selectionEnd) {
            mStr = str;
            mSelectionBegin = selectionBegin;
            mSelectionEnd = selectionEnd;
        }

        String mStr;
        int mSelectionBegin;
        int mSelectionEnd;

        @Override
        public void run() {
            if (mTextEdit2 != null) {
                InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                mTextEdit2.ic.clearAndSetNewTextAndSelection(mStr, mSelectionBegin, mSelectionEnd);
                imm.updateSelection(mTextEdit2, mSelectionBegin, mSelectionEnd, -1, -1);
            }
        }
    }

    static public void startTextInput(long textboxID, String str, int selectionBegin, int selectionEnd, int inputType) {
        mSingleton.commandHandler.post(new StartInfiniPaintTextInputTask(textboxID, str, selectionBegin, selectionEnd, inputType));
    }
    static public void updateCursorPos(int selectionBegin, int selectionEnd) {
        mSingleton.commandHandler.post(new InfiniPaintUpdateCursorPosTask(selectionBegin, selectionEnd));
    }
    static public void updateTextboxAndCursorPos(String str, int selectionBegin, int selectionEnd) {
        mSingleton.commandHandler.post(new InfiniPaintUpdateTextboxAndCursorPosTask(str, selectionBegin, selectionEnd));
    }
    static public void startNetworkService() {
        mSingleton.startForegroundService(new Intent(mSingleton, InfiniPaintNetworkService.class));
    }
    static public void stopNetworkService() {
        mSingleton.stopService(new Intent(mSingleton, InfiniPaintNetworkService.class));
    }

    static public void shareInternalFiles(String[] filePaths, String mimeType) {
        if(filePaths.length == 1) {
            File newFile = new File(getContext().getFilesDir(), filePaths[0]);
            Uri contentUri;

            try {
                contentUri = getUriForFile(getContext(), "com.erroratline0.infinipaint.fileprovider", newFile);
            } catch (Exception e) {
                Log.v("INFO", "[shareInternalFile] Exception " + e);
                return;
            }

            Intent sendIntent = new Intent();
            sendIntent.setAction(Intent.ACTION_SEND);
            sendIntent.putExtra(Intent.EXTRA_STREAM, contentUri);
            sendIntent.setType(mimeType);
            Intent shareIntent = Intent.createChooser(sendIntent, null);
            getContext().startActivity(shareIntent);
        }
        else if(filePaths.length > 1) {
            ArrayList<Uri> arrayList = new ArrayList<Uri>();
            for(String str : filePaths) {
                File newFile = new File(getContext().getFilesDir(), str);
                try {
                    arrayList.add(getUriForFile(getContext(), "com.erroratline0.infinipaint.fileprovider", newFile));
                } catch (Exception e) {
                    Log.v("INFO", "[shareInternalFile] Exception " + e);
                    return;
                }
            }
            Intent sendIntent = new Intent();
            sendIntent.setAction(Intent.ACTION_SEND_MULTIPLE);
            sendIntent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, arrayList);
            sendIntent.setType(mimeType);
            Intent shareIntent = Intent.createChooser(sendIntent, null);
            getContext().startActivity(shareIntent);
        }
    }

    static public void shareText(String textToSend) {
        Intent sendIntent = new Intent();
        sendIntent.setAction(Intent.ACTION_SEND);
        sendIntent.putExtra(Intent.EXTRA_TEXT, textToSend);
        sendIntent.setType("text/plain");
        Intent shareIntent = Intent.createChooser(sendIntent, null);
        getContext().startActivity(shareIntent);
    }
}
