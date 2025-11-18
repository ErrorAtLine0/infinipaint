#include "InputManager.hpp"

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_keycode.h>

#include <optional>

#include "Server/CommandList.hpp"

#ifdef __EMSCRIPTEN__
#include <EmscriptenHelpers/emscripten_browser_clipboard.h>

extern "C" {
    int isAcceptingInputEmscripten = 0;
    EMSCRIPTEN_KEEPALIVE inline int is_text_input_happening() {
        return isAcceptingInputEmscripten;
    }
}

#endif

InputManager::InputManager() {
    frame_reset({0, 0});

    defaultKeyAssignments[{0, SDLK_W}] = KEY_CAMERA_ROTATE_COUNTERCLOCKWISE;
    defaultKeyAssignments[{0, SDLK_Q}] = KEY_CAMERA_ROTATE_CLOCKWISE;
    defaultKeyAssignments[{0, SDLK_DELETE}] = KEY_DRAW_DELETE;
    defaultKeyAssignments[{CTRL_MOD, SDLK_Z}] = KEY_UNDO;
    defaultKeyAssignments[{CTRL_MOD, SDLK_R}] = KEY_REDO;
    defaultKeyAssignments[{0, SDLK_TAB}] = KEY_NOGUI;
    defaultKeyAssignments[{0, SDLK_F11}] = KEY_FULLSCREEN;
    defaultKeyAssignments[{CTRL_MOD, SDLK_S}] = KEY_SAVE;
    defaultKeyAssignments[{CTRL_MOD | SDL_KMOD_SHIFT, SDLK_S}] = KEY_SAVE_AS;
    defaultKeyAssignments[{CTRL_MOD, SDLK_C}] = KEY_COPY;
    defaultKeyAssignments[{CTRL_MOD, SDLK_X}] = KEY_CUT;
    defaultKeyAssignments[{CTRL_MOD, SDLK_V}] = KEY_PASTE;
    defaultKeyAssignments[{0, SDLK_B}] = KEY_DRAW_TOOL_BRUSH;
    defaultKeyAssignments[{0, SDLK_E}] = KEY_DRAW_TOOL_ERASER;
    defaultKeyAssignments[{0, SDLK_T}] = KEY_DRAW_TOOL_TEXTBOX;
    defaultKeyAssignments[{0, SDLK_R}] = KEY_DRAW_TOOL_RECTANGLE;
    defaultKeyAssignments[{0, SDLK_C}] = KEY_DRAW_TOOL_ELLIPSE;
    defaultKeyAssignments[{0, SDLK_X}] = KEY_DRAW_TOOL_RECTSELECT;
    defaultKeyAssignments[{0, SDLK_I}] = KEY_DRAW_TOOL_EYEDROPPER;
    defaultKeyAssignments[{0, SDLK_P}] = KEY_DRAW_TOOL_SCREENSHOT;
    defaultKeyAssignments[{0, SDLK_F1}] = KEY_OPEN_CHAT;
    defaultKeyAssignments[{0, SDLK_F3}] = KEY_SHOW_METRICS;
    defaultKeyAssignments[{0, SDLK_F2}] = KEY_SHOW_PLAYER_LIST;
    defaultKeyAssignments[{0, SDLK_SPACE}] = KEY_HOLD_TO_PAN;


#ifdef __EMSCRIPTEN__
    // Without this, SDL eats the CTRL-V event that initiates the paste event
    // https://github.com/pthom/hello_imgui/issues/3#issuecomment-1564536870
	EM_ASM({
		window.addEventListener('keydown', function(event) {
			if((event.ctrlKey || event.metaKey) && (event.key == 'v' || event.code == 'KeyV')) {
                if(Module["ccall"]('is_text_input_happening', 'number', [], []) === 1)
				    event.stopImmediatePropagation();
            }
		}, true);
	});

    emscripten_browser_clipboard::paste([](std::string&& pasteData, void* callbackData){
        InputManager* inMan = (InputManager*)callbackData;
        inMan->clipboardPasteEventHappened = true;
        inMan->clipboardPasteEventData["text/plain"] = pasteData;
        inMan->process_text_paste();
    }, this);
#endif

    keyAssignments = defaultKeyAssignments;
}

void InputManager::text_input_silence_everything() {
    text.set_accepting_input();
    for(auto& p : keys) {
        p.pressed = false;
        p.repeat = false;
        // Not silencing p.held, as that causes double presses on some keyboards
    }
    text.newInput.clear();
}

void InputManager::Text::set_rich_text_box_input(const std::shared_ptr<RichText::TextBox>& nTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& nCursor, const std::optional<RichText::TextStyleModifier::ModifierMap>& nModMap) {
    set_accepting_input();
    newTextBox = nTextBox;
    newCursor = nCursor;
    newModMap = nModMap;
}

void InputManager::Text::add_text_to_textbox(const std::string& inputText) {
    if(textBox && !inputText.empty())
        do_textbox_operation_with_undo([&]() {
            textBox->process_text_input(*cursor, inputText, modMap);
        });
}

void InputManager::Text::set_accepting_input() {
    acceptingInputNew = true;
}

bool InputManager::Text::get_accepting_input() {
    return acceptingInput;
}

void InputManager::Text::add_textbox_undo(const RichText::TextBox::Cursor& prevCursor, const RichText::TextData& prevRichText) {
    if(textBox) {
        textboxUndo.push({[textBox = textBox, cursor = cursor, prevCursor = prevCursor, prevRichText = prevRichText]() {
            textBox->set_rich_text_data(prevRichText);
            cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = std::max(prevCursor.selectionEndPos, prevCursor.selectionBeginPos);
            cursor->previousX = std::nullopt;
            return true;
        },
        [textBox = textBox, cursor = cursor, currentCursor = *cursor, currentRichText = textBox->get_rich_text_data()]() {
            textBox->set_rich_text_data(currentRichText);
            cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = std::max(currentCursor.selectionEndPos, currentCursor.selectionBeginPos);
            cursor->previousX = std::nullopt;
            return true;
        }});
    }
}

void InputManager::Text::do_textbox_operation_with_undo(const std::function<void()>& func) {
    if(textBox) {
        auto prevRichText = textBox->get_rich_text_data();
        auto prevCursor = *cursor;
        func();
        add_textbox_undo(prevCursor, prevRichText);
    }
}

bool InputManager::get_clipboard_paste_happened() {
#ifdef __EMSCRIPTEN__
    return clipboardPasteEventHappened;
#else
    return key(KEY_TEXT_PASTE).repeat;
#endif
}

std::string InputManager::get_clipboard_str() {
#ifdef __EMSCRIPTEN__
    auto it = clipboardPasteEventData.find("text/plain");
    if(it == clipboardPasteEventData.end())
        return "";
    return it->second;
#else
    char* data = SDL_GetClipboardText();
    std::string toRet(data);
    SDL_free(data);
    return toRet;
#endif
}

std::string InputManager::get_clipboard_data_for_mimetype(const std::string& mimeType) {
#ifdef __EMSCRIPTEN__
    auto it = clipboardPasteEventData.find(mimeType);
    if(it == clipboardPasteEventData.end())
        return "";
    return it->second;
#else
    size_t datSize = 0;
    void* dat = SDL_GetClipboardData(mimeType.c_str(), &datSize);
    if(!dat)
        return "";
    return std::string(static_cast<char*>(dat), datSize);
#endif
}

void InputManager::set_clipboard_str(std::string_view s) {
#ifdef __EMSCRIPTEN__
    emscripten_browser_clipboard::copy(std::string(s));
    lastCopiedRichText = std::nullopt;
#else
    SDL_SetClipboardText(s.data());
#endif
}

void InputManager::set_clipboard_plain_and_richtext_pair(const std::pair<std::string, RichText::TextData>& plainAndRichtextPair) {
    set_clipboard_str(plainAndRichtextPair.first);
    lastCopiedRichText = plainAndRichtextPair.second;
}

void InputManager::set_clipboard_data(const std::unordered_map<std::string, std::string>& newClipboardData) {
    SDL_ClearClipboardData();
    clipboardData = newClipboardData;

    std::vector<const char*> mimeTypes;
    for(auto& [k, v] : clipboardData)
        mimeTypes.emplace_back(k.c_str());

    SDL_SetClipboardData(
    [](void *userdata, const char *mime_type, size_t *size) {
        std::unordered_map<std::string, std::string>& clipboardData = *static_cast<std::unordered_map<std::string, std::string>*>(userdata);
        std::string mimeTypeStr(mime_type);
        auto it = clipboardData.find(mimeTypeStr);
        std::cout << "requesting: " << mime_type << std::endl;
        if(it == clipboardData.end())
            return static_cast<const void*>(nullptr);
        *size = it->second.length();
        return static_cast<const void*>(it->second.c_str());
    },
    [](void *userdata) {
    },
    static_cast<void*>(&clipboardData), mimeTypes.data(), mimeTypes.size());
}

std::string InputManager::key_assignment_to_str(const Vector2ui32& k) {
    std::string toRet;
    if(k.x() & SDL_KMOD_CTRL)
        toRet += "CTRL+";
    if(k.x() & SDL_KMOD_ALT)
        toRet += "ALT+";
    if(k.x() & SDL_KMOD_SHIFT)
        toRet += "SHIFT+";
    if(k.x() & SDL_KMOD_GUI)
        toRet += "META+";
    toRet += SDL_GetKeyName(k.y());
    return toRet;
}

Vector2ui32 InputManager::key_assignment_from_str(const std::string& s) {
    Vector2ui32 toRet = {0, 0};
    if(s.find("CTRL+") != std::string::npos)
        toRet.x() |= SDL_KMOD_CTRL;
    if(s.find("ALT+") != std::string::npos)
        toRet.x() |= SDL_KMOD_ALT;
    if(s.find("SHIFT+") != std::string::npos)
        toRet.x() |= SDL_KMOD_SHIFT;
    if(s.find("META+") != std::string::npos)
        toRet.x() |= SDL_KMOD_GUI;
    size_t startInd = s.find_last_of("+");
    if(startInd == std::string::npos)
        startInd = 0;
    else
        startInd++;
    toRet.y() = SDL_GetKeyFromName(s.substr(startInd).c_str());
    if(toRet.y() == 0)
        toRet.x() = 0;
    return toRet;
}

const InputManager::KeyData& InputManager::key(KeyCode kCode) {
    return keys[kCode];
}

void InputManager::set_key_down(const SDL_KeyboardEvent& e, KeyCode kCode) {
    auto& k = keys[kCode];
    if(!k.held || (std::chrono::steady_clock::now() - k.lastPressTime) > std::chrono::milliseconds(500)) // Timer works around some systems that emit a keypress twice (a repeat and a press event)
        k.repeat = true;
    if(!k.held) { // We use if statements instead of setting booleans so that values arent forced to false
        k.pressed = true;
        k.lastPressTime = std::chrono::steady_clock::now();
    }
    k.held = true;
}

void InputManager::set_key_up(const SDL_KeyboardEvent& e, KeyCode kCode) {
    keys[kCode].held = false;
}

void InputManager::set_pen_button_down(const SDL_PenButtonEvent& e) {
    pen.buttons[e.button].held = true;
    pen.buttons[e.button].pressed = true;
}

void InputManager::set_pen_button_up(const SDL_PenButtonEvent& e) {
    pen.buttons[e.button].held = false;
}

uint32_t InputManager::make_generic_key_mod(SDL_Keymod m) {
    uint32_t toRet = 0;
    if(m & SDL_KMOD_CTRL)
        toRet |= SDL_KMOD_CTRL;
    if(m & SDL_KMOD_ALT)
        toRet |= SDL_KMOD_ALT;
    if(m & SDL_KMOD_GUI)
        toRet |= SDL_KMOD_GUI;
    if(m & SDL_KMOD_SHIFT)
        toRet |= SDL_KMOD_SHIFT;
    return toRet;
}

void InputManager::backend_key_down_update(const SDL_KeyboardEvent& e) {
    auto kPress = e.key;
    auto kMod = e.mod;

    if(kPress != SDLK_LSHIFT && kPress != SDLK_RSHIFT && kPress != SDLK_LCTRL && kPress != SDLK_RCTRL &&
       kPress != SDLK_LGUI  && kPress != SDLK_RGUI &&
       kPress != SDLK_LALT   && kPress != SDLK_RALT) {
        lastPressedKeybind = {make_generic_key_mod(kMod), kPress};
    }

    // e.key != SDLK_LMETA  && e.key != SDLK_RMETA  && 

    if(stopKeyInput)
        return;

    switch(kPress) {
        case SDLK_UP:
            set_key_down(e, KEY_TEXT_UP);
            if(text.textBox && key(KEY_TEXT_UP).repeat)
                text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::UP, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
            break;
        case SDLK_DOWN:
            set_key_down(e, KEY_TEXT_DOWN);
            if(text.textBox && key(KEY_TEXT_DOWN).repeat)
                text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::DOWN, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
            break;
        case SDLK_LEFT:
            set_key_down(e, KEY_TEXT_LEFT);
            if(text.textBox && key(KEY_TEXT_LEFT).repeat)
                text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::LEFT, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
            break;
        case SDLK_RIGHT:
            set_key_down(e, KEY_TEXT_RIGHT);
            if(text.textBox && key(KEY_TEXT_RIGHT).repeat)
                text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::RIGHT, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
            break;
        case SDLK_BACKSPACE:
            set_key_down(e, KEY_TEXT_BACKSPACE);
            if(text.textBox && key(KEY_TEXT_BACKSPACE).repeat)
                text.do_textbox_operation_with_undo([&]() {
                    text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::BACKSPACE, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
                });
            break;
        case SDLK_DELETE:
            set_key_down(e, KEY_TEXT_DELETE);
            if(text.textBox && key(KEY_TEXT_DELETE).repeat)
                text.do_textbox_operation_with_undo([&]() {
                    text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::DEL, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
                });
            break;
        case SDLK_HOME:
            set_key_down(e, KEY_TEXT_HOME);
            if(text.textBox && key(KEY_TEXT_HOME).repeat)
                text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::HOME, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
            break;
        case SDLK_END:
            set_key_down(e, KEY_TEXT_END);
            if(text.textBox && key(KEY_TEXT_END).repeat)
                text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::END, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
            break;
        case SDLK_LSHIFT:
            set_key_down(e, KEY_TEXT_SHIFT);
            set_key_down(e, KEY_GENERIC_LSHIFT);
            break;
        case SDLK_LALT:
            set_key_down(e, KEY_GENERIC_LALT);
            break;
        case SDLK_LCTRL:
            set_key_down(e, KEY_TEXT_CTRL);
            set_key_down(e, KEY_GENERIC_LCTRL);
            break;
        case SDLK_LMETA:
            set_key_down(e, KEY_TEXT_META);
            set_key_down(e, KEY_GENERIC_LMETA);
            break;
        case SDLK_C:
            // Use either Ctrl or Meta (command for Mac) keys. We do either instead of checking, since checking can get complicated on Emscripten
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL))
                set_key_down(e, KEY_TEXT_COPY);
            if(text.textBox && key(KEY_TEXT_COPY).repeat)
                set_clipboard_plain_and_richtext_pair(text.textBox->process_copy(*text.cursor));
            break;
        case SDLK_X:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL))
                set_key_down(e, KEY_TEXT_CUT);
            if(text.textBox && key(KEY_TEXT_CUT).repeat)
                text.do_textbox_operation_with_undo([&]() {
                    set_clipboard_plain_and_richtext_pair(text.textBox->process_cut(*text.cursor));
                });
            break;
        case SDLK_V:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL))
                set_key_down(e, KEY_TEXT_PASTE);
            if(text.textBox && key(KEY_TEXT_PASTE).repeat)
                text.do_textbox_operation_with_undo([&]() {
                    process_text_paste();
                });
            break;
        case SDLK_A:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL))
                set_key_down(e, KEY_TEXT_SELECTALL);
            if(text.textBox && key(KEY_TEXT_SELECTALL).repeat)
                text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::SELECT_ALL, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
            break;
        case SDLK_Z:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL))
                set_key_down(e, KEY_TEXT_UNDO);
            if(text.textBox && key(KEY_TEXT_UNDO).repeat)
                text.textboxUndo.undo();
            break;
        case SDLK_R:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL))
                set_key_down(e, KEY_TEXT_REDO);
            if(text.textBox && key(KEY_TEXT_REDO).repeat)
                text.textboxUndo.redo();
            break;
        case SDLK_RETURN: {
            set_key_down(e, KEY_TEXT_ENTER);
            if(text.textBox && key(KEY_TEXT_ENTER).repeat)
                text.do_textbox_operation_with_undo([&]() {
                    text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::ENTER, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
                });
            break;
        }
        case SDLK_TAB: {
            set_key_down(e, KEY_TEXT_TAB);
            if(text.textBox && key(KEY_TEXT_TAB).repeat)
                text.do_textbox_operation_with_undo([&]() {
                    text.textBox->process_key_input(*text.cursor, RichText::TextBox::InputKey::TAB, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, text.modMap);
                });
            break;
        }
        case SDLK_ESCAPE:
            set_key_down(e, KEY_GENERIC_ESCAPE);
            break;
        default:
            break;
    }

    if(text.get_accepting_input())
        return;

    auto f = keyAssignments.find({make_generic_key_mod(kMod), kPress});
    if(f != keyAssignments.end())
        set_key_down(e, f->second);
}

void InputManager::process_text_paste() {
    if(text.textBox) {
        // Workaround for not being able to copy richtext to system clipboard, this should at least work within the application itself
        std::string plainClipboardStr = get_clipboard_str();
        if(lastCopiedRichText.has_value()) {
            if(lastCopiedRichText.value().get_plain_text() == plainClipboardStr)
                text.textBox->process_rich_text_input(*text.cursor, lastCopiedRichText.value());
            else
                text.textBox->process_text_input(*text.cursor, plainClipboardStr, text.modMap);
        }
        else
            text.textBox->process_text_input(*text.cursor, plainClipboardStr, text.modMap);
    }
}

bool InputManager::ctrl_or_meta_held() {
    return key(KEY_GENERIC_LCTRL).held || key(KEY_GENERIC_LMETA).held;
}

void InputManager::backend_key_up_update(const SDL_KeyboardEvent& e) {
    auto kPress = e.key;
    //auto kMod = e.mod;

    switch(kPress) {
        case SDLK_UP:
            set_key_up(e, KEY_TEXT_UP);
            break;
        case SDLK_DOWN:
            set_key_up(e, KEY_TEXT_DOWN);
            break;
        case SDLK_LEFT:
            set_key_up(e, KEY_TEXT_LEFT);
            break;
        case SDLK_RIGHT:
            set_key_up(e, KEY_TEXT_RIGHT);
            break;
        case SDLK_BACKSPACE:
            set_key_up(e, KEY_TEXT_BACKSPACE);
            break;
        case SDLK_DELETE:
            set_key_up(e, KEY_TEXT_DELETE);
            break;
        case SDLK_HOME:
            set_key_up(e, KEY_TEXT_HOME);
            break;
        case SDLK_END:
            set_key_up(e, KEY_TEXT_END);
            break;
        case SDLK_LSHIFT:
            set_key_up(e, KEY_TEXT_SHIFT);
            set_key_up(e, KEY_GENERIC_LSHIFT);
            break;
        case SDLK_LALT:
            set_key_up(e, KEY_GENERIC_LALT);
            break;
        case SDLK_LCTRL:
            set_key_up(e, KEY_TEXT_CTRL);
            set_key_up(e, KEY_GENERIC_LCTRL);
            break;
        case SDLK_C:
            set_key_up(e, KEY_TEXT_COPY);
            break;
        case SDLK_X:
            set_key_up(e, KEY_TEXT_CUT);
            break;
        case SDLK_V:
            set_key_up(e, KEY_TEXT_PASTE);
            break;
        case SDLK_A:
            set_key_up(e, KEY_TEXT_SELECTALL);
            break;
        case SDLK_Z:
            set_key_up(e, KEY_TEXT_UNDO);
            break;
        case SDLK_R:
            set_key_up(e, KEY_TEXT_REDO);
            break;
        case SDLK_RETURN:
            set_key_up(e, KEY_TEXT_ENTER);
            break;
        case SDLK_TAB:
            set_key_up(e, KEY_TEXT_TAB);
            break;
        case SDLK_ESCAPE:
            set_key_up(e, KEY_GENERIC_ESCAPE);
            break;
        default:
            break;
    }

    for(auto& p : keyAssignments) {
        if(p.first.y() == kPress) // Dont try matching modifiers when setting key to go up
            set_key_up(e, p.second);
    }
}

void InputManager::stop_key_input() {
    stopKeyInput = 2;
}

void InputManager::Mouse::set_pos(const Vector2f& newPos) {
    pos = newPos;
    move = newPos - lastPos;
}

void InputManager::frame_reset(const Vector2i& windowSize) {
    mouse.lastPos = mouse.pos;
    mouse.move = Vector2f({0.0f, 0.0f});
    mouse.scrollAmount = Vector2f({0.0f, 0.0f});
    mouse.leftClicks = 0;
    mouse.rightClicks = 0;
    mouse.middleClicks = 0;
    text.acceptingInput = text.acceptingInputNew;
    text.acceptingInputNew = false;
    if(text.cursor != text.newCursor || text.textBox != text.newTextBox)
        text.textboxUndo.clear();
    text.cursor = text.newCursor;
    text.textBox = text.newTextBox;
    text.modMap = text.newModMap;
    text.newCursor = nullptr;
    text.newTextBox = nullptr;

    text.newInput.clear();
    droppedItems.clear();

    cursorIcon = SystemCursorType::DEFAULT;

    for(auto& p : keys) {
        p.pressed = false;
        p.repeat = false;
    }

    for(auto& p : pen.buttons)
        p.pressed = false;

    lastPressedKeybind = std::nullopt;
    if(stopKeyInput)
        stopKeyInput--;

    toggleFullscreen = false;
    hideCursor = false;

#ifdef __EMSCRIPTEN__
    clipboardPasteEventHappened = false;
    clipboardPasteEventData.clear();
    isAcceptingInputEmscripten = text.acceptingInput ? 1 : 0;
#endif
}
