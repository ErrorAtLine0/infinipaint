#include "TextBoxHelpers.hpp"
#include "LayoutHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

void input_color_component_255(GUIManager& gui, const char* id, float* val, const TextBoxOptions& options) {
    InputManager::TextInputProperties textInputProps {
        .inputType = SDL_TextInputType::SDL_TEXTINPUT_TYPE_NUMBER,
        .capitalization = SDL_Capitalization::SDL_CAPITALIZE_NONE,
        .autocorrect = false,
        .multiline = false,
        .androidInputType = InputManager::AndroidInputType::ANDROIDTEXT_TYPE_CLASS_NUMBER
    };

    TextBoxData<float> d = textbox_options_to_data<float>(options);
    d.data = val;
    d.textInputProps = textInputProps;
    d.fromStr = [](const std::string& str) {
        int roundTo255;
        std::stringstream ss;
        ss << str;
        ss >> roundTo255;
        return std::clamp<float>(static_cast<float>(roundTo255) / 255.0f, 0.0f, 1.0f);
    };
    d.toStr = [](const float& a) {
        int convertTo255 = a * 255;
        std::stringstream ss;
        ss << convertTo255;
        return ss.str();
    };

    gui.element<TextBox<float>>(id, d);
}

void input_text(GUIManager& gui, const char* id, std::string* val, const TextBoxOptions& options) {
    InputManager::TextInputProperties textInputProps {
        .inputType = SDL_TextInputType::SDL_TEXTINPUT_TYPE_TEXT,
        .capitalization = SDL_Capitalization::SDL_CAPITALIZE_NONE,
        .autocorrect = false,
        .multiline = false,
        .androidInputType = InputManager::AndroidInputType::ANDROIDTEXT_TYPE_CLASS_TEXT
    };

    TextBoxData<std::string> d = textbox_options_to_data<std::string>(options);
    d.data = val;
    d.textInputProps = textInputProps;
    d.fromStr = [](const std::string& a) {
        return a;
    };
    d.toStr = [](const std::string& a) {
        return a;
    };

    gui.element<TextBox<std::string>>(id, d);
}

void input_text_field(GUIManager& gui, const char* id, std::string_view name, std::string* val, const TextBoxOptions& options) {
    left_to_right_line_layout(gui, [&]() {
        text_label(gui, name);
        input_text(gui, id, val, options);
    });
}

template <> void input_scalar<uint8_t>(GUIManager& gui, const char* id, uint8_t* val, uint8_t minVal, uint8_t maxVal, const TextBoxScalarOptions& options) {
    InputManager::TextInputProperties textInputProps {
        .inputType = SDL_TextInputType::SDL_TEXTINPUT_TYPE_NUMBER,
        .capitalization = SDL_Capitalization::SDL_CAPITALIZE_NONE,
        .autocorrect = false,
        .multiline = false,
        .androidInputType = InputManager::AndroidInputType::ANDROIDTEXT_TYPE_CLASS_NUMBER
    };

    TextBoxData<uint8_t> d = textbox_options_to_data<uint8_t>(options);
    d.data = val;
    d.textInputProps = textInputProps;
    d.fromStr = [minVal, maxVal](const std::string& a) {
        if(a.empty())
            return minVal;
        uint32_t toRet;
        std::stringstream ss;
        ss << a;
        ss >> toRet;
        if(ss.fail())
            return minVal;
        return static_cast<uint8_t>(std::clamp(toRet, static_cast<uint32_t>(minVal), static_cast<uint32_t>(maxVal)));
    };
    d.toStr = [options](const uint8_t& a) {
        return std::to_string(static_cast<uint32_t>(a));
    };

    gui.element<TextBox<uint8_t>>(id, d);
}

void input_path(GUIManager& gui, const char* id, std::filesystem::path* val, const TextBoxPathOptions& options) {
    InputManager::TextInputProperties textInputProps {
        .inputType = SDL_TextInputType::SDL_TEXTINPUT_TYPE_TEXT,
        .capitalization = SDL_Capitalization::SDL_CAPITALIZE_NONE,
        .autocorrect = false,
        .multiline = false,
        .androidInputType = InputManager::AndroidInputType::ANDROIDTEXT_TYPE_CLASS_TEXT
    };

    TextBoxData<std::filesystem::path> d = textbox_options_to_data<std::filesystem::path>(options);
    d.data = val;
    d.textInputProps = textInputProps;
    d.fromStr = [fileTypeRestriction = options.fileTypeRestriction](const std::string& a) {
        std::filesystem::path toRet = std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(a.c_str()), a.length()));
        if(fileTypeRestriction != std::filesystem::file_type::none && std::filesystem::status(toRet).type() != fileTypeRestriction)
            return std::optional<std::filesystem::path>(std::nullopt);
        return std::optional<std::filesystem::path>(toRet);
    };
    d.toStr = [](const std::filesystem::path& a) {
        return a.string();
    };

    gui.element<TextBox<std::filesystem::path>>(id, d);
}

void input_path_field(GUIManager& gui, const char* id, std::string_view name, std::filesystem::path* val, const TextBoxPathOptions& options) {
    left_to_right_line_layout(gui, [&]() {
        text_label(gui, name);
        input_path(gui, id, val, options);
    });
}

} }
