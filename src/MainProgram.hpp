#pragma once
#include "DrawData.hpp"
#include "GridManager.hpp"
#include <include/core/SkCanvas.h>
#include "InputManager.hpp"
#include "FontData.hpp"
#include "TimePoint.hpp"
#include "SharedTypes.hpp"
#include <Eigen/Dense>
#include <include/core/SkSurfaceProps.h>
#include "Toolbar.hpp"
#include "World.hpp"

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

        struct DrawingProgramImageCache {
            bool refresh = true;
            sk_sp<SkSurface> surface;
            std::shared_ptr<DrawComponent> firstCompUpdate = nullptr;
            SkCanvas* canvas;
            bool disableDrawCache = false;
        } drawProgCache;

        struct Window {
            Vector2i size = {-1, -1};
            Vector2i pos = {-1, -1};
            Vector2i writtenSize = {-1, -1};
            Vector2i writtenPos = {-1, -1};
            bool maximized = false;
            bool fullscreen = false;
            SkColorType defaultColorType;
            SkAlphaType defaultAlphaType;

            float scale = 1.0f;

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
            std::vector<std::shared_ptr<DrawComponent>> components;
            std::unordered_map<ServerClientID, ResourceData> resources;
            WorldVec pos;
        } clipboard;

        FontData fonts;
        TimePoint deltaTime;
        Toolbar toolbar;
        GridManager grid;
        std::shared_ptr<World> world;

        std::ofstream* logFile;
        std::deque<Toolbar::LogMessage> logMessages;

        bool disableIntelWorkaround = false;

        std::string newAddr;
        
        MainProgram();
        void update();
        void draw(SkCanvas* canvas);

        void init_draw_program_cache();

        bool setToQuit = false;
        
        void early_destroy();

        void new_tab(const World::OpenWorldInfo& tabInfo, bool createSameThread = false);
        void set_tab_to_close(size_t tabToClose);
        bool network_being_used();
        bool net_server_hosted();
        void update_display_names();

        void save_config();
        void load_config();

        std::filesystem::path homePath;
        std::filesystem::path configPath;

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

        std::optional<size_t> setTabToClose;

        std::string gen_random_display_name();

        void gui();
        void draw_grid(SkCanvas* canvas);
};
