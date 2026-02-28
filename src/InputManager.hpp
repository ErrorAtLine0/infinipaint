#pragma once
#include <Eigen/Dense>
#include <array>
#include "RichText/TextStyleModifier.hpp"
#include "RichText/TextBox.hpp"
#include "TimePoint.hpp"
#include <Helpers/Hashes.hpp>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <queue>
#include <Helpers/CallbackManager.hpp>
#include "UndoManager.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

using namespace Eigen;

struct InputManager {
    InputManager();

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

    bool toggleFullscreen = false;

    struct KeyData {
        bool pressed = false;
        bool held = false;
        bool repeat = false;
        std::chrono::steady_clock::time_point lastPressTime;
    };

    struct Mouse {
        void set_pos(const Vector2f& newPos);
        Vector2f scrollAmount = {0, 0};
        Vector2f pos = {0, 0};
        Vector2f previousPos = {0, 0};
        Vector2f move = {0, 0};
        bool leftDown = false;
        bool rightDown = false;
        bool middleDown = false;
        uint8_t leftClicks = 0;
        uint8_t rightClicks = 0;
        uint8_t middleClicks = 0;
        Vector2f lastPos = {0, 0};
        friend struct InputManager;
    } mouse;

    struct Touch {
        bool isDown = false;
        int leftClicksSaved = 0;
        std::chrono::steady_clock::time_point lastLeftClickTime;
    } touch;

    struct Pen {
        bool inProximity = false;
        bool isDown = false;
        bool isEraser = false;
        float pressure = 0.0f;
        int leftClicksSaved = 0;
        std::chrono::steady_clock::time_point lastPenLeftClickTime;
        std::array<KeyData, 256> buttons;
    } pen;

    struct Text {
        std::string newInput;
        bool lastAcceptingTextInputVal = false;

        void set_accepting_input();
        bool get_accepting_input();

        void set_rich_text_box_input(const std::shared_ptr<RichText::TextBox>& nTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& nCursor, bool isRichTextBox, const std::optional<RichText::TextStyleModifier::ModifierMap>& nModMap = std::nullopt);
        void add_text_to_textbox(const std::string& inputText);

        void add_textbox_undo(const RichText::TextBox::Cursor& prevCursor, const RichText::TextData& prevRichText);
        void do_textbox_operation_with_undo(const std::function<void()>& func);

        private:
            bool isNextPasteRich = false;

            bool isRichTextBox;
            std::shared_ptr<RichText::TextBox> textBox;
            std::shared_ptr<RichText::TextBox::Cursor> cursor;
            std::optional<RichText::TextStyleModifier::ModifierMap> modMap;

            bool newIsRichTextBox;
            std::shared_ptr<RichText::TextBox> newTextBox;
            std::shared_ptr<RichText::TextBox::Cursor> newCursor;
            std::optional<RichText::TextStyleModifier::ModifierMap> newModMap;

            UndoManager textboxUndo;

            bool acceptingInput = false;
            bool acceptingInputNew = false;

            friend struct InputManager;
    } text;

    struct DroppedItem {
        Vector2f pos;
        std::optional<std::string> dataText;
        std::optional<std::filesystem::path> dataPath;
    };

#ifdef __APPLE__
    SDL_Keymod CTRL_MOD = SDL_KMOD_GUI;
#else
    SDL_Keymod CTRL_MOD = SDL_KMOD_CTRL;
#endif

    int stopKeyInput = 0;
    std::optional<Vector2ui32> lastPressedKeybind;

    std::vector<DroppedItem> droppedItems;

    void text_input_silence_everything();

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
        KEY_TEXT_DOWN,
        KEY_TEXT_UP,
        KEY_TEXT_LEFT,
        KEY_TEXT_RIGHT,
        KEY_TEXT_BACKSPACE,
        KEY_TEXT_DELETE,
        KEY_TEXT_HOME,
        KEY_TEXT_END,
        KEY_TEXT_SHIFT,
        KEY_TEXT_COPY,
        KEY_TEXT_CUT,
        KEY_TEXT_PASTE,
        KEY_TEXT_ENTER,
        KEY_TEXT_TAB,
        KEY_TEXT_CTRL,
        KEY_TEXT_META,
        KEY_TEXT_SELECTALL,
        KEY_TEXT_UNDO,
        KEY_TEXT_REDO,
        KEY_GENERIC_ESCAPE,
        KEY_GENERIC_LSHIFT,
        KEY_GENERIC_LALT,
        KEY_GENERIC_LCTRL,
        KEY_GENERIC_LMETA,
        KEY_COUNT
    };

    typedef unsigned KeyCode;

    bool ctrl_or_meta_held();
    void stop_key_input();
    void set_pen_button_up(const SDL_PenButtonEvent& e);
    void set_pen_button_down(const SDL_PenButtonEvent& e);
    uint32_t make_generic_key_mod(SDL_Keymod m);

    void set_key_up(const SDL_KeyboardEvent& e, KeyCode kCode);
    void set_key_down(const SDL_KeyboardEvent& e, KeyCode kCode);
    void backend_key_up_update(const SDL_KeyboardEvent& e);
    void backend_key_down_update(const SDL_KeyboardEvent& e);

    void set_clipboard_str(std::string_view s);
    void set_clipboard_plain_and_richtext_pair(const std::pair<std::string, RichText::TextData>& plainAndRichtextPair);
    std::string get_clipboard_str_SDL();
    void get_clipboard_image_data_SDL(const std::function<void(std::string_view data)>& callback);
    void call_text_paste(bool isRichTextPaste);
    void process_text_paste(const std::string& plainClipboardStr);

    std::optional<RichText::TextData> lastCopiedRichText;

    std::string key_assignment_to_str(const Vector2ui32& k);
    Vector2ui32 key_assignment_from_str(const std::string& s);

    const KeyData& key(KeyCode kCode);

    CallbackManager<SDL_KeyboardEvent> sdlKeyCallbacks;

    struct KeyCallbackArgs {
        bool down;
        bool repeat;
    };
    typedef CallbackManager<KeyCallbackArgs> KeyCallbackManager;
    std::array<KeyCallbackManager, KEY_COUNT> keyCallbacks;

    std::unordered_map<Vector2ui32, KeyCode> keyAssignments;
    std::unordered_map<Vector2ui32, KeyCode> defaultKeyAssignments;
    std::array<KeyData, KEY_COUNT> keys;
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
