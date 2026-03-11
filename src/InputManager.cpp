#include "InputManager.hpp"

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>

#include <SDL3/SDL_pen.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_timer.h>
#include <optional>

#include <Helpers/Logger.hpp>

#ifdef _WIN32
    #include <include/core/SkStream.h>
    #include <include/encode/SkPngEncoder.h>
    #include "../deps/clip/clip.h"
#endif

#ifdef __EMSCRIPTEN__
#include <EmscriptenHelpers/emscripten_browser_clipboard.h>

extern "C" {
    int isAcceptingInputEmscripten = 0;
    EMSCRIPTEN_KEEPALIVE inline int is_text_input_happening() {
        return isAcceptingInputEmscripten;
    }
}
#endif

#include "MainProgram.hpp"

template <typename T> std::optional<T> shared_ptr_to_opt(const std::shared_ptr<T> o) {
    if(o)
        return *o;
    else
        return std::nullopt;
}

InputManager::InputManager(MainProgram& initMain):
    main(initMain) {
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
    defaultKeyAssignments[{CTRL_MOD | SDL_KMOD_SHIFT, SDLK_V}] = KEY_PASTE_IMAGE;
    defaultKeyAssignments[{0, SDLK_B}] = KEY_DRAW_TOOL_BRUSH;
    defaultKeyAssignments[{0, SDLK_E}] = KEY_DRAW_TOOL_ERASER;
    defaultKeyAssignments[{0, SDLK_U}] = KEY_DRAW_TOOL_ZOOM;
    defaultKeyAssignments[{0, SDLK_L}] = KEY_DRAW_TOOL_LASSOSELECT;
    defaultKeyAssignments[{0, SDLK_H}] = KEY_DRAW_TOOL_PAN;
    defaultKeyAssignments[{0, SDLK_T}] = KEY_DRAW_TOOL_TEXTBOX;
    defaultKeyAssignments[{0, SDLK_R}] = KEY_DRAW_TOOL_RECTANGLE;
    defaultKeyAssignments[{0, SDLK_C}] = KEY_DRAW_TOOL_ELLIPSE;
    defaultKeyAssignments[{0, SDLK_X}] = KEY_DRAW_TOOL_RECTSELECT;
    defaultKeyAssignments[{0, SDLK_I}] = KEY_DRAW_TOOL_EYEDROPPER;
    defaultKeyAssignments[{0, SDLK_P}] = KEY_DRAW_TOOL_SCREENSHOT;
    defaultKeyAssignments[{0, SDLK_N}] = KEY_DRAW_TOOL_LINE;
    defaultKeyAssignments[{0, SDLK_F1}] = KEY_OPEN_CHAT;
    defaultKeyAssignments[{0, SDLK_F3}] = KEY_SHOW_METRICS;
    defaultKeyAssignments[{0, SDLK_F2}] = KEY_SHOW_PLAYER_LIST;
    defaultKeyAssignments[{0, SDLK_SPACE}] = KEY_HOLD_TO_PAN;
    defaultKeyAssignments[{0, SDLK_Z}] = KEY_HOLD_TO_ZOOM;
    defaultKeyAssignments[{0, SDLK_ESCAPE}] = KEY_DESELECT_AND_EDIT_TOOL;

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
    emscripten_browser_clipboard::paste_event([](std::string&& pasteData, void* callbackData){
        InputManager* inMan = (InputManager*)callbackData;
        inMan->text.isNextPasteRich = true;
        std::string pData = pasteData;
        inMan->process_text_paste(pData);
    }, this);
#endif

    keyAssignments = defaultKeyAssignments;
}

void InputManager::set_rich_text_box_input_front(const std::shared_ptr<RichText::TextBox>& nTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& nCursor, bool isRichTextBox, const std::shared_ptr<SCollision::AABB<float>>& textboxRect, const TextInputProperties& inputProps, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& nModMap) {
    Text::TextBoxInfo textBoxInfo;
    textBoxInfo.textBox = nTextBox;
    textBoxInfo.cursor = nCursor;
    textBoxInfo.modMap = nModMap;
    textBoxInfo.isRichTextBox = isRichTextBox;
    textBoxInfo.rect = textboxRect;
    textBoxInfo.textInputProperties = inputProps;
    text.textBoxes.emplace_front(textBoxInfo);
    text.update_accepting_input(main.window.sdlWindow);
}

void InputManager::set_rich_text_box_input_back(const std::shared_ptr<RichText::TextBox>& nTextBox, const std::shared_ptr<RichText::TextBox::Cursor>& nCursor, bool isRichTextBox, const std::shared_ptr<SCollision::AABB<float>>& textboxRect, const TextInputProperties& inputProps, const std::shared_ptr<RichText::TextStyleModifier::ModifierMap>& nModMap) {
    Text::TextBoxInfo textBoxInfo;
    textBoxInfo.textBox = nTextBox;
    textBoxInfo.cursor = nCursor;
    textBoxInfo.modMap = nModMap;
    textBoxInfo.isRichTextBox = isRichTextBox;
    textBoxInfo.rect = textboxRect;
    textBoxInfo.textInputProperties = inputProps;
    text.textBoxes.emplace_back(textBoxInfo);
    text.update_accepting_input(main.window.sdlWindow);
}

void InputManager::remove_rich_text_box_input(const std::shared_ptr<RichText::TextBox>& nTextBox) {
    std::erase_if(text.textBoxes, [&nTextBox](Text::TextBoxInfo& info) {
        return (info.textBox == nTextBox);
    });
    text.update_accepting_input(main.window.sdlWindow);
}

void InputManager::Text::add_text_to_textbox(const std::string& inputText) {
    if(!textBoxes.empty() && !inputText.empty()) {
        auto& frontTextBox = textBoxes.front();
        do_textbox_operation_with_undo([&]() {
            frontTextBox.textBox->process_text_input(*frontTextBox.cursor, inputText, shared_ptr_to_opt(frontTextBox.modMap));
        });
    }
}

void InputManager::Text::add_textbox_undo(const RichText::TextBox::Cursor& prevCursor, const RichText::TextData& prevRichText) {
    if(!textBoxes.empty()) {
        auto& frontTextBox = textBoxes.front();
        frontTextBox.textboxUndo.push({[textBox = frontTextBox.textBox, cursor = frontTextBox.cursor, prevCursor = prevCursor, prevRichText = prevRichText]() {
            textBox->set_rich_text_data(prevRichText);
            cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = std::max(prevCursor.selectionEndPos, prevCursor.selectionBeginPos);
            cursor->previousX = std::nullopt;
            return true;
        },
        [textBox = frontTextBox.textBox, cursor = frontTextBox.cursor, currentCursor = *frontTextBox.cursor, currentRichText = frontTextBox.textBox->get_rich_text_data()]() {
            textBox->set_rich_text_data(currentRichText);
            cursor->selectionBeginPos = cursor->selectionEndPos = cursor->pos = std::max(currentCursor.selectionEndPos, currentCursor.selectionBeginPos);
            cursor->previousX = std::nullopt;
            return true;
        }});
    }
}

void InputManager::Text::do_textbox_operation_with_undo(const std::function<void()>& func) {
    if(!textBoxes.empty()) {
        auto& frontTextBox = textBoxes.front();
        auto prevRichText = frontTextBox.textBox->get_rich_text_data();
        auto prevCursor = *frontTextBox.cursor;
        func();
        add_textbox_undo(prevCursor, prevRichText);
    }
}

bool InputManager::Text::is_accepting_input() {
    return !textBoxes.empty();
}

void InputManager::Text::update_accepting_input(SDL_Window* window) {
    if(!textBoxes.empty()) {
        auto& textbox = textBoxes.front();
        if(!propID.has_value())
            propID = SDL_CreateProperties();
        SDL_PropertiesID propIDVal = propID.value();
        // There is a bug where decimal places disappear when textinput type is set to number on android. We'll just set those textinputs back to text type
        SDL_SetNumberProperty(propIDVal, SDL_PROP_TEXTINPUT_TYPE_NUMBER, (textbox.textInputProperties.inputType == SDL_TEXTINPUT_TYPE_NUMBER) ? SDL_TEXTINPUT_TYPE_TEXT : textbox.textInputProperties.inputType);
        SDL_SetNumberProperty(propIDVal, SDL_PROP_TEXTINPUT_CAPITALIZATION_NUMBER, textbox.textInputProperties.capitalization);
        SDL_SetNumberProperty(propIDVal, SDL_PROP_TEXTINPUT_AUTOCORRECT_BOOLEAN, textbox.textInputProperties.autocorrect);
        SDL_SetNumberProperty(propIDVal, SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN, textbox.textInputProperties.multiline);
        SDL_SetNumberProperty(propIDVal, SDL_PROP_TEXTINPUT_ANDROID_INPUTTYPE_NUMBER, (textbox.textInputProperties.androidInputType & ANDROIDTEXT_TYPE_CLASS_NUMBER) ? ANDROIDTEXT_TYPE_CLASS_TEXT : textbox.textInputProperties.androidInputType);
        if(!textBoxes.front().rect)
            SDL_SetTextInputArea(window, nullptr, 0);
        else {
            SDL_Rect r = textBoxes.front().rect->get_sdl_rect();
            SDL_SetTextInputArea(window, &r, 0);
        }
        SDL_StartTextInputWithProperties(window, propIDVal);
        #ifdef __EMSCRIPTEN__
            isAcceptingInputEmscripten = 1;
        #endif
    }
    else {
        SDL_StopTextInput(window);
        SDL_SetTextInputArea(window, nullptr, 0);
        #ifdef __EMSCRIPTEN__
            isAcceptingInputEmscripten = 0;
        #endif
    }
}

std::string InputManager::get_clipboard_str_SDL() {
    char* data = SDL_GetClipboardText();
    std::string toRet(data);
    SDL_free(data);
    return toRet;
}

#ifdef _WIN32
// From clip library example, useful for debugging
void print_clip_image_spec(const clip::image_spec& spec) {
  std::cout << "Image in clipboard "
            << spec.width << "x" << spec.height
            << " (" << spec.bits_per_pixel << "bpp)\n"
            << "Format:" << "\n"
            << std::hex
            << "  Red   mask: " << spec.red_mask << "\n"
            << "  Green mask: " << spec.green_mask << "\n"
            << "  Blue  mask: " << spec.blue_mask << "\n"
            << "  Alpha mask: " << spec.alpha_mask << "\n"
            << std::dec
            << "  Red   shift: " << spec.red_shift << "\n"
            << "  Green shift: " << spec.green_shift << "\n"
            << "  Blue  shift: " << spec.blue_shift << "\n"
            << "  Alpha shift: " << spec.alpha_shift << "\n";
}
#endif

void InputManager::get_clipboard_image_data_SDL(const std::function<void(std::string_view data)>& callback) {
#ifndef __EMSCRIPTEN__
    static std::unordered_set<std::string> validMimetypes;
    if(validMimetypes.empty()) {
        validMimetypes.emplace("image/png");
        validMimetypes.emplace("image/gif");
        validMimetypes.emplace("image/jpeg");
        validMimetypes.emplace("image/svg+xml");
        validMimetypes.emplace("image/webp");
#ifdef _WIN32
        validMimetypes.emplace("image/bmp");
#endif
    }
    size_t mimeTypeSize;
    char** mimeTypes = SDL_GetClipboardMimeTypes(&mimeTypeSize);
    if(mimeTypes) {
        for(size_t i = 0; i < mimeTypeSize; i++) {
            if(validMimetypes.contains(mimeTypes[i])) {
                if(std::string(mimeTypes[i]) == "image/bmp") {
#ifdef _WIN32
                    std::vector<uint8_t> result;
                    try {
                        if(!clip::has(clip::image_format()))
                            throw std::runtime_error("Clipboard doesn't contain an image");
    
                        clip::image img;
                        if(!clip::get_image(img))
                            throw std::runtime_error("Error getting image from clipboard");
    
                        clip::image_spec spec = img.spec();
                        
                        SDL_PixelFormat pFormat = SDL_GetPixelFormatForMasks(spec.bits_per_pixel, spec.red_mask, spec.green_mask, spec.blue_mask, spec.alpha_mask);
                        if(pFormat != SDL_PIXELFORMAT_UNKNOWN) {
                            SDL_Surface* surfOriginal = SDL_CreateSurfaceFrom(spec.width, spec.height, pFormat, img.data(), spec.bytes_per_row);
                            if(surfOriginal) {
                                SDL_LockSurface(surfOriginal);
                                SDL_Surface* cSurf = SDL_ConvertSurface(surfOriginal, SDL_PIXELFORMAT_RGBA32);
                                SDL_UnlockSurface(surfOriginal);
                                SDL_DestroySurface(surfOriginal);
                                if(cSurf) {
                                    SDL_LockSurface(cSurf);
                                    SkDynamicMemoryWStream out;
                                    if(SkPngEncoder::Encode(&out, SkPixmap(SkImageInfo::Make(cSurf->w, cSurf->h, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType), cSurf->pixels, cSurf->pitch), {})) {
                                        out.flush();
                                        SDL_UnlockSurface(cSurf);
                                        SDL_DestroySurface(cSurf);
                                        result = out.detachAsVector();
                                    }
                                    else {
                                        SDL_UnlockSurface(cSurf);
                                        SDL_DestroySurface(cSurf);
                                        throw std::runtime_error("Could not encode image data to PNG");
                                    }
                                }
                                else
                                    throw std::runtime_error("Bitmap could not be converted to new format");
                            }
                            else
                                throw std::runtime_error("Bitmap could not be read into surface");
                        }
                        else
                            throw std::runtime_error("Bitmap format is unsupported");
                    }
                    catch(const std::runtime_error& e) {
                        Logger::get().log("INFO", std::string("[InputManager::get_clipboard_image_data_SDL] Error thrown: ") + e.what());
                    }
                    catch(...) {
                        Logger::get().log("INFO", "[InputManager::get_clipboard_image_data_SDL] Unknown error thrown");
                    }
                    if(!result.empty())
                        callback(std::string_view((char*)(result.data()), result.size())); // This part of the code should throw errors
#endif
                }
                else {
                    size_t clipboardDataSize;
                    void* clipboardData = SDL_GetClipboardData(mimeTypes[i], &clipboardDataSize);
                    callback(std::string_view(static_cast<char*>(clipboardData), clipboardDataSize));
                    SDL_free(clipboardData);
                }
                return;
            }
        }
    }
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
    k.held = true;
}

void InputManager::set_key_up(const SDL_KeyboardEvent& e, KeyCode kCode) {
    keys[kCode].held = false;
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

void InputManager::backend_drop_file_event(const SDL_DropEvent& e) {
    Vector2f mousePos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mousePos);

    main.input_drop_file_callback({
        .pos = mousePos,
        .source = e.source,
        .data = e.data
    });
}

void InputManager::backend_drop_text_event(const SDL_DropEvent& e) {
    Vector2f mousePos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mousePos);

    main.input_drop_text_callback({
        .pos = mousePos,
        .source = e.source,
        .data = e.data
    });
}

void InputManager::backend_mouse_button_up_update(const SDL_MouseButtonEvent& e) {
    Vector2f mousePos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mousePos);

    if(e.button == 1)
        mouse.leftDown = false;
    else if(e.button == 2)
        mouse.middleDown = false;
    else if(e.button == 3)
        mouse.rightDown = false;
    MouseButtonCallbackArgs args{
        .deviceType = MouseDeviceType::MOUSE,
        .button = static_cast<MouseButton>(e.button),
        .down = e.down,
        .clicks = e.clicks,
        .pos = mousePos
    };
    main.input_mouse_button_callback(args);

    isTouchDevice = false;
}

void InputManager::backend_mouse_button_down_update(const SDL_MouseButtonEvent& e) {
    Vector2f mousePos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mousePos);

    if(e.button == 1)
        mouse.leftDown = true;
    else if(e.button == 2)
        mouse.middleDown = true;
    else if(e.button == 3)
        mouse.rightDown = true;

    MouseButtonCallbackArgs args{
        .deviceType = MouseDeviceType::MOUSE,
        .button = static_cast<MouseButton>(e.button),
        .down = e.down,
        .clicks = e.clicks,
        .pos = mousePos
    };
    main.input_mouse_button_callback(args);

    isTouchDevice = false;
}

void InputManager::backend_mouse_motion_update(const SDL_MouseMotionEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mouseNewPos);

    Vector2f mouseRel = {e.xrel * main.window.density, e.yrel * main.window.density};
    main.input_mouse_motion_callback({
        .deviceType = MouseDeviceType::MOUSE,
        .pos = mouseNewPos,
        .move = mouseRel
    });

    isTouchDevice = false;
}

void InputManager::backend_mouse_wheel_update(const SDL_MouseWheelEvent& e) {
    Vector2f mouseNewPos = {e.mouse_x * main.window.density, e.mouse_y * main.window.density};
    mouse.set_pos(mouseNewPos);

    main.input_mouse_wheel_callback({
        .mousePos = mouseNewPos,
        .amount = {e.x, e.y},
        .tickAmount = {e.integer_x, e.integer_y}
    });

    isTouchDevice = false;
}

void InputManager::backend_pen_button_down_update(const SDL_PenButtonEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mouseNewPos);

    pen.previousPos = mouseNewPos;
    pen.buttons[e.button].held = true;
    pen.isEraser = e.pen_state & SDL_PEN_INPUT_ERASER_TIP;

    if(e.button == main.toolbar.tabletOptions.middleClickButton) {
        main.input_mouse_button_callback({
            .deviceType = MouseDeviceType::PEN,
            .button = MouseButton::MIDDLE,
            .down = e.down,
            .clicks = 1,
            .pos = mouseNewPos
        });
    }
    else if(e.button == main.toolbar.tabletOptions.rightClickButton) {
        main.input_mouse_button_callback({
            .deviceType = MouseDeviceType::PEN,
            .button = MouseButton::RIGHT,
            .down = e.down,
            .clicks = 1,
            .pos = mouseNewPos
        });
    }

    main.input_pen_button_callback({
        .button = e.button,
        .down = e.down,
        .pos = mouseNewPos
    });

    isTouchDevice = false;
}

void InputManager::backend_pen_button_up_update(const SDL_PenButtonEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mouseNewPos);

    pen.previousPos = mouseNewPos;
    pen.buttons[e.button].held = false;
    pen.isEraser = e.pen_state & SDL_PEN_INPUT_ERASER_TIP;

    if(e.button == main.toolbar.tabletOptions.middleClickButton) {
        main.input_mouse_button_callback({
            .deviceType = MouseDeviceType::PEN,
            .button = MouseButton::MIDDLE,
            .down = e.down,
            .clicks = 1,
            .pos = mouseNewPos
        });
    }
    else if(e.button == main.toolbar.tabletOptions.rightClickButton) {
        main.input_mouse_button_callback({
            .deviceType = MouseDeviceType::PEN,
            .button = MouseButton::RIGHT,
            .down = e.down,
            .clicks = 1,
            .pos = mouseNewPos
        });
    }

    main.input_pen_button_callback({
        .button = e.button,
        .down = e.down,
        .pos = mouseNewPos
    });

    isTouchDevice = false;
}

void InputManager::backend_pen_touch_down_update(const SDL_PenTouchEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mouseNewPos);

    pen.previousPos = mouseNewPos;
    pen.isDown = true;
    mouse.leftDown = true;
    if((std::chrono::steady_clock::now() - pen.lastPenLeftClickTime) > std::chrono::milliseconds(300))
        pen.leftClicksSaved = 0;
    pen.leftClicksSaved++;
    pen.lastPenLeftClickTime = std::chrono::steady_clock::now();
    pen.isEraser = e.eraser;

    main.input_mouse_button_callback({
        .deviceType = MouseDeviceType::PEN,
        .button = MouseButton::LEFT,
        .down = e.down,
        .clicks = pen.leftClicksSaved,
        .pos = mouseNewPos
    });
    main.input_pen_touch_callback({
        .down = e.down,
        .eraser = e.eraser,
        .pos = mouseNewPos
    });

    isTouchDevice = false;
}

void InputManager::backend_pen_touch_up_update(const SDL_PenTouchEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mouseNewPos);

    pen.previousPos = mouseNewPos;
    pen.isDown = false;
    mouse.leftDown = false;
    pen.isEraser = e.eraser;
    pen.pressure = 0.0;

    main.input_mouse_button_callback({
        .deviceType = MouseDeviceType::PEN,
        .button = MouseButton::LEFT,
        .down = e.down,
        .clicks = 0,
        .pos = mouseNewPos
    });
    main.input_pen_touch_callback({
        .down = e.down,
        .eraser = e.eraser,
        .pos = mouseNewPos
    });

    isTouchDevice = false;
}

void InputManager::backend_pen_motion_update(const SDL_PenMotionEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mouseNewPos);

    Vector2f mouseRel = mouseNewPos - pen.previousPos;
    pen.previousPos = mouseNewPos;
    pen.isEraser = e.pen_state & SDL_PEN_INPUT_ERASER_TIP;

    main.input_mouse_motion_callback({
        .deviceType = MouseDeviceType::PEN,
        .pos = mouseNewPos,
        .move = mouseRel
    });
    main.input_pen_motion_callback({
        .pos = mouseNewPos,
        .move = mouseRel
    });

    isTouchDevice = false;
}

void InputManager::backend_pen_axis_update(const SDL_PenAxisEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.density, e.y * main.window.density};
    mouse.set_pos(mouseNewPos);

    pen.previousPos = mouseNewPos;
    pen.isEraser = e.pen_state & SDL_PEN_INPUT_ERASER_TIP;

    // For now, only pass pressure axis
    if(e.axis == SDL_PEN_AXIS_PRESSURE) {
        pen.pressure = e.value;
        main.input_pen_axis_callback({
            .pos = mouseNewPos,
            .axis = e.axis,
            .value = e.value
        });
    }
}

std::vector<Vector2f> InputManager::get_multiple_finger_positions() {
    std::vector<Vector2f> pos;
    for(const SDL_TouchFingerEvent& fingerEvent : touch.fingers)
        pos.emplace_back(fingerEvent.x * main.window.size.x() * main.window.density, fingerEvent.y * main.window.size.y() * main.window.density);
    return pos;
}

std::vector<Vector2f> InputManager::get_multiple_finger_motions() {
    std::vector<Vector2f> pos;
    for(const SDL_TouchFingerEvent& fingerEvent : touch.fingers)
        pos.emplace_back(fingerEvent.dx * main.window.size.x() * main.window.density, fingerEvent.dy * main.window.size.y() * main.window.density);
    return pos;
}

void InputManager::touch_finger_do_mouse_down() {
    auto& prevTouch = touch.fingers[0];
    Vector2f mouseNewPos = {prevTouch.x * main.window.size.x() * main.window.density, prevTouch.y * main.window.size.y() * main.window.density};
    mouse.set_pos(mouseNewPos);
    mouse.leftDown = true;

    if((std::chrono::steady_clock::now() - touch.lastLeftClickTime) > std::chrono::milliseconds(500))
        touch.leftClicksSaved = 0;
    touch.leftClicksSaved++;
    touch.lastLeftClickTime = std::chrono::steady_clock::now();

    main.input_mouse_button_callback({
        .deviceType = MouseDeviceType::TOUCH,
        .button = MouseButton::LEFT,
        .down = true,
        .clicks = touch.leftClicksSaved,
        .pos = mouseNewPos
    });
}

void InputManager::touch_finger_do_mouse_up(const SDL_TouchFingerEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.size.x() * main.window.density, e.y * main.window.size.y() * main.window.density};
    mouse.set_pos(mouseNewPos);
    mouse.leftDown = false;

    main.input_mouse_button_callback({
        .deviceType = MouseDeviceType::TOUCH,
        .button = MouseButton::LEFT,
        .down = false,
        .clicks = 0,
        .pos = mouseNewPos
    });
}

void InputManager::touch_finger_do_mouse_motion(const SDL_TouchFingerEvent& e) {
    Vector2f mouseNewPos = {e.x * main.window.size.x() * main.window.density, e.y * main.window.size.y() * main.window.density};
    mouse.set_pos(mouseNewPos);
    Vector2f mouseRel = {e.dx * main.window.size.x() * main.window.density, e.dy * main.window.size.y() * main.window.density};
    main.input_mouse_motion_callback({
        .deviceType = MouseDeviceType::TOUCH,
        .pos = mouseNewPos,
        .move = mouseRel
    });
}

void InputManager::backend_touch_finger_down_update(const SDL_TouchFingerEvent& e) {
    touch.fingers.emplace_back(e);
    auto fingerPosVec = get_multiple_finger_positions();
    switch(touch.touchEventType) {
        case Touch::NO_TOUCH_EVENT: {
            if(touch.fingers.size() == 2) {
                touch.touchEventType = Touch::TWO_FINGER_EVENT;
                main.input_multi_finger_touch_callback({
                    .down = true,
                    .pos = fingerPosVec
                });
            }
            break;
        }
        case Touch::ONE_FINGER_EVENT: {
            if(touch.fingers.size() == 2) {
                touch_finger_do_mouse_up(e);
                touch.touchEventType = Touch::TWO_FINGER_EVENT;
                main.input_multi_finger_touch_callback({
                    .down = true,
                    .pos = fingerPosVec
                });
            }
            break;
        }
        case Touch::TWO_FINGER_EVENT: {
            break;
        }
        case Touch::EVENT_DONE: {
            if(touch.fingers.size() == 1)
                touch.touchEventType = Touch::NO_TOUCH_EVENT;
            break;
        }
    }


    if((std::chrono::steady_clock::now() - touch.lastFingerTapTime) > std::chrono::milliseconds(500))
        touch.fingerTapsSaved = 0;
    touch.fingerTapsSaved++;
    touch.lastFingerTapTime = std::chrono::steady_clock::now();

    main.input_finger_touch_callback({
        .fingerID = e.fingerID,
        .down = true,
        .pos = fingerPosVec.back(),
        .fingerDownCount = touch.fingers.size(),
        .fingerTapCount = touch.fingerTapsSaved
    });

    isTouchDevice = true;
}

void InputManager::backend_touch_finger_up_update(const SDL_TouchFingerEvent& e) {
    switch(touch.touchEventType) {
        case Touch::NO_TOUCH_EVENT: {
            touch_finger_do_mouse_down();
            touch_finger_do_mouse_up(e);
            touch.touchEventType = Touch::EVENT_DONE;
            break;
        }
        case Touch::ONE_FINGER_EVENT: {
            touch_finger_do_mouse_up(e);
            touch.touchEventType = Touch::EVENT_DONE;
            break;
        }
        case Touch::TWO_FINGER_EVENT: {
            main.input_multi_finger_touch_callback({
                .down = false,
                .pos = get_multiple_finger_positions()
            });
            touch.touchEventType = Touch::EVENT_DONE;
            break;
        }
        case Touch::EVENT_DONE: {
            break;
        }
    }

    main.input_finger_touch_callback({
        .fingerID = e.fingerID,
        .down = false,
        .pos = {e.x * main.window.size.x() * main.window.density, e.y * main.window.size.y() * main.window.density},
        .fingerDownCount = touch.fingers.size(),
        .fingerTapCount = 0
    });

    std::erase_if(touch.fingers, [&e](auto& tE) {
        return e.fingerID == tE.fingerID;
    });

    isTouchDevice = true;
}

void InputManager::backend_touch_finger_motion_update(const SDL_TouchFingerEvent& e) {
    switch(touch.touchEventType) {
        case Touch::NO_TOUCH_EVENT: {
            touch_finger_do_mouse_down();
            touch_finger_do_mouse_motion(e);
            touch.touchEventType = Touch::ONE_FINGER_EVENT;
            break;
        }
        case Touch::ONE_FINGER_EVENT: {
            touch_finger_do_mouse_motion(e);
            break;
        }
        case Touch::TWO_FINGER_EVENT: {
            main.input_multi_finger_motion_callback({
                .pos = get_multiple_finger_positions(),
                .move = get_multiple_finger_motions()
            });
            break;
        }
        case Touch::EVENT_DONE: {
            break;
        }
    }
    for(auto& tE : touch.fingers) {
        if(tE.fingerID == e.fingerID) {
            tE = e;
            break;
        }
    }

    main.input_finger_motion_callback({
        .fingerID = e.fingerID,
        .pos = {e.x * main.window.size.x() * main.window.density, e.y * main.window.size.y() * main.window.density},
        .move = {e.dx * main.window.size.x() * main.window.density, e.dy * main.window.size.y() * main.window.density},
        .fingerDownCount = touch.fingers.size()
    });

    isTouchDevice = true;
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

    std::shared_ptr<RichText::TextBox> textBox = text.textBoxes.empty() ? nullptr : text.textBoxes.front().textBox;
    std::shared_ptr<RichText::TextBox::Cursor> cursor = text.textBoxes.empty() ? nullptr : text.textBoxes.front().cursor;
    std::optional<RichText::TextStyleModifier::ModifierMap> modMap = text.textBoxes.empty() ? std::nullopt : shared_ptr_to_opt(text.textBoxes.front().modMap);

    switch(kPress) {
        case SDLK_UP:
            set_key_down(e, KEY_TEXT_UP);
            if(textBox)
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::UP, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
            break;
        case SDLK_DOWN:
            set_key_down(e, KEY_TEXT_DOWN);
            if(textBox)
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::DOWN, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
            break;
        case SDLK_LEFT:
            set_key_down(e, KEY_TEXT_LEFT);
            if(textBox)
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::LEFT, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
            break;
        case SDLK_RIGHT:
            set_key_down(e, KEY_TEXT_RIGHT);
            if(textBox)
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::RIGHT, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
            break;
        case SDLK_BACKSPACE:
            set_key_down(e, KEY_TEXT_BACKSPACE);
            if(textBox)
                text.do_textbox_operation_with_undo([&]() {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::BACKSPACE, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
                });
            break;
        case SDLK_DELETE:
            set_key_down(e, KEY_TEXT_DELETE);
            if(textBox)
                text.do_textbox_operation_with_undo([&]() {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::DEL, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
                });
            break;
        case SDLK_HOME:
            set_key_down(e, KEY_TEXT_HOME);
            if(textBox)
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::HOME, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
            break;
        case SDLK_END:
            set_key_down(e, KEY_TEXT_END);
            if(textBox)
                textBox->process_key_input(*cursor, RichText::TextBox::InputKey::END, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
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
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL)) {
                set_key_down(e, KEY_TEXT_COPY);
                if(textBox)
                    set_clipboard_plain_and_richtext_pair(textBox->process_copy(*cursor));
            }
            break;
        case SDLK_X:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL)) {
                set_key_down(e, KEY_TEXT_CUT);
                if(textBox)
                    text.do_textbox_operation_with_undo([&]() {
                        set_clipboard_plain_and_richtext_pair(textBox->process_cut(*cursor));
                    });
            }
            break;
        case SDLK_V:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL)) {
                set_key_down(e, KEY_TEXT_PASTE);
                if(textBox)
                    text.do_textbox_operation_with_undo([&]() {
                        call_text_paste(true);
                    });
            }
            break;
        case SDLK_A:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL)) {
                set_key_down(e, KEY_TEXT_SELECTALL);
                if(textBox)
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::SELECT_ALL, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
            }
            break;
        case SDLK_Z:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL)) {
                set_key_down(e, KEY_TEXT_UNDO);
                if(textBox)
                    text.textBoxes.front().textboxUndo.undo();
            }
            break;
        case SDLK_R:
            if((kMod & SDL_KMOD_GUI) || (kMod & SDL_KMOD_CTRL)) {
                set_key_down(e, KEY_TEXT_REDO);
                if(textBox)
                    text.textBoxes.front().textboxUndo.redo();
            }
            break;
        case SDLK_RETURN: {
            set_key_down(e, KEY_TEXT_ENTER);
            if(textBox)
                text.do_textbox_operation_with_undo([&]() {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::ENTER, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
                });
            main.input_key_callback(KeyCallbackArgs{.key = KEY_TEXT_ENTER, .down = e.down, .repeat = e.repeat});
            break;
        }
        case SDLK_TAB: {
            set_key_down(e, KEY_TEXT_TAB);
            if(textBox)
                text.do_textbox_operation_with_undo([&]() {
                    textBox->process_key_input(*cursor, RichText::TextBox::InputKey::TAB, ctrl_or_meta_held(), key(KEY_GENERIC_LSHIFT).held, modMap);
                });
            break;
        }
        case SDLK_ESCAPE:
            set_key_down(e, KEY_TEXT_ESCAPE);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_TEXT_ESCAPE, .down = e.down, .repeat = e.repeat});
            break;
        default:
            break;
    }

    if(text.is_accepting_input())
        return;

    auto f = keyAssignments.find({make_generic_key_mod(kMod), kPress});
    if(f != keyAssignments.end()) {
        set_key_down(e, f->second);
        main.input_key_callback(KeyCallbackArgs{.key = f->second, .down = e.down, .repeat = e.repeat});
    }
    switch(kPress) {
        case SDLK_UP:
            set_key_down(e, KEY_GENERIC_UP);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_UP, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_DOWN:
            set_key_down(e, KEY_GENERIC_DOWN);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_DOWN, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_LEFT:
            set_key_down(e, KEY_GENERIC_LEFT);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_LEFT, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_RIGHT:
            set_key_down(e, KEY_GENERIC_RIGHT);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_RIGHT, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_RETURN:
            set_key_down(e, KEY_GENERIC_ENTER);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_ENTER, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_ESCAPE:
            set_key_down(e, KEY_GENERIC_ESCAPE);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_ESCAPE, .down = e.down, .repeat = e.repeat});
            break;
    }
}

void InputManager::call_text_paste(bool isRichTextPaste) {
    text.isNextPasteRich = isRichTextPaste;
#ifdef __EMSCRIPTEN__
    emscripten_browser_clipboard::paste_async([](std::string&& pasteData, void* callbackData){
        InputManager* inMan = (InputManager*)callbackData;
        std::string pData = pasteData;
        inMan->process_text_paste(pData);
    }, this);
#else
    process_text_paste(get_clipboard_str_SDL());
#endif
}

void InputManager::process_text_paste(const std::string& plainClipboardStr) {
    if(!text.textBoxes.empty()) {
        // Workaround for not being able to copy richtext to system clipboard, this should at least work within the application itself
        if(text.textBoxes.front().isRichTextBox && text.isNextPasteRich && lastCopiedRichText.has_value()) {
            if(lastCopiedRichText.value().get_plain_text() == remove_carriage_returns_from_str(plainClipboardStr))
                text.textBoxes.front().textBox->process_rich_text_input(*text.textBoxes.front().cursor, lastCopiedRichText.value());
            else
                text.textBoxes.front().textBox->process_text_input(*text.textBoxes.front().cursor, plainClipboardStr, shared_ptr_to_opt(text.textBoxes.front().modMap));
        }
        else
            text.textBoxes.front().textBox->process_text_input(*text.textBoxes.front().cursor, plainClipboardStr, shared_ptr_to_opt(text.textBoxes.front().modMap));
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
            main.input_key_callback(KeyCallbackArgs{.key = KEY_TEXT_ENTER, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_TAB:
            set_key_up(e, KEY_TEXT_TAB);
            break;
        case SDLK_ESCAPE:
            set_key_up(e, KEY_TEXT_ESCAPE);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_TEXT_ESCAPE, .down = e.down, .repeat = e.repeat});
            break;
        default:
            break;
    }

    for(auto& p : keyAssignments) {
        if(p.first.y() == kPress) { // Dont try matching modifiers when setting key to go up
            set_key_up(e, p.second);
            main.input_key_callback(KeyCallbackArgs{.key = p.second, .down = e.down, .repeat = e.repeat});
        }
    }

    switch(kPress) {
        case SDLK_UP:
            set_key_up(e, KEY_GENERIC_UP);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_UP, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_DOWN:
            set_key_up(e, KEY_GENERIC_DOWN);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_DOWN, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_LEFT:
            set_key_up(e, KEY_GENERIC_LEFT);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_LEFT, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_RIGHT:
            set_key_up(e, KEY_GENERIC_RIGHT);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_RIGHT, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_RETURN:
            set_key_up(e, KEY_GENERIC_ENTER);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_ENTER, .down = e.down, .repeat = e.repeat});
            break;
        case SDLK_ESCAPE:
            set_key_up(e, KEY_GENERIC_ESCAPE);
            main.input_key_callback(KeyCallbackArgs{.key = KEY_GENERIC_ESCAPE, .down = e.down, .repeat = e.repeat});
            break;
    }
}

void InputManager::stop_key_input() {
    stopKeyInput = 2;
}

void InputManager::Mouse::set_pos(const Vector2f& newPos) {
    pos = newPos;
}

void InputManager::update() {
    if(touch.touchEventType == Touch::NO_TOUCH_EVENT && (SDL_GetTicksNS() - touch.fingers[0].timestamp) > (200 * 1000000)) { // 200ms to determine whether touch is with a single finger
        touch_finger_do_mouse_down();
        touch.touchEventType = Touch::ONE_FINGER_EVENT;
    }
    if(!text.textBoxes.empty()) {
        //auto& textbox = text.textBoxes.front();
        //if(textbox.rect) {
        //    SDL_Rect r = textbox.rect->get_sdl_rect();
        //    //SDL_SetTextInputArea(main.window.sdlWindow, &r, textbox.cursor->pos.fTextByteIndex);
        //}
        //else
        //    //SDL_SetTextInputArea(main.window.sdlWindow, nullptr, textbox.cursor->pos.fTextByteIndex);
    }
}

void InputManager::frame_reset(const Vector2i& windowSize) {
    cursorIcon = SystemCursorType::DEFAULT;

    lastPressedKeybind = std::nullopt;
    if(stopKeyInput)
        stopKeyInput--;

    toggleFullscreen = false;
    hideCursor = false;
}
