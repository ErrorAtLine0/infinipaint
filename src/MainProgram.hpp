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
        float fpsLimit = 10000.0f;

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
            int vsyncValue = 1;

            float density = 1.0f;
            float scale = 1.0f;
            #ifndef __EMSCRIPTEN__
                bool applyDisplayScale = true;
            #endif

            bool disableGraphicsDriverWorkarounds = false;

            int defaultMSAASampleCount = 0;
            SkSurfaceProps defaultMSAASurfaceProps;

            #ifdef USE_SKIA_BACKEND_GRAPHITE
                std::function<skgpu::graphite::Recorder*()> recorder;
            #elif USE_SKIA_BACKEND_GANESH
                sk_sp<GrDirectContext> ctx;
            #endif
            sk_sp<SkSurface> surface;

            SDL_Window* sdlWindow;
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
        void draw(SkCanvas* canvas);

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

        std::filesystem::path homePath;
        std::filesystem::path configPath;
        std::filesystem::path documentsPath;

        bool drawGui = true;
        bool takingScreenshot = false;
        bool transparentBackground = false;

        Vector3f defaultCanvasBackgroundColor = DEFAULT_CANVAS_BACKGROUND_COLOR;

        size_t worldIndex = 0;
        std::vector<std::shared_ptr<World>> worlds;

        std::string displayName;

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
