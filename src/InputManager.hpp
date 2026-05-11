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

#pragma once
#include <Eigen/Dense>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_touch.h>
#include <array>
#include "RichText/TextBox.hpp"
#include <Helpers/Hashes.hpp>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <Helpers/CallbackManager.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include <Helpers/SCollision.hpp>

#include "CustomEvents.hpp"

using namespace Eigen;
class MainProgram;


struct InputManagerCallPasteInfo {
    std::optional<Vector2f> pastePosition;
    bool allowRichText = true;
};

struct InputManager {
    InputManager(MainProgram& initMain);

    void update();
    void frame_reset(const Vector2i& windowSize);

    // Just renamed SDL3 cursor types, but if we need to change to a different backend we can
    // translate this enum to different commands in main.cpp
    enum class SystemCursorType : unsigned {
        DEFAULT = 0,
        TEXT,
        WAIT,
        CROSSHAIR,
        PROGRESS,
        NWSE_RESIZE,
        NESW_RESIZE,
        EW_RESIZE,
        NS_RESIZE,
        MOVE,
        NOT_ALLOWED,
        POINTER,
        NW_RESIZE,
        N_RESIZE,
        NE_RESIZE,
        E_RESIZE,  
        SE_RESIZE,
        S_RESIZE,
        SW_RESIZE,
        W_RESIZE
    } cursorIcon;
    bool hideCursor = false;

    struct KeyData {
        bool held = false;
        std::chrono::steady_clock::time_point lastPressTime;
    };

    struct Mouse {
        void set_pos(const Vector2f& newPos);
        Vector2f pos = {0, 0};
        bool leftDown = false;
        bool rightDown = false;
        bool middleDown = false;
    } mouse;

    struct Touch {
        uint8_t leftClicksSaved = 0;
        uint8_t fingerTapsSaved = 0;
        std::chrono::steady_clock::time_point lastLeftClickTime;
        std::chrono::steady_clock::time_point lastFingerTapTime;
        std::vector<SDL_TouchFingerEvent> fingers;
        enum TypeOfTouchEvent {
            NO_TOUCH_EVENT,
            ONE_FINGER_EVENT,
            TWO_FINGER_EVENT,
            EVENT_DONE
        } touchEventType = EVENT_DONE;
    } touch;

    struct Pen {
        bool inProximity = false;
        bool isDown = false;
        bool isEraser = false;
        float pressure = 0.0f;
        uint8_t leftClicksSaved = 0;
        Vector2f previousPos;

        std::chrono::steady_clock::time_point lastPenLeftClickTime;
        std::array<KeyData, 256> buttons;
    } pen;

    bool isTouchDevice = true;

    enum AndroidInputType : int {
        ANDROIDTEXT_TYPE_CLASS_DATETIME = 0x00000004,
        ANDROIDTEXT_TYPE_CLASS_NUMBER = 0x00000002,
        ANDROIDTEXT_TYPE_CLASS_PHONE = 0x00000003,
        ANDROIDTEXT_TYPE_CLASS_TEXT = 0x00000001,
        ANDROIDTEXT_TYPE_DATETIME_VARIATION_DATE = 0x00000010,
        ANDROIDTEXT_TYPE_DATETIME_VARIATION_NORMAL = 0x00000000,
        ANDROIDTEXT_TYPE_DATETIME_VARIATION_TIME = 0x00000020,
        ANDROIDTEXT_TYPE_MASK_CLASS = 0x0000000f,
        ANDROIDTEXT_TYPE_MASK_FLAGS = 0x00fff000,
        ANDROIDTEXT_TYPE_MASK_VARIATION = 0x00000ff0,
        ANDROIDTEXT_TYPE_NULL = 0x00000000,
        ANDROIDTEXT_TYPE_NUMBER_FLAG_DECIMAL = 0x00002000,
        ANDROIDTEXT_TYPE_NUMBER_FLAG_SIGNED = 0x00001000,
        ANDROIDTEXT_TYPE_NUMBER_VARIATION_NORMAL = 0x00000000,
        ANDROIDTEXT_TYPE_NUMBER_VARIATION_PASSWORD = 0x00000010,
        ANDROIDTEXT_TYPE_TEXT_FLAG_AUTO_COMPLETE = 0x00010000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_AUTO_CORRECT = 0x00008000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_CAP_CHARACTERS = 0x00001000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_CAP_SENTENCES = 0x00004000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_CAP_WORDS = 0x00002000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_ENABLE_TEXT_CONVERSION_SUGGESTIONS = 0x00100000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_ENABLE_TEXT_SUGGESTION_SELECTED = 0x00200000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_IME_MULTI_LINE = 0x00040000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_MULTI_LINE = 0x00020000,
        ANDROIDTEXT_TYPE_TEXT_FLAG_NO_SUGGESTIONS = 0x00080000,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_EMAIL_ADDRESS = 0x00000020,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_EMAIL_SUBJECT = 0x00000030,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_FILTER = 0x000000b0,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_LONG_MESSAGE = 0x00000050,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_NORMAL = 0x00000000,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_PASSWORD = 0x00000080,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_PERSON_NAME = 0x00000060,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_PHONETIC = 0x000000c0,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_POSTAL_ADDRESS = 0x00000070,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_SHORT_MESSAGE = 0x00000040,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_URI = 0x00000010,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_VISIBLE_PASSWORD = 0x00000090,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_WEB_EDIT_TEXT = 0x000000a0,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_WEB_EMAIL_ADDRESS = 0x000000d0,
        ANDROIDTEXT_TYPE_TEXT_VARIATION_WEB_PASSWORD = 0x000000e0
    };

    struct TextInputProperties {
        SDL_TextInputType inputType;
        SDL_Capitalization capitalization;
        bool autocorrect;
        bool multiline;
        int androidInputType;
    };

    struct TextBoxStartInfo {
        CustomEvents::InputTextBoxID id;
        std::optional<SCollision::AABB<float>> rect;
        TextInputProperties inputProperties;

        // Android textbox info
        std::shared_ptr<RichText::TextBox> textBox;
        std::shared_ptr<RichText::TextBox::Cursor> cursor;
        std::shared_ptr<RichText::TextStyleModifier::ModifierMap> modMap;
    };

    void refresh_receiving_text_box_input();
    void update_textbox_rectangle(CustomEvents::InputTextBoxID textboxID, std::optional<SCollision::AABB<float>> textboxRect);
    bool text_is_accepting_input();

    void text_update_textbox_rectangle();
    void update_text_box_area_offset();

    std::optional<CustomEvents::InputTextBoxID> currentTextboxID;
    std::optional<SDL_PropertiesID> textPropID;

#ifdef __APPLE__
    SDL_Keymod CTRL_MOD = SDL_KMOD_GUI;
#else
    SDL_Keymod CTRL_MOD = SDL_KMOD_CTRL;
#endif

    enum KeyCodeEnum : unsigned {
        // Assignable
        KEY_CAMERA_ROTATE_CLOCKWISE = 0,
        KEY_CAMERA_ROTATE_COUNTERCLOCKWISE,
        KEY_DRAW_DELETE,
        KEY_UNDO,
        KEY_REDO,
        KEY_NOGUI,
        KEY_FULLSCREEN,
        KEY_SAVE,
        KEY_SAVE_AS,
        KEY_COPY,
        KEY_CUT,
        KEY_PASTE,
        KEY_PASTE_IMAGE,
        KEY_DRAW_TOOL_BRUSH,
        KEY_DRAW_TOOL_ERASER,
        KEY_DRAW_TOOL_ZOOM,
        KEY_DRAW_TOOL_PAN,
        KEY_DRAW_TOOL_LASSOSELECT,
        KEY_DRAW_TOOL_TEXTBOX,
        KEY_DRAW_TOOL_RECTANGLE,
        KEY_DRAW_TOOL_ELLIPSE,
        KEY_DRAW_TOOL_RECTSELECT,
        KEY_DRAW_TOOL_EYEDROPPER,
        KEY_DRAW_TOOL_SCREENSHOT,
        KEY_DRAW_TOOL_LINE,
        KEY_SHOW_METRICS,
        KEY_OPEN_CHAT,
        KEY_SHOW_PLAYER_LIST,
        KEY_HOLD_TO_PAN,
        KEY_HOLD_TO_ZOOM,
        KEY_DESELECT_AND_EDIT_TOOL,

        KEY_ASSIGNABLE_COUNT, // Not a real key

        // Unassignable
        KEY_TEXT_BACKSPACE,
        KEY_TEXT_DELETE,
        KEY_TEXT_HOME,
        KEY_TEXT_END,
        KEY_TEXT_COPY,
        KEY_TEXT_CUT,
        KEY_TEXT_PASTE,
        KEY_TEXT_TAB,
        KEY_TEXT_SELECTALL,
        KEY_TEXT_UNDO,
        KEY_TEXT_REDO,
        KEY_GENERIC_UP,
        KEY_GENERIC_DOWN,
        KEY_GENERIC_LEFT,
        KEY_GENERIC_RIGHT,
        KEY_GENERIC_ENTER,
        KEY_GENERIC_ESCAPE,
        KEY_GENERIC_LSHIFT,
        KEY_GENERIC_LALT,
        KEY_GENERIC_LCTRL,
        KEY_GENERIC_LMETA,

        KEY_COUNT
    };

    typedef unsigned KeyCode;

    bool ctrl_or_meta_held();
    uint32_t make_generic_key_mod(SDL_Keymod m);
    void set_key_up(const SDL_KeyboardEvent& e, KeyCode kCode, bool acceptingTextInput);
    void set_key_down(const SDL_KeyboardEvent& e, KeyCode kCode, bool acceptingTextInput);
    std::vector<Vector2f> get_multiple_finger_positions();
    std::vector<Vector2f> get_multiple_finger_motions();
    void touch_finger_do_mouse_down();
    void touch_finger_do_mouse_up(const SDL_TouchFingerEvent& f);
    void touch_finger_do_mouse_motion(const SDL_TouchFingerEvent& f);

    Vector2f backend_cursor_pos_calculation(const Vector2f& cursorPos);
    Vector2f backend_cursor_delta_calculation(const Vector2f& cursorDelta);
    Vector2f backend_touch_cursor_pos_calculation(const Vector2f& cursorPos);
    Vector2f backend_touch_cursor_delta_calculation(const Vector2f& cursorDelta);

    void backend_input_text_event(const std::string& str);
    void backend_drop_file_event(const SDL_DropEvent& e);
    void backend_drop_text_event(const SDL_DropEvent& e);
    void backend_mouse_button_up_update(const SDL_MouseButtonEvent& e);
    void backend_mouse_button_down_update(const SDL_MouseButtonEvent& e);
    void backend_mouse_motion_update(const SDL_MouseMotionEvent& e);
    void backend_mouse_wheel_update(const SDL_MouseWheelEvent& e);
    void backend_key_up_update(const SDL_KeyboardEvent& e);
    void backend_key_down_update(const SDL_KeyboardEvent& e);
    void backend_pen_button_down_update(const SDL_PenButtonEvent& e);
    void backend_pen_button_up_update(const SDL_PenButtonEvent& e);
    void backend_pen_touch_down_update(const SDL_PenTouchEvent& e);
    void backend_pen_touch_up_update(const SDL_PenTouchEvent& e);
    void backend_pen_motion_update(const SDL_PenMotionEvent& e);
    void backend_pen_axis_update(const SDL_PenAxisEvent& e);
    void backend_touch_finger_down_update(const SDL_TouchFingerEvent& e);
    void backend_touch_finger_up_update(const SDL_TouchFingerEvent& e);
    void backend_touch_finger_motion_update(const SDL_TouchFingerEvent& e);
    void backend_window_resize_update();
    void backend_window_scale_update(const SDL_WindowEvent& e);

    void update_safe_area();

    void set_clipboard_str(std::string_view s);
    void set_clipboard_plain_and_richtext_pair(const std::pair<std::string, RichText::TextData>& plainAndRichtextPair);
    std::string get_clipboard_str_SDL();
    void get_clipboard_image_data_SDL(const std::function<void(std::string_view data)>& callback);

    void call_paste(CustomEvents::PasteEvent::DataType type, const InputManagerCallPasteInfo& info = {});
    void process_text_paste(const std::string& pasteStr, bool allowRichText);
    void process_image_paste(std::string_view pasteData, const std::optional<Vector2f>& pastePos);

    std::optional<RichText::TextData> lastCopiedRichText;

    std::string key_assignment_to_str(const Vector2ui32& k) const;
    Vector2ui32 key_assignment_from_str(const std::string& s) const;

    const KeyData& key(KeyCode kCode);

    struct TextCallbackArgs {
        std::string str;
    };

    struct KeyCallbackArgs {
        KeyCode key;
        bool down;
        bool repeat;
    };

    std::unordered_map<Vector2ui32, KeyCode> keyAssignments;
    std::unordered_map<Vector2ui32, KeyCode> defaultKeyAssignments;
    std::array<KeyData, KEY_COUNT> keys;

    enum class MouseButton : uint8_t {
        LEFT = 1,
        MIDDLE,
        RIGHT
    } button;

    enum class MouseDeviceType {
        MOUSE,
        PEN,
        TOUCH
    };

    struct MouseButtonCallbackArgs {
        MouseDeviceType deviceType;
        MouseButton button;
        bool down;
        uint8_t clicks;
        Vector2f pos;
    };

    struct MouseMotionCallbackArgs {
        MouseDeviceType deviceType;
        Vector2f pos;
        Vector2f move;
    };

    struct MouseWheelCallbackArgs {
        Vector2f mousePos;
        Vector2f amount;
        Vector2i tickAmount;
    };

    struct PenButtonCallbackArgs {
        uint8_t button;
        bool down;
        Vector2f pos;
    };

    struct PenTouchCallbackArgs {
        bool down;
        bool eraser;
        Vector2f pos;
    };

    struct PenMotionCallbackArgs {
        Vector2f pos;
        Vector2f move;
    };

    struct PenAxisCallbackArgs {
        Vector2f pos;
        SDL_PenAxis axis;
        float value;
    };

    struct DropCallbackArgs {
        Vector2f pos;
        const char* source;
        const char* data;
    };

    struct MultiFingerTouchCallbackArgs {
        bool down;
        std::vector<Vector2f> pos;
    };

    struct MultiFingerMotionCallbackArgs {
        std::vector<Vector2f> pos;
        std::vector<Vector2f> move;
    };

    struct FingerTouchCallbackArgs {
        SDL_FingerID fingerID;
        bool down;
        Vector2f pos;
        size_t fingerDownCount;
        int fingerTapCount;
    };

    struct FingerMotionCallbackArgs {
        SDL_FingerID fingerID;
        Vector2f pos;
        Vector2f move;
        size_t fingerDownCount;
    };

    static MouseButtonCallbackArgs convert_finger_touch_to_mouse_button(const FingerTouchCallbackArgs& touch);
    static MouseMotionCallbackArgs convert_finger_motion_to_mouse_motion(const FingerMotionCallbackArgs& motion);

    struct WindowResizeCallbackArgs {
        Vector2i size;
        SCollision::AABB<float> safeArea;
        float density;
    };

    struct WindowScaleCallbackArgs {
        float scale;
    };

    std::mutex safeAreaWithoutIMEMutex;
    std::optional<SCollision::AABB<float>> safeAreaWithoutIME;
    std::atomic<bool> excludeIMEFromSafeArea = false;
    Vector2f screenOffset = {0.0f, 0.0f};

    MainProgram& main;
};

NLOHMANN_JSON_SERIALIZE_ENUM(InputManager::KeyCodeEnum, {
    {InputManager::KEY_CAMERA_ROTATE_CLOCKWISE, "Rotate Canvas CW"},
    {InputManager::KEY_CAMERA_ROTATE_COUNTERCLOCKWISE, "Rotate Canvas CCW"},
    {InputManager::KEY_DRAW_DELETE, "Delete Selection"},
    {InputManager::KEY_UNDO, "Undo"},
    {InputManager::KEY_REDO, "Redo"},
    {InputManager::KEY_NOGUI, "Toggle GUI"},
    {InputManager::KEY_FULLSCREEN, "Toggle Fullscreen"},
    {InputManager::KEY_SAVE, "Save"},
    {InputManager::KEY_SAVE_AS, "Save As"},
    {InputManager::KEY_COPY, "Copy"},
    {InputManager::KEY_CUT, "Cut"},
    {InputManager::KEY_PASTE, "Paste"},
    {InputManager::KEY_PASTE_IMAGE, "Paste Image"},
    {InputManager::KEY_DRAW_TOOL_BRUSH, "Brush Tool"},
    {InputManager::KEY_DRAW_TOOL_ERASER, "Eraser Tool"},
    {InputManager::KEY_DRAW_TOOL_ZOOM, "Zoom Tool"},
    {InputManager::KEY_DRAW_TOOL_LASSOSELECT, "Lasso Select Tool"},
    {InputManager::KEY_DRAW_TOOL_PAN, "Pan Tool"},
    {InputManager::KEY_DRAW_TOOL_TEXTBOX, "Textbox Tool"},
    {InputManager::KEY_DRAW_TOOL_RECTANGLE, "Rectangle Tool"},
    {InputManager::KEY_DRAW_TOOL_ELLIPSE, "Ellipse Tool"},
    {InputManager::KEY_DRAW_TOOL_RECTSELECT, "Rect Select Tool"},
    {InputManager::KEY_DRAW_TOOL_EYEDROPPER, "Color Select Tool"},
    {InputManager::KEY_DRAW_TOOL_SCREENSHOT, "Take Screenshot"},
    {InputManager::KEY_DRAW_TOOL_LINE, "Line Tool"},
    {InputManager::KEY_SHOW_METRICS, "Show Metrics"},
    {InputManager::KEY_OPEN_CHAT, "Open Chat"},
    {InputManager::KEY_SHOW_PLAYER_LIST, "Show Player List"},
    {InputManager::KEY_HOLD_TO_PAN, "Hold to Pan"},
    {InputManager::KEY_HOLD_TO_ZOOM, "Hold to Zoom"},
    {InputManager::KEY_DESELECT_AND_EDIT_TOOL, "Deselect selection / Edit Tool"}
})
