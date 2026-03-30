#pragma once
#include "../GUIManager.hpp"
#include "../Elements/TextBox.hpp"
#include "TextLabelHelpers.hpp"
#include "LayoutHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

template <typename T, typename OptT> TextBoxData<T> textbox_options_to_data(const OptT& options) {
    TextBoxData<T> toRet;
    toRet.updateEveryEdit = options.updateEveryEdit;
    toRet.onEnter = options.onEnter;
    toRet.onEdit = options.onEdit;
    toRet.immutable = options.immutable;
    return toRet;
}

struct TextBoxOptions {
    bool updateEveryEdit = false;
    bool immutable = false;
    std::function<void()> onEnter;
    std::function<void()> onEdit;
};

struct TextBoxScalarOptions {
    int decimalPrecision = 0;
    bool updateEveryEdit = false;
    bool immutable = false;
    std::function<void()> onEnter;
    std::function<void()> onEdit;
};

struct TextBoxPathOptions {
    std::filesystem::file_type fileTypeRestriction;
    bool updateEveryEdit = false;
    bool immutable = false;
    std::function<void()> onEnter;
    std::function<void()> onEdit;
};

struct TextBoxHexColorOptions {
    bool hasAlpha = true;
    bool updateEveryEdit = false;
    bool immutable = false;
    std::function<void()> onEnter;
    std::function<void()> onEdit;
};

void input_color_component_255(GUIManager& gui, const char* id, float* val, const TextBoxOptions& options = {});
void input_text(GUIManager& gui, const char* id, std::string* val, const TextBoxOptions& options = {});
void input_text_field(GUIManager& gui, const char* id, std::string_view name, std::string* val, const TextBoxOptions& options = {});
void input_scalar(GUIManager& gui, const char* id, uint8_t* val, uint8_t minVal, uint8_t maxVal, const TextBoxScalarOptions& options = {});
void input_path(GUIManager& gui, const char* id, std::filesystem::path* val, const TextBoxPathOptions& options = {});
void input_path_field(GUIManager& gui, const char* id, std::string_view name, std::filesystem::path* val, const TextBoxPathOptions& options = {});

template <typename T> void input_scalar(GUIManager& gui, const char* id, T* val, T minVal, T maxVal, const TextBoxScalarOptions& options = {}) {
    InputManager::TextInputProperties textInputProps {
        .inputType = SDL_TextInputType::SDL_TEXTINPUT_TYPE_NUMBER,
        .capitalization = SDL_Capitalization::SDL_CAPITALIZE_NONE,
        .autocorrect = false,
        .multiline = false,
        .androidInputType = InputManager::AndroidInputType::ANDROIDTEXT_TYPE_CLASS_NUMBER
    };
    if(std::is_signed<T>())
        textInputProps.androidInputType |= InputManager::AndroidInputType::ANDROIDTEXT_TYPE_NUMBER_FLAG_SIGNED;
    if(std::is_floating_point<T>())
        textInputProps.androidInputType |= InputManager::AndroidInputType::ANDROIDTEXT_TYPE_NUMBER_FLAG_DECIMAL;

    TextBoxData<T> d = textbox_options_to_data<T>(options);
    d.data = val;
    d.textInputProps = textInputProps;
    d.fromStr = [minVal, maxVal](const std::string& a) {
        if(a.empty())
            return minVal;
        T toRet;
        std::stringstream ss;
        ss << a;
        ss >> toRet;
        if(ss.fail())
            return minVal;
        return std::clamp(toRet, minVal, maxVal);
    };
    d.toStr = [decimalPrecision = options.decimalPrecision, minVal, maxVal](const T& a) {
        std::stringstream ss;
        if(std::is_floating_point<T>()) {
            ss.precision(decimalPrecision);
            ss << std::fixed << a;
        }
        else
            ss << a;
        return ss.str();
    };

    gui.element<TextBox<T>>(id, d);
}

template <typename TContainer, typename T> void input_scalars_field(GUIManager& gui, const char* id, std::string_view name, TContainer* val, size_t elemCount, T minVal, T maxVal, const TextBoxScalarOptions& options = {}) {
    gui.new_id(id, [&] {
        left_to_right_line_layout(gui, [&]() {
            text_label(gui, name);
            for(size_t i = 0; i < elemCount; i++)
                gui.new_id(i, [&] { input_scalar<T>(gui, "field", &(*val)[i], minVal, maxVal, options); });
        });
    });
}

template <typename T> void input_scalar_field(GUIManager& gui, const char* id, std::string_view name, T* val, T minVal, T maxVal, const TextBoxScalarOptions& options = {}) {
    left_to_right_line_layout(gui, [&]() {
        text_label(gui, name);
        input_scalar<T>(gui, id, val, minVal, maxVal, options);
    });
}

template <typename T> void input_color_hex(GUIManager& gui, const char* id, T* val, const TextBoxHexColorOptions& options = {}) {
    InputManager::TextInputProperties textInputProps {
        .inputType = SDL_TextInputType::SDL_TEXTINPUT_TYPE_TEXT,
        .capitalization = SDL_Capitalization::SDL_CAPITALIZE_NONE,
        .autocorrect = false,
        .multiline = false,
        .androidInputType = InputManager::AndroidInputType::ANDROIDTEXT_TYPE_CLASS_TEXT
    };

    TextBoxData<T> d = textbox_options_to_data<T>(options);
    d.data = val;
    d.textInputProps = textInputProps;
    d.fromStr = [hasAlpha = options.hasAlpha](const std::string& a) {
        T def;
        def[0] = 0.0f;
        def[1] = 0.0f;
        def[2] = 0.0f;
        if(hasAlpha)
            def[3] = 1.0f;
        unsigned startIndex = 0;
        if(a.empty())
            return def;

        if(a[0] == '#')
            startIndex++;

        for(unsigned i = 0; i < (hasAlpha ? 4 : 3); i++) {
            int byteColor;
            std::stringstream ss;
            size_t startingAt = startIndex + i * 2;
            if(startingAt + 1 >= a.size())
                break;
            ss << a.substr(startingAt, 2);
            ss >> std::hex >> byteColor;
            if(ss.fail())
                break;
            def[i] = byteColor / 255.0f;
        }
        return def;
    };
    d.toStr = [hasAlpha = options.hasAlpha](const T& a) {
        std::stringstream ss;
        ss << "#" << std::setfill('0') << std::setw(2) << std::hex;
        for(size_t i = 0; i < (hasAlpha ? 4 : 3); i++) {
            int convertTo255 = static_cast<int>(std::clamp<float>(a[i], 0, 1) * 255);
            ss << std::setfill('0') << std::setw(2) << std::hex << convertTo255;
        }
        return ss.str();
    };

    gui.element<TextBox<T>>(id, d);
}

} }
