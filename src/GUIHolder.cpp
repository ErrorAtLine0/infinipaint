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

#include "GUIHolder.hpp"
#include <filesystem>
#include <fstream>
#include "MainProgram.hpp"

#include <include/core/SkStream.h>

GUIHolder::GUIHolder(MainProgram& m):
    main(m)
{
    gui.io.textTypeface = main.fonts->map["Roboto"];
    gui.io.fonts = main.fonts;
    gui.io.input = &main.input;

    // NOTE: On windows, when the native file picker is open, any call to MakeFromFile fails
    // So, it's better to load the icons at the beginning of the program so that the icon loading doesn't fail later
    load_icons_at("data/icons");

    load_default_theme();
}

void GUIHolder::load_icons_at(const std::filesystem::path& pathToLoad) {
    int globCount;
    char** filesInPath = SDL_GlobDirectory(pathToLoad.string().c_str(), nullptr, 0, &globCount);
    if(filesInPath) {
        for(int i = 0; i < globCount; i++) {
            std::filesystem::path filePath = pathToLoad / std::filesystem::path(filesInPath[i]);
            SDL_PathInfo fileInfo;
            if(SDL_GetPathInfo(filePath.string().c_str(), &fileInfo) && fileInfo.type == SDL_PATHTYPE_FILE) {
                std::string iconRelativePath = filePath.relative_path().string();
                std::replace(iconRelativePath.begin(), iconRelativePath.end(), '\\', '/');
                std::string iconData = read_file_to_string(iconRelativePath);
                auto stream = SkMemoryStream(iconData.c_str(), iconData.size(), false);
                auto svgDom = SkSVGDOM::Builder().make(stream);
                if(!svgDom)
                    throw std::runtime_error("[Toolbar::Toolbar] Could not parse SVG " + iconRelativePath);
                else {
                    if(svgDom->containerSize().width() == 0 || svgDom->containerSize().height() == 0)
                        svgDom->setContainerSize({1000, 1000});
                    gui.io.svgData[iconRelativePath] = svgDom;
                }
            }
        }
        SDL_free(filesInPath);
    }
}

void GUIHolder::load_default_theme() {
    gui.io.theme = GUIStuff::get_default_dark_mode();
}

void GUIHolder::save_theme(const std::filesystem::path& configPath, const std::string& themeName) {
    std::filesystem::create_directory(configPath / "themes");
    std::ofstream f(configPath / "themes" / (themeName + ".json"));
    if(f.is_open()) {
        using json = nlohmann::json;
        json j;
        j = *gui.io.theme;
        f << j;
        f.close();
    }
}

bool GUIHolder::load_theme(const std::filesystem::path& configPath, const std::string& themeName) {
    std::filesystem::path themeDir = configPath / "themes";
    bool successfullyLoaded = false;
    if(std::filesystem::exists(themeDir) && std::filesystem::is_directory(themeDir)) {
        std::ifstream f(themeDir / (themeName + ".json"));
        if(f.is_open()) {
            using json = nlohmann::json;
            try {
                json j;
                f >> j;
                auto theme(std::make_shared<GUIStuff::Theme>());
                j.get_to(*theme);
                gui.io.theme = theme;
                successfullyLoaded = true;
            } catch(...) {}
            f.close();
        }
    }
    if(!successfullyLoaded)
        load_default_theme();
    return successfullyLoaded;
}

void GUIHolder::update() {
    gui.io.deltaTime = main.deltaTime;
    gui.update();
}

void GUIHolder::window_update() {
    calculate_final_gui_scale();
    gui.update_window(main.window.size.cast<float>(), main.window.safeArea, final_gui_scale());
}

float GUIHolder::final_gui_scale() {
    return finalCalculatedGuiScale;
}

void GUIHolder::calculate_final_gui_scale() {
    finalCalculatedGuiScale = final_gui_scale_not_fit();
    //Vector2f maxWindowSizeBeforeForcedFit = final_gui_scale_not_fit() * Vector2f{700.0f, 700.0f};
    //Vector2f fitRatio = {main.window.size.x() / maxWindowSizeBeforeForcedFit.x(), main.window.size.y() / maxWindowSizeBeforeForcedFit.y()};
    //finalCalculatedGuiScale = final_gui_scale_not_fit() * std::min(std::min(fitRatio.x(), fitRatio.y()), 1.0f);
}

float GUIHolder::final_gui_scale_not_fit() {
    return main.conf.guiScale * main.get_scale_and_density_factor_gui();
}

void GUIHolder::delete_cache_surface() {
    gui.io.surface = nullptr;
}

void GUIHolder::draw(SkCanvas* canvas, bool skiaAA) {
    if(!gui.io.surface) {
        gui.io.surface = main.create_native_surface(main.window.size, true);
        gui.io.redrawSurface = true;
    }

    gui.draw(canvas, skiaAA);
}

void GUIHolder::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    gui.input_paste_callback(paste);
}

void GUIHolder::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {
    gui.input_android_text_box_input_callback(textboxInput);
}

void GUIHolder::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    gui.input_text_key_callback(key);
}

void GUIHolder::input_text_callback(const InputManager::TextCallbackArgs& text) {
    gui.input_text_callback(text);
}

void GUIHolder::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    gui.input_key_callback(key);
}

void GUIHolder::input_text_key_callback() {
}

void GUIHolder::input_text_input_callback() {
}

void GUIHolder::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    gui.input_mouse_button_callback(button);
}

void GUIHolder::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    gui.input_mouse_motion_callback(motion);
}

void GUIHolder::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    gui.input_mouse_wheel_callback(wheel);
}

void GUIHolder::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
    gui.input_finger_touch_callback(touch);
}

void GUIHolder::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
    gui.input_finger_motion_callback(motion);
}

std::optional<InputManager::TextBoxStartInfo> GUIHolder::get_text_box_start_info() {
    return gui.get_text_box_start_info();
}
