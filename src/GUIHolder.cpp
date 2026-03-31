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
    gui.io.layoutRun = [&] {
        main.toolbar.layout_run();
    };
    gui.io.input = &main.input;

    // NOTE: On windows, when the native file picker is open, any call to MakeFromFile fails
    // So, it's better to load the icons at the beginning of the program so that the icon loading doesn't fail later
    load_icons_at("data/icons");

#ifdef __ANDROID__
    load_icons_at("data/icons/RemixIcon"); // SDL_GlobDirectory doesn't work recursively on Android
#endif
    
    load_default_theme();
}

void GUIHolder::load_icons_at(const std::filesystem::path& pathToLoad) {
    int globCount;
    char** filesInPath = SDL_GlobDirectory(pathToLoad.c_str(), nullptr, 0, &globCount);
    if(filesInPath) {
        for(int i = 0; i < globCount; i++) {
            std::filesystem::path filePath = pathToLoad / std::filesystem::path(filesInPath[i]);
            SDL_PathInfo fileInfo;
            if(SDL_GetPathInfo(filePath.c_str(), &fileInfo) && fileInfo.type == SDL_PATHTYPE_FILE) {
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
    gui.io.windowSize = main.window.size.cast<float>();
    gui.io.windowPos = {0, 0};
    calculate_final_gui_scale();
    gui.layout_if_necessary();
}

float GUIHolder::final_gui_scale() {
    return finalCalculatedGuiScale;
}

void GUIHolder::calculate_final_gui_scale() {
    Vector2f maxWindowSizeBeforeForcedFit = final_gui_scale_not_fit() * Vector2f{700.0f, 700.0f};
    Vector2f fitRatio = {main.window.size.x() / maxWindowSizeBeforeForcedFit.x(), main.window.size.y() / maxWindowSizeBeforeForcedFit.y()};
    finalCalculatedGuiScale = final_gui_scale_not_fit() * std::min(std::min(fitRatio.x(), fitRatio.y()), 1.0f);
}

float GUIHolder::final_gui_scale_not_fit() {
    return main.conf.guiScale * main.get_scale_and_density_factor_gui();
}

void GUIHolder::draw(SkCanvas* canvas, bool skiaAA) {
    canvas->save();
    canvas->scale(final_gui_scale(), final_gui_scale());
    gui.draw(canvas, skiaAA);
    canvas->restore();
}

void GUIHolder::input_key_callback(const InputManager::KeyCallbackArgs& key) {
}

void GUIHolder::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
}

void GUIHolder::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
}

void GUIHolder::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
}

void GUIHolder::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
}

void GUIHolder::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
}
