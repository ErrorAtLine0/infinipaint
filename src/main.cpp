#include "Helpers/SCollision.hpp"
#include "Helpers/StringHelpers.hpp"
#include "Helpers/FileDownloader.hpp"
#include "include/gpu/GpuTypes.h"
#include "sago/platform_folders.h"
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_oldnames.h>
#include <chrono>
#include <filesystem>
#include <include/core/SkAlphaType.h>
#include <include/core/SkColorType.h>
#ifdef USE_BACKEND_OPENGL 
#ifndef __EMSCRIPTEN__
    #ifdef USE_BACKEND_OPENGLES_3_0
        #define GLAD_GLES2_IMPLEMENTATION
        #include <glad/gles3_0.h>
    #elif USE_BACKEND_OPENGL_2_1
        #define GLAD_GL_IMPLEMENTATION
        #include <glad/gl2_1.h>
    #else
        #define GLAD_GL_IMPLEMENTATION
        #include <glad/gl3_3.h>
    #endif
#endif
#endif

#include "cereal/archives/portable_binary.hpp"
#include <include/core/SkCanvas.h>
#include <include/core/SkSurface.h>
#include <include/core/SkSurfaceProps.h>
#ifdef USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
    #include <include/gpu/ganesh/GrBackendSurface.h>
#elif USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#endif

#include <iostream>
#include <cereal/types/string.hpp>

#include "MainProgram.hpp"

#include <include/codec/SkPngDecoder.h>

#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL_main.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pen.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL.h>

#include <fstream>

#ifdef __EMSCRIPTEN__
    #include <SDL3/SDL_opengl.h>
    #include <include/gpu/ganesh/gl/GrGLDirectContext.h>
    #include <include/gpu/ganesh/gl/GrGLInterface.h>
    #include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
    #include <include/gpu/ganesh/gl/GrGLTypes.h>
    #include <emscripten/html5.h>
    #include <emscripten.h>
    #include <emscripten/emscripten.h>
#elif USE_BACKEND_VULKAN
    #ifdef USE_SKIA_BACKEND_GRAPHITE
        #include "VulkanContext/GraphiteNativeVulkanWindowContext.h"
    #elif USE_SKIA_BACKEND_GANESH
        #include "VulkanContext/VulkanWindowContext.h"
    #endif
    #include <tools/window/DisplayParams.h>
    #include <SDL3/SDL_vulkan.h>
#elif USE_BACKEND_OPENGL
    #include <include/gpu/ganesh/gl/GrGLDirectContext.h>
    #include <include/gpu/ganesh/gl/GrGLInterface.h>
    #include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
    #include <include/gpu/ganesh/gl/GrGLTypes.h>
#endif

#ifdef __linux__
#include <unistd.h>
#include <pwd.h>
#elif _WIN32
#include <shlobj.h>
#endif

#include <Helpers/Logger.hpp>
#include "SwitchCWD.hpp"

// Use dedicated graphics card on Windows
#ifdef _WIN32
extern "C" 
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#include <unicode/udata.h>
std::string icudt; // Put this in global space so that it isn't deallocated

struct MainStruct {
    #ifdef USE_BACKEND_VULKAN
        #ifdef USE_SKIA_BACKEND_GANESH
            std::unique_ptr<skwindow::internal::VulkanWindowContext> vulkanWindowContext;
        #elif USE_SKIA_BACKEND_GRAPHITE
            std::unique_ptr<skwindow::internal::GraphiteVulkanWindowContext> vulkanWindowContext;
        #endif
    #elif USE_BACKEND_OPENGL
        unsigned kStencilBits = 8;
        SDL_GLContext gl_context;
        GLint defaultFBO = 0;
    #endif

    #ifdef USE_SKIA_BACKEND_GRAPHITE
    #elif USE_SKIA_BACKEND_GANESH
        sk_sp<GrDirectContext> ctx;
        GrBackendRenderTarget target;
    #endif

    std::unique_ptr<MainProgram> m;
    
    SDL_Window* window;
    
    std::array<SDL_Cursor*, SDL_SYSTEM_CURSOR_COUNT> systemCursors;
    unsigned currentCursor = 0;
    
    SkCanvas* canvas;
    sk_sp<SkSurface> surface;

    sk_sp<SkSurface> intermediateSurface;
    SkCanvas* intermediateCanvas;

    SDL_Surface* iconSurface = nullptr;
    std::string iconData;

    SDL_Cursor* hiddenCursor = nullptr;

    std::filesystem::path configPath;
    std::filesystem::path homePath;
    std::ofstream logFile;
} MainData;

#ifdef __EMSCRIPTEN__
    bool emscriptenFilesystemReadReady = false;
    bool emscriptenFilesystemLoadConfigDone = false;
#endif

bool load_file_to_string(std::string& toRet, std::string_view fileName) {
    // https://nullptr.org/cpp-read-file-into-string/
    std::ifstream file(fileName.data(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        std::cout << "[load_file_to_string] Could not open file " << fileName << std::endl;
        return false;
    }
    size_t fileSize;
    auto tellgResult = file.tellg();
    if(tellgResult == -1) {
        std::cout << "[load_file_to_string] tellg failed for file " << fileName << std::endl;
        return false;
    }

    fileSize = static_cast<size_t>(tellgResult);

    file.seekg(0, std::ios_base::beg);

    toRet.resize(fileSize);
    file.read(toRet.data(), fileSize);

    file.close();
    return true;
}

void initialize_sdl(MainStruct& mS, int wWidth, int wHeight) {
    SDL_SetHint(SDL_HINT_APP_NAME, "InfiniPaint");
    SDL_SetHint(SDL_HINT_PEN_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas"); // Ensures that SDL only grabs input when browser is focused on canvas

    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        throw std::runtime_error("[SDL_Init] " + std::string(SDL_GetError()));

    Uint32 window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifdef USE_BACKEND_VULKAN
    window_flags |= SDL_WINDOW_VULKAN;
#elif USE_BACKEND_OPENGL
    window_flags |= SDL_WINDOW_OPENGL;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);

    #if defined(__EMSCRIPTEN__) || defined(USE_BACKEND_OPENGLES_3_0)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #elif defined(USE_BACKEND_OPENGL_2_1)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    #endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, mS.kStencilBits);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
#endif

    mS.window = SDL_CreateWindow("InfiniPaint", wWidth, wHeight, window_flags);
    if(mS.window == nullptr)
        throw std::runtime_error("[SDL_CreateWindow] " + std::string(SDL_GetError()));

    if(load_file_to_string(mS.iconData, "data/progicons/icon.png")) {
        sk_sp<SkData> newData = SkData::MakeWithoutCopy(mS.iconData.c_str(), mS.iconData.size());
        std::unique_ptr<SkCodec> codec = SkCodec::MakeFromData(newData, {SkPngDecoder::Decoder()});
        if(codec) {
            sk_sp<SkImage> newImage = std::get<0>(codec->getImage());
            SkPixmap px;
            newImage->peekPixels(&px);
            mS.iconSurface = SDL_CreateSurfaceFrom(newImage->width(), newImage->height(), SDL_PIXELFORMAT_RGBA32, px.writable_addr(), px.rowBytes());
            SDL_SetWindowIcon(mS.window, mS.iconSurface);
        }
    }

    SDL_SetWindowPosition(mS.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

#ifdef USE_BACKEND_OPENGL
    mS.gl_context = SDL_GL_CreateContext(mS.window);
    if(mS.gl_context == nullptr)
        throw std::runtime_error("[SDL_GL_CreateContext] " + std::string(SDL_GetError()));

    SDL_GL_MakeCurrent(mS.window, mS.gl_context);

    #ifndef __EMSCRIPTEN__

        #ifdef USE_BACKEND_OPENGLES_3_0
            if(!gladLoadGLES2(SDL_GL_GetProcAddress))
                throw std::runtime_error("[gladLoadGLES2] Failed to load GLAD OpenGLES 3.0 Loader");
        #elif defined(USE_BACKEND_OPENGL_2_1)
            if(!gladLoadGL(SDL_GL_GetProcAddress))
                throw std::runtime_error("[gladLoadGL] Failed to load GLAD OpenGL 2.1 Loader");
        #else
            if(!gladLoadGL(SDL_GL_GetProcAddress))
                throw std::runtime_error("[gladLoadGL] Failed to load GLAD OpenGL 3.3 Loader");
        #endif

        #ifndef USE_BACKEND_OPENGL_2_1
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mS.defaultFBO);
        #endif

        Logger::get().log("INFO", "GL Version: " + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))));
    #endif

#endif

    SDL_SetWindowSize(mS.window, wWidth, wHeight);

    SDL_ShowWindow(mS.window);

    for(unsigned i = 0; i < SDL_SYSTEM_CURSOR_COUNT; i++)
        mS.systemCursors[i] = SDL_CreateSystemCursor((SDL_SystemCursor)i);
}

void sdl_terminate(MainStruct& mS) {
    for(unsigned i = 0; i < SDL_SYSTEM_CURSOR_COUNT; i++)
        SDL_DestroyCursor(mS.systemCursors[i]);
    if(mS.iconSurface)
        SDL_DestroySurface(mS.iconSurface);
    if(mS.hiddenCursor)
        SDL_DestroyCursor(mS.hiddenCursor);

    //SDL_GL_DestroyContext(mS.gl_context);
    //SDL_DestroyWindow(mS.window);
    //SDL_Quit();
}

void resize_window(MainStruct& mS) {
    // Having intermediate surface allows for changing options more easily
    #ifdef USE_BACKEND_OPENGLES_3_0
        SkImageInfo imgInfo = SkImageInfo::Make(mS.m->window.size.x(), mS.m->window.size.y(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    #else
        SkImageInfo imgInfo = SkImageInfo::MakeN32Premul(mS.m->window.size.x(), mS.m->window.size.y());
    #endif
#ifdef USE_SKIA_BACKEND_GRAPHITE
    mS.intermediateSurface = SkSurfaces::RenderTarget(mS.vulkanWindowContext->graphiteRecorder(), imgInfo, skgpu::Mipmapped::kNo, &mS.m->window.defaultMSAASurfaceProps);
#elif USE_SKIA_BACKEND_GANESH
    mS.intermediateSurface = SkSurfaces::RenderTarget(mS.ctx.get(), skgpu::Budgeted::kNo, imgInfo, mS.m->window.defaultMSAASampleCount, &mS.m->window.defaultMSAASurfaceProps);
#endif
    if(!mS.intermediateSurface)
        throw std::runtime_error("[resize_window] Could not make intermediate surface");
    mS.intermediateCanvas = mS.intermediateSurface->getCanvas();

    if(!mS.intermediateCanvas)
        throw std::runtime_error("[resize_window] Could not create intermediate canvas");

    mS.m->window.surface = mS.intermediateSurface;

#ifdef USE_BACKEND_VULKAN
    mS.vulkanWindowContext->resize(mS.m->window.size.x(), mS.m->window.size.y());
#elif USE_BACKEND_OPENGL

    GrGLFramebufferInfo info;
    info.fFBOID = (GrGLuint)mS.defaultFBO;
    info.fFormat = GL_RGBA8;

    SkSurfaceProps props2;
    mS.target = GrBackendRenderTargets::MakeGL(mS.m->window.size.x(), mS.m->window.size.y(), 0, mS.kStencilBits, info);
    mS.surface = SkSurfaces::WrapBackendRenderTarget(mS.ctx.get(), mS.target, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, &props2);

    if(!mS.surface)
        throw std::runtime_error("[resize_window] Could not make surface");
    mS.canvas = mS.surface->getCanvas();
    if(!mS.canvas)
        throw std::runtime_error("[resize_window] No canvas made");

#endif
}

#ifdef __EMSCRIPTEN__
extern "C" {
    EMSCRIPTEN_KEEPALIVE inline void emscripten_filesystem_loaded() {
        emscriptenFilesystemReadReady = true;
    }
}

const char* emscripten_before_unload(int eventType, const void *reserved, void *userData) {
    MainStruct& mS = *((MainStruct*)userData);
    mS.m->save_config();
    return "";
}
#endif

void init_logs(MainStruct& mS) {
    mS.homePath = std::filesystem::current_path();
#ifdef __EMSCRIPTEN__
    EM_ASM(
        FS.mkdir('/infinipaint');
        FS.mount(FS.filesystems.IDBFS, {}, '/infinipaint');
        
        FS.syncfs(true, (err) => {
            if(err)
                console.log("Error syncing IDBFS: ", err);
            else {
                console.log("Synced to IDBFS. Read Ready");
                Module["ccall"]('emscripten_filesystem_loaded', null, [], []);
            }
        });
    );
    mS.homePath = std::filesystem::path("/");
    mS.configPath = std::filesystem::path("/infinipaint");
#elif CONFIG_NEXT_TO_EXECUTABLE
    std::string CONFIG_FOLDER_NAME = "config";
    std::filesystem::create_directory(CONFIG_FOLDER_NAME);
    mS.configPath = mS.homePath / CONFIG_FOLDER_NAME;
#else
    std::string CONFIG_FOLDER_NAME = "infinipaint";
    std::filesystem::path configHome(sago::getConfigHome());
    std::filesystem::create_directory(configHome);
    mS.configPath = configHome / CONFIG_FOLDER_NAME;
#endif

    if(!std::filesystem::create_directory(mS.configPath))
        std::cout << "Failed to create config directory." << std::endl;

    mS.logFile = std::ofstream(mS.configPath / "log.txt");
    Logger::get().add_log("FATAL", [&, mS = &mS](const std::string& text) {
        mS->logFile << "[FATAL] " << text << std::endl;
        std::cerr << "[FATAL] " << text << std::endl;
        mS->logFile.close();
    });
    Logger::get().add_log("INFO", [&, mS = &mS](const std::string& text) {
        mS->logFile << "[INFO] " << text << std::endl;
        std::cout << "[INFO] " << text << std::endl;
    });

    Logger::get().log("INFO", "Home Path: " + mS.homePath.string());
    Logger::get().log("INFO", "Config Path: " + mS.configPath.string());
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    std::vector<std::filesystem::path> listOfFilesToOpenFromCommand;
    for(int i = 1; i < argc; i++)
        listOfFilesToOpenFromCommand.emplace_back(std::filesystem::canonical(std::filesystem::path(argv[i])));
    switch_cwd();

    UErrorCode uerr = U_ZERO_ERROR;
    icudt = read_file_to_string("data/icudt77l.dat");
    udata_setCommonData((void *) icudt.data(), &uerr);
    if (U_FAILURE(uerr)) {
        char* errorName = const_cast<char*>(u_errorName(uerr));
        std::cout << errorName << std::endl;
    }

    MainStruct* mSPtr = new MainStruct;
    *appstate = (void*)mSPtr;
    MainStruct& mS = *mSPtr;

    init_logs(mS);

    FileDownloader::init();

#ifdef NDEBUG
    try {
#endif
        GrContextOptions opts;
        opts.fSuppressPrints = true;

        int initWidth = 1000;
        int initHeight = 900;

#ifdef __EMSCRIPTEN__
        Vector2d sizeD;
        EMSCRIPTEN_RESULT result = emscripten_get_element_css_size("#canvas", &sizeD.x(), &sizeD.y());
        if(result != EMSCRIPTEN_RESULT_SUCCESS)
            std::cout << "Failed to get canvas size" << std::endl;
        else {
            initWidth = sizeD.x();
            initHeight = sizeD.y();
            std::cout << "Initial window size: " << initWidth << " " << initHeight << std::endl;
        }
#endif

        initialize_sdl(mS, initWidth, initHeight);

        int32_t cursorData[2] = {0, 0};
        mS.hiddenCursor = SDL_CreateCursor((Uint8 *)cursorData, (Uint8 *)cursorData, 8, 8, 4, 4);

        mS.m = std::make_unique<MainProgram>();
        mS.m->window.defaultMSAASampleCount = 0;
        mS.m->window.defaultMSAASurfaceProps = SkSurfaceProps(SkSurfaceProps::kDynamicMSAA_Flag, kUnknown_SkPixelGeometry);
        mS.m->logFile = &mS.logFile;
        mS.m->configPath = mS.configPath;
        mS.m->homePath = mS.homePath;
        mS.m->window.sdlWindow = mS.window;
        mS.m->window.scale = SDL_GetWindowPixelDensity(mS.window);
#ifdef __EMSCRIPTEN__
        emscripten_set_beforeunload_callback((void*)mSPtr, emscripten_before_unload);
#else
        mS.m->load_config();
#endif
        #ifdef USE_BACKEND_VULKAN
            #ifdef USE_SKIA_BACKEND_GRAPHITE
                std::unique_ptr<const skwindow::DisplayParams> displayParams = skwindow::DisplayParamsBuilder().build();

                mS.vulkanWindowContext = std::make_unique<skwindow::internal::GraphiteVulkanWindowContext>(std::move(displayParams),
                                                                [window = mS.window](VkInstance instance) {
                                                                    VkSurfaceKHR toRet;
                                                                    if(!SDL_Vulkan_CreateSurface(window, instance, nullptr, &toRet))
                                                                        throw std::runtime_error("Failed to create SDL vulkan surface");
                                                                    else
                                                                        std::cout << "Successfully created surface!" << std::endl;
                                                                    return toRet;
                                                                },
                                                                SDL_Vulkan_GetPresentationSupport,
                                                                (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr());
                mS.surface = mS.vulkanWindowContext->getBackbufferSurface();
            #elif USE_SKIA_BACKEND_GANESH
                std::unique_ptr<const skwindow::DisplayParams> displayParams = skwindow::DisplayParamsBuilder().build();

                mS.vulkanWindowContext = std::make_unique<skwindow::internal::VulkanWindowContext>(std::move(displayParams),
                                            [window = mS.window](VkInstance instance) {
                                                VkSurfaceKHR toRet;
                                                if(!SDL_Vulkan_CreateSurface(window, instance, nullptr, &toRet))
                                                    throw std::runtime_error("Failed to create SDL vulkan surface");
                                                else
                                                    std::cout << "Successfully created surface!" << std::endl;
                                                return toRet;
                                            },
                                            SDL_Vulkan_GetPresentationSupport,
                                            (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr());

                mS.ctx = mS.vulkanWindowContext->fContext; // fContext is supposed to be private, but put it in public for convenience for this app
                mS.surface = mS.vulkanWindowContext->getBackbufferSurface();
            #endif
        #elif USE_BACKEND_OPENGL
            sk_sp<const GrGLInterface> iface = GrGLMakeNativeInterface();

            mS.ctx = GrDirectContexts::MakeGL(iface, opts);
            if(!mS.ctx)
                throw std::runtime_error("[GrDirectContexts::MakeGL] Could not make context");
        #endif

#ifdef USE_SKIA_BACKEND_GRAPHITE
        mS.m->window.recorder = [&]() { return mS.vulkanWindowContext->graphiteRecorder(); };
#elif USE_SKIA_BACKEND_GANESH
        mS.m->window.ctx = mS.ctx;
#endif

#ifdef __EMSCRIPTEN__
        SDL_GetWindowPosition(mS.window, &mS.m->window.pos.x(), &mS.m->window.pos.y());
        mS.m->window.writtenPos.x() = mS.m->window.pos.x();
        mS.m->window.writtenPos.y() = mS.m->window.pos.y();
        mS.m->window.size.x() = initWidth;
        mS.m->window.size.y() = initHeight;
        mS.m->window.writtenSize.x() = initWidth;
        mS.m->window.writtenSize.y() = initHeight;
#else
        if(mS.m->window.pos.x() >= 0 && mS.m->window.pos.y() >= 0 && mS.m->window.size.x() >= 0 && mS.m->window.size.y() >= 0) {
            SDL_SetWindowSize(mS.window, mS.m->window.size.x(), mS.m->window.size.y());
            SDL_SetWindowPosition(mS.window, mS.m->window.pos.x(), mS.m->window.pos.y());
        }
        else {
            SDL_GetWindowPosition(mS.window, &mS.m->window.pos.x(), &mS.m->window.pos.y());
            mS.m->window.writtenPos.x() = mS.m->window.pos.x();
            mS.m->window.writtenPos.y() = mS.m->window.pos.y();
            mS.m->window.size.x() = initWidth;
            mS.m->window.size.y() = initHeight;
            mS.m->window.writtenSize.x() = initWidth;
            mS.m->window.writtenSize.y() = initHeight;
        }

        if(mS.m->window.maximized)
            SDL_MaximizeWindow(mS.window);

        if(mS.m->window.fullscreen)
            SDL_SetWindowFullscreen(mS.window, true);
#endif

        resize_window(mS);

        if(listOfFilesToOpenFromCommand.empty()) {
            mS.m->new_tab({
                .conType = World::CONNECTIONTYPE_LOCAL
            }, true);
        }
        else {
            for(auto& f : listOfFilesToOpenFromCommand) {
                mS.m->new_tab({
                    .conType = World::CONNECTIONTYPE_LOCAL,
                    .fileSource = f.string()
                }, true);
            }
        }
#ifdef NDEBUG
    }
    catch(const std::exception& e) {
        Logger::get().log("FATAL", e.what());
        return SDL_APP_FAILURE;
    }
#endif
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    std::chrono::steady_clock::time_point frameTimeStart = std::chrono::steady_clock::now();

    MainStruct& mS = *((MainStruct*)appstate);
#ifdef NDEBUG
    try {
#endif
        mS.m->window.scale = SDL_GetWindowPixelDensity(mS.window);

#ifdef __EMSCRIPTEN__
        if(emscriptenFilesystemReadReady && !emscriptenFilesystemLoadConfigDone) {
            mS.m->load_config();
            emscriptenFilesystemLoadConfigDone = true;
        }
#endif

        mS.m->update();

        if(mS.m->setToQuit)
            return SDL_APP_SUCCESS;

        if(mS.m->input.toggleFullscreen) {
            SDL_SetWindowFullscreen(mS.window, !mS.m->window.fullscreen);
            mS.m->window.fullscreen = !mS.m->window.fullscreen;
        }

        if(mS.m->input.hideCursor) {
            SDL_HideCursor();
            SDL_SetCursor(mS.hiddenCursor); // Using this because SDL hidden cursor sometimes doesnt work
        }
        else {
            SDL_ShowCursor();
            SDL_SetCursor(mS.systemCursors[static_cast<unsigned>(mS.m->input.cursorIcon)]);
        }

        if(mS.m->input.text.get_accepting_input() && !mS.m->input.text.lastAcceptingTextInputVal)
            SDL_StartTextInput(mS.window);
        else if(!mS.m->input.text.get_accepting_input() && mS.m->input.text.lastAcceptingTextInputVal)
            SDL_StopTextInput(mS.window);
        mS.m->input.text.lastAcceptingTextInputVal = mS.m->input.text.get_accepting_input();

        mS.intermediateCanvas->save();
        mS.m->draw(mS.intermediateCanvas);
        mS.intermediateCanvas->restore();

        #ifdef USE_BACKEND_VULKAN
            mS.vulkanWindowContext->getBackbufferSurface()->getCanvas()->drawImage(mS.intermediateSurface->makeTemporaryImage(), 0, 0);
            mS.vulkanWindowContext->swapBuffers();
        #elif USE_BACKEND_OPENGL

            mS.canvas->drawImage(mS.intermediateSurface->makeTemporaryImage(), 0, 0);

            mS.ctx->flush();

            SDL_GL_SwapWindow(mS.window);
        #endif

        mS.m->input.frame_reset(mS.m->window.size);
#ifdef NDEBUG
    }
    catch(const std::exception& e) {
        Logger::get().log("FATAL", e.what());
        return SDL_APP_FAILURE;
    }
#endif

    mS.m->window.lastFrameTime = std::chrono::steady_clock::now() - frameTimeStart;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    MainStruct& mS = *((MainStruct*)appstate);

#ifdef NDEBUG
    try {
#endif
        switch(event->type) {
            case SDL_EVENT_QUIT:
                return SDL_APP_SUCCESS;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if(event->window.windowID == SDL_GetWindowID(mS.window))
                    return SDL_APP_SUCCESS;
                break;
            case SDL_EVENT_WINDOW_MOVED:
                mS.m->window.pos.x() = event->window.data1;
                mS.m->window.pos.y() = event->window.data2;
                if(!mS.m->window.fullscreen && !mS.m->window.maximized) {
                    mS.m->window.writtenPos.x() = event->window.data1;
                    mS.m->window.writtenPos.y() = event->window.data2;
                }
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                mS.m->window.size.x() = event->window.data1;
                mS.m->window.size.y() = event->window.data2;
                if(!mS.m->window.fullscreen && !mS.m->window.maximized) {
                    mS.m->window.writtenSize.x() = event->window.data1;
                    mS.m->window.writtenSize.y() = event->window.data2;
                }
                resize_window(mS);
                break;
            case SDL_EVENT_WINDOW_MAXIMIZED:
                mS.m->window.maximized = true;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                mS.m->window.maximized = false;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                mS.m->window.scale = SDL_GetWindowPixelDensity(mS.window);
                #ifdef _WIN32
                    if(!mS.m->toolbar.tabletOptions.ignoreMouseMovementWhenPenInProximity || !mS.m->input.pen.inProximity)
                #endif
                    {
					    mS.m->input.mouse.set_pos({event->motion.x * mS.m->window.scale, event->motion.y * mS.m->window.scale});
                    }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if(event->button.button == 1)
                    mS.m->input.mouse.leftDown = false;
                else if(event->button.button == 2)
                    mS.m->input.mouse.middleDown = false;
                else if(event->button.button == 3)
                    mS.m->input.mouse.rightDown = false;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if(event->button.button == 1) {
                    mS.m->input.mouse.leftDown = true;
                    mS.m->input.mouse.leftClicks = event->button.clicks;
                }
                else if(event->button.button == 2) {
                    mS.m->input.mouse.middleDown = true;
                    mS.m->input.mouse.middleClicks = event->button.clicks;
                }
                else if(event->button.button == 3) {
                    mS.m->input.mouse.rightDown = true;
                    mS.m->input.mouse.rightClicks = event->button.clicks;
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                mS.m->input.mouse.scrollAmount.x() += event->wheel.x;
                mS.m->input.mouse.scrollAmount.y() += event->wheel.y;
                break;
            case SDL_EVENT_KEY_DOWN:
               mS.m->input.backend_key_down_update(event->key);
               break;
            case SDL_EVENT_KEY_UP:
                mS.m->input.backend_key_up_update(event->key);
                break;
            case SDL_EVENT_TEXT_INPUT:
                mS.m->input.text.newInput += event->text.text;
                mS.m->input.text.add_text_to_textbox(event->text.text);
                break;
            case SDL_EVENT_DROP_FILE:
                mS.m->window.scale = SDL_GetWindowPixelDensity(mS.window);
                mS.m->input.droppedItems.emplace_back(InputManager::DroppedItem{
                    true,
                    Vector2f{event->drop.x, event->drop.y} * mS.m->window.scale,
                    event->drop.data
                });
                break;
            case SDL_EVENT_DROP_TEXT:
                mS.m->window.scale = SDL_GetWindowPixelDensity(mS.window);
                mS.m->input.droppedItems.emplace_back(InputManager::DroppedItem{
                    false,
                    Vector2f{event->drop.x, event->drop.y} * mS.m->window.scale,
                    event->drop.data
                });
                break;
            case SDL_EVENT_PEN_PROXIMITY_IN: {
                mS.m->input.pen.inProximity = true;
                break;
            }
            case SDL_EVENT_PEN_PROXIMITY_OUT: {
                mS.m->input.pen.inProximity = false;
                break;
            }
            case SDL_EVENT_PEN_MOTION: {
                mS.m->window.scale = SDL_GetWindowPixelDensity(mS.window);
                mS.m->input.pen.isEraser = event->pmotion.pen_state & SDL_PEN_INPUT_ERASER_TIP;
				mS.m->input.mouse.set_pos({event->pmotion.x * mS.m->window.scale, event->pmotion.y * mS.m->window.scale});
                break;
            }
            case SDL_EVENT_PEN_DOWN: {
                mS.m->input.pen.isDown = true;
                mS.m->input.mouse.leftDown = true;
                if((std::chrono::steady_clock::now() - mS.m->input.pen.lastPenLeftClickTime) > std::chrono::milliseconds(300))
                    mS.m->input.pen.leftClicksSaved = 0;
                mS.m->input.pen.leftClicksSaved++;
                mS.m->input.mouse.leftClicks = mS.m->input.pen.leftClicksSaved;
                mS.m->input.pen.lastPenLeftClickTime = std::chrono::steady_clock::now();
                mS.m->input.pen.isEraser = event->ptouch.eraser;
                //mS.m->input.mouse.set_pos({event->ptouch.x, event->ptouch.y});
                break;
            }
            case SDL_EVENT_PEN_AXIS: {
                if(event->paxis.axis == SDL_PEN_AXIS_PRESSURE)
                    mS.m->input.pen.pressure = event->paxis.value;
                //mS.m->input.mouse.set_pos({event->paxis.x, event->paxis.y});
                break;
            }
            case SDL_EVENT_PEN_UP: {
                mS.m->input.pen.isDown = false;
                mS.m->input.mouse.leftDown = false;
                mS.m->input.pen.isEraser = event->ptouch.eraser;
                mS.m->input.pen.pressure = 0.0;
                //mS.m->input.mouse.set_pos({event->ptouch.x, event->ptouch.y});
                break;
            }
            case SDL_EVENT_PEN_BUTTON_UP: {
                mS.m->input.set_pen_button_up(event->pbutton);
                //mS.m->input.mouse.set_pos({event->pbutton.x, event->pbutton.y});
                break;
            }
            case SDL_EVENT_PEN_BUTTON_DOWN: {
                mS.m->input.set_pen_button_down(event->pbutton);
                //mS.m->input.mouse.set_pos({event->pbutton.x, event->pbutton.y});
                break;
            }
            default:
                break;
        }
#ifdef NDEBUG
    }
    catch(const std::exception& e) {
        Logger::get().log("FATAL", e.what());
        return SDL_APP_FAILURE;
    }
#endif

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    MainStruct& mS = *((MainStruct*)appstate);
    try {
        mS.m->save_config();
        mS.m->early_destroy();
        sdl_terminate(mS);
    }
    catch(const std::exception& e) {
        Logger::get().log("FATAL", e.what());
    }

    delete (&mS);

    FileDownloader::cleanup();

    SDL_Quit();
}
