#pragma once
#include "DrawData.hpp"
#include <SDL3/SDL_render.h>
#include <chrono>
#include <include/core/SkCanvas.h>
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

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include "include/gpu/graphite/Recorder.h"
#elif USE_SKIA_BACKEND_GANESH
    #include "include/gpu/ganesh/GrDirectContext.h"
#endif

using namespace Eigen;

class MainProgram {
    public:
        static constexpr size_t LOG_SIZE = 30;

        InputManager input;

        std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();

        Vector3f userColor;

        struct Window {
            std::chrono::steady_clock::duration lastFrameTime = std::chrono::milliseconds(16);
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

            int defaultMSAASampleCount = 0;
            SkSurfaceProps defaultMSAASurfaceProps;

            #ifdef USE_SKIA_BACKEND_GRAPHITE
                std::function<skgpu::graphite::Recorder*()> recorder;
            #elif USE_SKIA_BACKEND_GANESH
                sk_sp<GrDirectContext> ctx;
            #endif

            sk_sp<SkSurface> intermediateSurface;
            SkCanvas* intermediateCanvas;

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
        Toolbar toolbar;
        std::shared_ptr<World> world;
        ToolConfiguration toolConfig;

        std::ofstream* logFile;
        std::deque<Toolbar::LogMessage> logMessages;

        std::string newAddr;
        
        MainProgram();
        void update();
        void draw(SkCanvas* canvas, std::shared_ptr<World> worldToDraw, const DrawData& drawData);
        sk_sp<SkSurface> create_native_surface(Vector2i resolution, bool isMSAA);

        bool setToQuit = false;
        
        void early_destroy();

        void new_tab(const World::OpenWorldInfo& tabInfo, bool createSameThread = false);
        void set_tab_to_close(const std::weak_ptr<World>& tabToClose);
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

        void refresh_draw_surfaces();

        std::filesystem::path homePath;
        std::filesystem::path configPath;
        std::filesystem::path documentsPath;

        bool drawGui = true;

        size_t worldIndex = 0;
        std::vector<std::shared_ptr<World>> worlds;

        GUIHolder g;
        GlobalConfig conf;

        void input_drop_file_callback(const InputManager::DropCallbackArgs& drop);
        void input_drop_text_callback(const InputManager::DropCallbackArgs& drop);
        void input_key_callback(const InputManager::KeyCallbackArgs& key);
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

        ~MainProgram();
    private:
        void new_tab_open();
        std::atomic<bool> tabSetToOpen = false;
        World::OpenWorldInfo newTabToOpenInfo;

        std::vector<std::weak_ptr<World>> setTabsToClose;

        std::string gen_random_display_name();

        void gui();
        void draw_grid(SkCanvas* canvas);
};
