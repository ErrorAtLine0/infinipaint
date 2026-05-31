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

#ifdef __ANDROID__
#include "AndroidJNICalls.hpp"
#include <jni.h>
#include "InputManager.hpp"
#include "MainProgram.hpp"
#include <SDL3/SDL_system.h>
#include <string>
#include <Helpers/Logger.hpp>
#include <modules/skunicode/include/SkUnicode.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include "NetThreadManager.hpp"

std::string jstring2string(JNIEnv *env, jstring jStr) {
    if (!jStr)
        return "";

    const jchar* c = env->GetStringChars(jStr, nullptr);
    if(!c)
        return "";
    jsize len = env->GetStringLength(jStr);

    SkString str = SkUnicodes::ICU::Make()->convertUtf16ToUtf8((const char16_t*)c, len);

    return std::string(str.c_str(), str.size());
}

jstring string2jstring(JNIEnv* env, const std::string& s) {
    std::u16string u16str = SkUnicodes::ICU::Make()->convertUtf8ToUtf16(s.c_str(), s.size());
    return env->NewString((const jchar*)u16str.data(), u16str.length());
}

namespace AndroidJNICalls {
    InputManager* globalInputManager;

    std::mutex textboxMutex;
    CustomEvents::InputTextBoxID textboxID = 0;
    std::shared_ptr<RichText::TextBox> textBox;
    std::shared_ptr<RichText::TextBox::Cursor> cursor;
    std::shared_ptr<RichText::TextStyleModifier::ModifierMap> modMap;
    bool textChanged = false;
    bool cursorChanged = false;

    void shareInternalFile(const std::string& filePath, const std::string& mimeType) {
        Logger::get().log(Logger::LogType::INFO, "[AndroidJNICalls::shareInternalFile] Share internal file " + filePath);
        JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
        jobject activity = (jobject)SDL_GetAndroidActivity();
        jclass clazz = env->GetObjectClass(activity);
        jmethodID method_id = env->GetStaticMethodID(clazz, "shareInternalFile", "(Ljava/lang/String;Ljava/lang/String;)V");
        env->CallStaticVoidMethod(clazz, method_id, string2jstring(env, filePath), string2jstring(env, mimeType));
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
    }

    void shareText(const std::string& str) {
        Logger::get().log(Logger::LogType::INFO, "[AndroidJNICalls::shareText] Share text " + str);
        JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
        jobject activity = (jobject)SDL_GetAndroidActivity();
        jclass clazz = env->GetObjectClass(activity);
        jmethodID method_id = env->GetStaticMethodID(clazz, "shareText", "(Ljava/lang/String;)V");
        env->CallStaticVoidMethod(clazz, method_id, string2jstring(env, str));
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
    }

    int get_android_text_pos_from_cursor_pos(const std::shared_ptr<RichText::TextBox>& t, const RichText::TextPosition& p) {
        return t->get_utf16_location_from_codepoint_location(t->get_codepoint_location_from_text_position(p));
    }

    RichText::TextPosition get_cursor_pos_from_android_text_pos(const std::shared_ptr<RichText::TextBox>& t, int p) {
        return t->get_text_position_from_codepoint_location(t->get_codepoint_location_from_utf16_location(p));
    }

    void startTextInput(CustomEvents::InputTextBoxID newTextboxID, const std::shared_ptr<RichText::TextBox>& newTextbox, const std::shared_ptr<RichText::TextBox::Cursor>& newCursor, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& newModMap, int inputType) {
        std::scoped_lock a{textboxMutex};
        if(!textBox) {
            textBox = std::make_shared<RichText::TextBox>();
            cursor = std::make_shared<RichText::TextBox::Cursor>();
        }

        textChanged = false;
        cursorChanged = false;

        textboxID = newTextboxID;
        textBox->set_rich_text_data(newTextbox->get_rich_text_data());
        *cursor = *newCursor;

        if(newModMap)
            modMap = std::make_shared<RichText::TextStyleModifier::ModifierMap>(*newModMap);
        else
            modMap = nullptr;

        JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
        jobject activity = (jobject)SDL_GetAndroidActivity();
        jclass clazz = env->GetObjectClass(activity);
        jmethodID method_id = env->GetStaticMethodID(clazz, "startTextInput", "(JLjava/lang/String;III)V");
        env->CallStaticVoidMethod(clazz, method_id, textboxID, string2jstring(env, textBox->get_string()),
                                  get_android_text_pos_from_cursor_pos(textBox, std::min(cursor->selectionBeginPos, cursor->selectionEndPos)),
                                  get_android_text_pos_from_cursor_pos(textBox, std::max(cursor->selectionBeginPos, cursor->selectionEndPos)), inputType);
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
    }

    void startNetworkService() {
        Logger::get().log(Logger::LogType::INFO, "[AndroidJNICalls::startNetworkService] Start service");
        JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
        jobject activity = (jobject)SDL_GetAndroidActivity();
        jclass clazz = env->GetObjectClass(activity);
        jmethodID method_id = env->GetStaticMethodID(clazz, "startNetworkService", "()V");
        env->CallStaticVoidMethod(clazz, method_id);
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
    }

    void stopNetworkService() {
        Logger::get().log(Logger::LogType::INFO, "[AndroidJNICalls::stopNetworkService] Stop service");
        JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
        jobject activity = (jobject)SDL_GetAndroidActivity();
        jclass clazz = env->GetObjectClass(activity);
        jmethodID method_id = env->GetStaticMethodID(clazz, "stopNetworkService", "()V");
        env->CallStaticVoidMethod(clazz, method_id);
        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(clazz);
    }

    void updateModMap(CustomEvents::InputTextBoxID tId, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& newModMap) {
        std::scoped_lock a{textboxMutex};
        if(tId == textboxID && newModMap && modMap)
            *modMap = *newModMap;
    }

    void updateCursorPos(CustomEvents::InputTextBoxID tId, const std::shared_ptr<RichText::TextBox::Cursor>& newCursor) {
        std::scoped_lock a{textboxMutex};

        if(tId == textboxID) {
            *cursor = *newCursor;

            JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
            jobject activity = (jobject)SDL_GetAndroidActivity();
            jclass clazz = env->GetObjectClass(activity);
            jmethodID method_id = env->GetStaticMethodID(clazz, "updateCursorPos", "(II)V");
            env->CallStaticVoidMethod(clazz, method_id,
                                      get_android_text_pos_from_cursor_pos(textBox, std::min(cursor->selectionBeginPos, cursor->selectionEndPos)),
                                      get_android_text_pos_from_cursor_pos(textBox, std::max(cursor->selectionBeginPos, cursor->selectionEndPos)));
            env->DeleteLocalRef(activity);
            env->DeleteLocalRef(clazz);
        }
    }

    void updateTextboxAndCursorPos(CustomEvents::InputTextBoxID tId, const std::shared_ptr<RichText::TextBox>& newTextbox, const std::shared_ptr<RichText::TextBox::Cursor>& newCursor) {
        std::scoped_lock a{textboxMutex};

        if(tId == textboxID) {
            *cursor = *newCursor;
            textBox->set_rich_text_data(newTextbox->get_rich_text_data());

            JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
            jobject activity = (jobject)SDL_GetAndroidActivity();
            jclass clazz = env->GetObjectClass(activity);
            jmethodID method_id = env->GetStaticMethodID(clazz, "updateCursorPos", "(II)V");
            env->CallStaticVoidMethod(clazz, method_id,
                                      get_android_text_pos_from_cursor_pos(textBox, std::min(cursor->selectionBeginPos, cursor->selectionEndPos)),
                                      get_android_text_pos_from_cursor_pos(textBox, std::max(cursor->selectionBeginPos, cursor->selectionEndPos)));
            env->DeleteLocalRef(activity);
            env->DeleteLocalRef(clazz);
        }
    }

    std::pair<bool, bool> getChangesToCommit(CustomEvents::InputTextBoxID tId) {
        std::scoped_lock a{textboxMutex};

        if(tId == textboxID)
            return {textChanged, cursorChanged};
        return {false, false};
    }

    std::pair<bool, bool> commitAll(CustomEvents::InputTextBoxID tId, const std::shared_ptr<RichText::TextBox>& t, const std::shared_ptr<RichText::TextBox::Cursor>& c) {
        std::scoped_lock a{textboxMutex};

        if(tId == textboxID) {
            bool oldTextChanged = textChanged;
            bool oldCursorChanged = cursorChanged;

            if (textChanged) {
                t->set_rich_text_data(textBox->get_rich_text_data());
                textChanged = false;
            }
            if (cursorChanged) {
                *c = *cursor;
                cursorChanged = false;
            }
            return {oldTextChanged, oldCursorChanged};
        }
        else
            return {false, false};
    }


    struct TextInputData {
        enum class CommandType {
            REPLACE_TEXT,
            SHIFT_CURSOR,
            SET_CURSOR
        } command;
        std::string strData;
        Vector3i intData;
    };

    void input_android_text_box(const TextInputData& textboxInput) {
        switch(textboxInput.command) {
            case TextInputData::CommandType::REPLACE_TEXT: {
                textBox->remove(get_cursor_pos_from_android_text_pos(textBox, textboxInput.intData.x()), get_cursor_pos_from_android_text_pos(textBox, textboxInput.intData.y()));
                textBox->insert(get_cursor_pos_from_android_text_pos(textBox, textboxInput.intData.x()), textboxInput.strData, modMap ? std::optional(*modMap) : std::nullopt);
                textChanged = cursorChanged = true;
                break;
            }
            case TextInputData::CommandType::SHIFT_CURSOR: {
                int newCursorPosition = textboxInput.intData.x();
                cursor->pos = std::min(cursor->selectionBeginPos, cursor->selectionEndPos);
                if(newCursorPosition < 0) {
                    for(int i = 0; i < (-newCursorPosition); i++)
                        cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = textBox->move(RichText::TextBox::Movement::LEFT, cursor->pos);
                }
                else {
                    for(int i = 0; i < newCursorPosition; i++)
                        cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = textBox->move(RichText::TextBox::Movement::RIGHT, cursor->pos);
                }
                cursorChanged = true;
                break;
            }
            case TextInputData::CommandType::SET_CURSOR: {
                cursor->selectionBeginPos = get_cursor_pos_from_android_text_pos(textBox, textboxInput.intData.x());
                cursor->selectionEndPos = cursor->pos = get_cursor_pos_from_android_text_pos(textBox, textboxInput.intData.y());
                cursorChanged = true;
                break;
            }
        }
    }
}

jobject activity;

using namespace AndroidJNICalls;

extern "C"
JNIEXPORT void JNICALL
Java_com_erroratline0_infinipaint_InfiniPaintSurface_nativeInfiniPaintUpdateIMESafeArea(JNIEnv *env, jclass clazz, jint top, jint bottom, jint left, jint right) {
    if(globalInputManager) {
        std::scoped_lock a{globalInputManager->safeAreaWithoutIMEMutex};
        globalInputManager->safeAreaWithoutIME = {{left, top}, {globalInputManager->main.window.size.x() - right, globalInputManager->main.window.size.y() - bottom}};

        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_EVENT_WINDOW_SAFE_AREA_CHANGED;
        SDL_PushEvent(&event);
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_erroratline0_infinipaint_InfiniPaintTextBoxEditable_nativeReplace(JNIEnv *env,
                                                                           jclass clazz, jlong id, jint st,
                                                                           jint en, jstring str) {
    std::scoped_lock a{textboxMutex};
    input_android_text_box({
                                   .command = TextInputData::CommandType::REPLACE_TEXT,
                                   .strData = jstring2string(env, str),
                                   .intData = {std::min(st, en), std::max(en, st), 0}
                           });
    CustomEvents::emit_event(
            CustomEvents::AndroidTextBoxInputEvent{
                    .command = CustomEvents::AndroidTextBoxInputEvent::CommandType::COMMIT_ALL,
            });
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_erroratline0_infinipaint_InfiniPaintTextBoxEditable_nativeGetString(JNIEnv *env,
                                                                             jclass clazz, jlong id) {
    std::scoped_lock a{textboxMutex};
    return string2jstring(env, textBox->get_string());
}

extern "C"
JNIEXPORT jintArray JNICALL
Java_com_erroratline0_infinipaint_InfiniPaintTextBoxInputConnection_nativeGetSelection(JNIEnv *env,
                                                                                       jclass clazz,
                                                                                       jlong m_text_box_id) {
    jintArray result;
    result = env->NewIntArray(2);
    jint fill[2];
    for(int i = 0; i < 2; i++)
        fill[i] = 0;

    std::scoped_lock a{textboxMutex};

    fill[0] = get_android_text_pos_from_cursor_pos(textBox, cursor->selectionBeginPos);
    fill[1] = get_android_text_pos_from_cursor_pos(textBox, cursor->selectionEndPos);

    env->SetIntArrayRegion(result, 0, 2, fill);
    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_erroratline0_infinipaint_InfiniPaintTextBoxInputConnection_nativeShiftSelection(
        JNIEnv *env, jclass clazz, jlong m_text_box_id, jint amount) {
    std::scoped_lock a{textboxMutex};
    input_android_text_box({
                                   .command = TextInputData::CommandType::SHIFT_CURSOR,
                                   .strData = "",
                                   .intData = {amount, 0, 0}
                           });

    CustomEvents::emit_event(
            CustomEvents::AndroidTextBoxInputEvent{
                    .command = CustomEvents::AndroidTextBoxInputEvent::CommandType::COMMIT_ALL,
            });
}

extern "C"
JNIEXPORT void JNICALL
Java_com_erroratline0_infinipaint_InfiniPaintTextBoxInputConnection_nativeSetSelection(JNIEnv *env,
                                                                                       jclass clazz,
                                                                                       jlong m_text_box_id,
                                                                                       jint start,
                                                                                       jint end) {
    std::scoped_lock a{textboxMutex};
    input_android_text_box({
                                   .command = TextInputData::CommandType::SET_CURSOR,
                                   .intData = {std::min(start, end), std::max(start, end), 0}
                           });

    CustomEvents::emit_event(
            CustomEvents::AndroidTextBoxInputEvent{
                    .command = CustomEvents::AndroidTextBoxInputEvent::CommandType::COMMIT_ALL,
            });
}

extern "C"
JNIEXPORT void JNICALL
Java_com_erroratline0_infinipaint_InfiniPaintNetworkService_networkServiceStarted(JNIEnv *env, jclass clazz) {
    NetThreadManager::get().init_thread();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_erroratline0_infinipaint_InfiniPaintNetworkService_networkServiceDestroyed(JNIEnv *env, jclass clazz) {
    NetThreadManager::get().destroy_thread();
}

#endif
