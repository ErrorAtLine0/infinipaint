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
#include "CustomEvents.hpp"
#include "DrawData.hpp"
#include <SDL3/SDL_render.h>
#include <chrono>
#include <include/core/SkCanvas.h>
#include "Helpers/Hashes.hpp"
#include "InputManager.hpp"
#include "FontData.hpp"
#include "TimePoint.hpp"
#include "SharedTypes.hpp"
#include <Eigen/Dense>
#include <include/core/SkSurfaceProps.h>
#include "Toolbar.hpp"
#include "World.hpp"
#include "DrawingProgram/ToolConfiguration.hpp"
#include "GUIHolder.hpp"
#include "GlobalConfig.hpp"
#include "Screens/Screen.hpp"

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include "include/gpu/graphite/Recorder.h"
#elif USE_SKIA_BACKEND_GANESH
    #include "include/gpu/ganesh/GrDirectContext.h"
#endif

using namespace Eigen;

struct UserLogMessage {
    static constexpr float DISPLAY_TIME = 8.0f;
    static constexpr float FADE_START_TIME = 7.0f;
    std::string text;
    enum {
        COLOR_NORMAL = 0,
        COLOR_ERROR
    } color;
    enum {
        DISPLAY_FOR_ALL,
        DISPLAY_PHONE_ONLY,
        DISPLAY_DESKTOP_ONLY
    } whereToDisplay = DISPLAY_DESKTOP_ONLY;
    TimePoint time;
};

class MainProgram {
    public:
        static constexpr size_t LOG_SIZE = 30;
        static constexpr const char* UPDATE_DOWNLOAD_URL = "https://infinipaint.com/download.html";

        InputManager input;

        struct Window {
            std::chrono::steady_clock::duration lastFrameTime = std::chrono::milliseconds(16);

            SCollision::AABB<float> safeArea;

            Vector2i size = {-1, -1};
            Vector2i pos = {-1, -1};
            Vector2i writtenSize = {-1, -1};
            Vector2i writtenPos = {-1, -1};
            bool maximized = false;
            bool fullscreen = false;
            SkColorType defaultColorType;
            SkAlphaType defaultAlphaType;

            float density = 1.0f;
            float scale = 1.0f;
            bool windowFocus = true;
            bool mouseFocus = true;

            int defaultMSAASampleCount = 0;
            SkSurfaceProps defaultMSAASurfaceProps;

            #ifdef USE_SKIA_BACKEND_GRAPHITE
                std::function<skgpu::graphite::Recorder*()> recorder;
            #elif USE_SKIA_BACKEND_GANESH
                sk_sp<GrDirectContext> ctx;
            #endif

            sk_sp<SkSurface> nativeSurface;
            sk_sp<SkSurface> intermediateSurfaceMSAA;

            SDL_Window* sdlWindow;
            bool canCreateSurfaces = false;
        } window;

        struct Clipboard {
            std::vector<std::unique_ptr<CanvasComponentContainer::CopyData>> components;
            std::unordered_map<NetworkingObjects::NetObjID, ResourceData> resources;
            WorldVec pos;
            WorldScalar inverseScale;
        } clipboard;

        std::shared_ptr<FontData> fonts;
        TimePoint deltaTime;
        GUIHolder g;
        ToolConfiguration toolConfig;

        std::ofstream* logFile;
        std::deque<UserLogMessage> logMessages;

        MainProgram();
        void update();
        void background_update();
        void draw_world(SkCanvas* canvas, std::shared_ptr<World> worldToDraw, const DrawData& drawData);
        void draw(SkCanvas* canvas);
        sk_sp<SkSurface> create_native_surface(Vector2i resolution, bool isMSAA);

        bool setToQuit = false;
        
        void early_destroy();

        bool network_being_used();
        bool net_server_hosted();
        void update_display_names();

        void save_config();
        void load_config();

        void init_net_library();
        void set_vsync_value(int vsyncValue);
        void update_scale_and_density();
        float get_scale_and_density_factor_gui();
        bool app_close_requested();
        void toggle_full_screen();
        void update_main_loop_call_rate(unsigned newCallbackRate);
        
        void set_first_screen(std::unique_ptr<Screen> firstScreen);
        void set_screen(std::function<std::unique_ptr<Screen>(std::unique_ptr<Screen>)> screenFunc);

        void refresh_draw_surfaces();

        std::filesystem::path homePath;
        std::filesystem::path documentsPath;

        bool drawGui = true;

        std::shared_ptr<World> world;
        std::vector<std::shared_ptr<World>> worlds;

        GlobalConfig conf;

        std::optional<unsigned> keybindWaiting;
        void input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile);
        void input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile);
        void input_paste_callback(const CustomEvents::PasteEvent& paste);
        void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput);
        void input_mobile_import_canvas_callback(const CustomEvents::MobileImportCanvasEvent& mobileImport);

        void input_app_about_to_go_to_background_callback();
        void input_app_about_to_go_to_foreground_callback();
        void input_global_back_button_callback();
        bool input_keybind_callback(const Vector2ui32& newKey);
        void input_drop_file_callback(const InputManager::DropCallbackArgs& drop);
        void input_drop_text_callback(const InputManager::DropCallbackArgs& drop);
        void input_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_text_callback(const InputManager::TextCallbackArgs& text);
        void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button);
        void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
        void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel);
        void input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button);
        void input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch);
        void input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion);
        void input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis);
        void input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch);
        void input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion);
        void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch);
        void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion);
        void input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w);
        void input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w);
        void input_window_mouse_focus_gained();
        void input_window_mouse_focus_lost();
        void input_window_focus_gained();
        void input_window_focus_lost();

        std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info();

        void create_new_tab(const CustomEvents::OpenInfiniPaintFileEvent& openFile);
        void set_tab_to_close(World* world);
        void switch_to_tab(size_t wIndex);

        ~MainProgram();

        void update_notification_check();
        struct UpdateCheckerData {
            bool showGui = false;
            bool updateCheckDone = false;
            std::string newVersionStr;
            std::shared_ptr<FileDownloader::DownloadData> versionFile;
        } updateCheckerData;

        float calculate_gui_scale();
    private:
        std::unordered_set<World*> tabsToClose;
        void close_set_to_close_tabs();
        void post_callback();
        void run_new_screen_func();

        std::string gen_random_display_name();
        std::function<std::unique_ptr<Screen>(std::unique_ptr<Screen>)> newScreenFunc;
        std::unique_ptr<Screen> screen;

        void gui();
        void draw_grid(SkCanvas* canvas);
};
