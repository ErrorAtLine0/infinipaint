cmake_minimum_required(VERSION 3.27.0)
set(CMAKE_CXX_STANDARD 23)
project(infinipaint)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options to set
set(availableGraphicsBackends OpenGL3.3 OpenGL2.1 OpenGLES3.0 Vulkan)
if(WIN32)
    set(GRAPHICS_BACKEND Vulkan CACHE STRING "Graphics backend to use")
else()
    set(GRAPHICS_BACKEND OpenGL3.3 CACHE STRING "Graphics backend to use")
endif()

set_property(CACHE GRAPHICS_BACKEND PROPERTY STRINGS ${availableGraphicsBackends})
list(FIND availableGraphicsBackends ${GRAPHICS_BACKEND} graphicsBackendIndex)
if(graphicsBackendIndex EQUAL -1)
    message(FATAL_ERROR "Graphics backend must be one of: ${availableGraphicsBackends}")
endif()

option(USE_GRAPHITE "Use Skia Graphite Backend (only works for Vulkan graphics backend)" OFF)

option(USE_ASAN "Enable Address Sanitizer" OFF)
option(USE_UBSAN "Enable Undefined Behaviour Sanitizer" OFF)
option(USE_LEAKSAN "Enable Leak Sanitizer" OFF)
option(USE_THREADSAN "Enable Thread Sanitizer" OFF)
option(USE_MEMORYSAN "Enable Memory Sanitizer" OFF)

option(NO_CONAN_BUILD "Not using conan to get packages" OFF)
option(EMSCRIPTEN_SINGLE_HTML_FILE "Building with emscripten generates a single HTML file" OFF)
option(CONFIG_NEXT_TO_EXECUTABLE "Place configuration folder next to executable (portable executable)" OFF)
option(MACOS_MAKE_BUNDLE "Package the application as a bundle" ON)

# Setting sources
set(sources "src/main.cpp"
            "src/SwitchCWD.cpp"
            "src/MainProgram.cpp"
            "src/DrawData.cpp"
            "src/FontData.cpp"
            "src/ClientData.cpp"
            "src/GridManager.cpp"
            "src/WorldGrid.cpp"
            "src/TimePoint.cpp"
            "src/InputManager.cpp"
            "src/UndoManager.cpp"
            "src/CanvasTheme.cpp"
            "src/DrawCamera.cpp"
            "src/DrawCollision.cpp"
            "src/CoordSpaceHelper.cpp"
            "src/CoordSpaceHelperTransform.cpp"
            "src/ResourceManager.cpp"
            "src/Toolbar.cpp"
            "src/World.cpp"
            "src/FileHelpers.cpp"
            "src/VersionConstants.cpp"
            "src/ScaleUpCanvas.cpp"
            "src/WorldUndoManager.cpp"
            "src/Bookmarks/BookmarkListItem.cpp"
            "src/Bookmarks/BookmarkManager.cpp"
            "src/ResourceDisplay/ResourceDisplay.cpp"
            "src/ResourceDisplay/SvgResourceDisplay.cpp"
            "src/ResourceDisplay/ImageResourceDisplay.cpp"
            "src/ResourceDisplay/FileResourceDisplay.cpp"
            "src/RichText/TextBox.cpp"
            "src/RichText/TextStyleModifier.cpp"
            "src/RichText/ParagraphStyleData.cpp"
            "src/DrawingProgram/ToolConfiguration.cpp"
            "src/DrawingProgram/DrawingProgramSelection.cpp"
            "src/DrawingProgram/DrawingProgramCache.cpp"
            "src/DrawingProgram/DrawingProgram.cpp"
            "src/DrawingProgram/Tools/BrushTool.cpp"
            "src/DrawingProgram/Tools/LineDrawTool.cpp"
            "src/DrawingProgram/Tools/TextBoxTool.cpp"
            "src/DrawingProgram/Tools/EraserTool.cpp"
            "src/DrawingProgram/Tools/RectSelectTool.cpp"
            "src/DrawingProgram/Tools/LassoSelectTool.cpp"
            "src/DrawingProgram/Tools/RectDrawTool.cpp"
            "src/DrawingProgram/Tools/EllipseDrawTool.cpp"
            "src/DrawingProgram/Tools/EyeDropperTool.cpp"
            "src/DrawingProgram/Tools/ScreenshotTool.cpp"
            "src/DrawingProgram/Tools/EditTool.cpp"
            "src/DrawingProgram/Tools/ZoomCanvasTool.cpp"
            "src/DrawingProgram/Tools/PanCanvasTool.cpp"
            "src/DrawingProgram/Tools/GridModifyTool.cpp"
            "src/DrawingProgram/Tools/DrawingProgramToolBase.cpp"
            "src/DrawingProgram/Tools/EditTools/DrawingProgramEditToolBase.cpp"
            "src/DrawingProgram/Tools/EditTools/ImageEditTool.cpp"
            "src/DrawingProgram/Tools/EditTools/EllipseDrawEditTool.cpp"
            "src/DrawingProgram/Tools/EditTools/RectDrawEditTool.cpp"
            "src/DrawingProgram/Tools/EditTools/TextBoxEditTool.cpp"
            "src/DrawingProgram/Tools/EditTools/BrushEditTool.cpp"
            "src/DrawingProgram/Layers/DrawingProgramLayerManagerGUI.cpp"
            "src/DrawingProgram/Layers/DrawingProgramLayerManager.cpp"
            "src/DrawingProgram/Layers/DrawingProgramLayerListItem.cpp"
            "src/DrawingProgram/Layers/DrawingProgramLayerFolder.cpp"
            "src/DrawingProgram/Layers/DrawingProgramLayer.cpp"
            "src/DrawingProgram/Layers/SerializedBlendMode.cpp"
            "src/CanvasComponents/CanvasComponentContainer.cpp"
            "src/CanvasComponents/CanvasComponentAllocator.cpp"
            "src/CanvasComponents/BrushStrokeCanvasComponent.cpp"
            "src/CanvasComponents/RectangleCanvasComponent.cpp"
            "src/CanvasComponents/TextBoxCanvasComponent.cpp"
            "src/CanvasComponents/EllipseCanvasComponent.cpp"
            "src/CanvasComponents/ImageCanvasComponent.cpp"
            "src/CanvasComponents/CanvasComponent.cpp"
            "src/GUIStuff/GUIManager.cpp"
            "src/GUIStuff/GUIManagerID.cpp"
            "src/GUIStuff/ClayImplement.cpp"
            "src/GUIStuff/Elements/Element.cpp"
            "src/GUIStuff/Elements/SVGIcon.cpp"
            "src/GUIStuff/Elements/RadioButton.cpp"
            "src/GUIStuff/Elements/CheckBox.cpp"
            "src/GUIStuff/Elements/SelectableButton.cpp"
            "src/GUIStuff/Elements/MovableTabList.cpp"
            "src/GUIStuff/Elements/PaintCircleMenu.cpp"
            "src/GUIStuff/Elements/RotateWheel.cpp"
            "src/GUIStuff/Elements/TextParagraph.cpp"
            "src/GUIStuff/Elements/FontPicker.cpp"
            "src/GUIStuff/Elements/TreeListing.cpp"
            "include/sago/platform_folders.cpp"
            "include/Helpers/Networking/NetLibrary.cpp"
            "include/Helpers/Networking/NetServer.cpp"
            "include/Helpers/Networking/NetClient.cpp"
            "include/Helpers/Networking/ByteStream.cpp"
            "include/Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.cpp"
            "include/Helpers/NetworkingObjects/NetObjManagerTypeList.cpp"
            "include/Helpers/NetworkingObjects/NetObjManager.cpp"
            "include/Helpers/NetworkingObjects/NetObjID.cpp"
            "include/Helpers/Hashes.cpp"
            "include/Helpers/BezierEasing.cpp"
            "include/Helpers/Random.cpp"
            "include/Helpers/Logger.cpp"
            "include/Helpers/StringHelpers.cpp"
            "include/Helpers/Serializers.cpp"
            "include/Helpers/FileDownloader.cpp"
            "include/Helpers/FileDownloaderEmscripten.cpp"
            "include/Helpers/VersionNumber.cpp"
        )

if((NOT EMSCRIPTEN) AND (GRAPHICS_BACKEND STREQUAL "Vulkan"))
    list(APPEND sources
        "src/VulkanContext/VkTestMemoryAllocator.cpp"
        "src/VulkanContext/VkTestUtils.cpp"
        "src/VulkanContext/WindowContext.cpp"
        "src/VulkanContext/VulkanWindowContext.cpp"
    )
    if(USE_GRAPHITE)
        list(APPEND sources
            "src/VulkanContext/GraphiteNativeVulkanWindowContext.cpp"
            "src/VulkanContext/GraphiteToolUtils.cpp"
        )
    endif()
endif()

if(WIN32)
    list(APPEND sources 
        "src/WindowsFontData/CustomFontSetManager.cpp"
        "windowsinstall/infinipaint.manifest"
        "windowsinstall/app.rc")
endif()

# Get libraries
find_package(ICU 77.1 REQUIRED COMPONENTS uc)

if(NO_CONAN_BUILD)
    set(SKIA_INCLUDE "modules/skia" CACHE PATH "Path to Skia root directory")
    set(SKIA_LIB_PATH "modules/skia/out/Static" CACHE PATH "Path to Skia libs")
    find_library(SKIA_LIB skia PATHS ${SKIA_LIB_PATH})
    find_library(SKIA_SVG_LIB svg PATHS ${SKIA_LIB_PATH})
    find_library(SKIA_RESOURCES_LIB skresources PATHS ${SKIA_LIB_PATH})
    find_library(SKIA_PARAGRAPH_LIB skparagraph PATHS ${SKIA_LIB_PATH})
    find_library(SKIA_SHAPER_LIB skshaper PATHS ${SKIA_LIB_PATH})
    find_library(SKIA_UNICODE_ICU_LIB skunicode_icu PATHS ${SKIA_LIB_PATH})
    find_library(SKIA_UNICODE_CORE_LIB skunicode_core PATHS ${SKIA_LIB_PATH})

    find_package(Freetype REQUIRED)
    find_package(Fontconfig REQUIRED)
    find_package(PNG REQUIRED)
    find_package(ZLIB REQUIRED)
else()
    find_package(skia-infinipaint REQUIRED)
endif()

if(NOT EMSCRIPTEN)
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
    find_package(LibDataChannel REQUIRED)
    find_package(CURL REQUIRED)
    find_package(harfbuzz REQUIRED)
else()
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-static)
endif()

if((NOT EMSCRIPTEN) AND (NOT APPLE))
    find_package(TBB REQUIRED)
endif()

find_package(zstd REQUIRED)

message("Skia root: " ${SKIA_INCLUDE})
message("Build type: " ${CMAKE_BUILD_TYPE})

add_executable(infinipaint ${sources})

if(WIN32)
    set_target_properties(infinipaint PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    if(MACOS_MAKE_BUNDLE)
        target_compile_definitions(infinipaint PRIVATE MACOS_MAKE_BUNDLE)
        set_target_properties(infinipaint PROPERTIES MACOSX_BUNDLE TRUE)
        set_target_properties(infinipaint PROPERTIES
            BUNDLE True
            MACOSX_BUNDLE_GUI_IDENTIFIER com.erroratline0.infinipaint
            MACOSX_BUNDLE_BUNDLE_NAME infinipaint
            MACOSX_BUNDLE_BUNDLE_VERSION "0.4.1"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.4.1"
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/macosinstall/Info.plist
        )
        add_custom_command(TARGET infinipaint POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:infinipaint>/../Resources/data"
        )
        add_custom_command(TARGET infinipaint POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/macosinstall/appicon.icns"
            "$<TARGET_FILE_DIR:infinipaint>/../Resources/appicon.icns"
        )
    endif()
endif()

if(EMSCRIPTEN)
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        target_compile_options(infinipaint PRIVATE -g3)
        target_link_options(infinipaint PRIVATE -g3)
    else()
        target_compile_options(infinipaint PRIVATE -O3)
        target_link_options(infinipaint PRIVATE -O3)
    endif()

    target_compile_options(infinipaint PRIVATE -Wall -sUSE_HARFBUZZ=1 -fdiagnostics-color -sUSE_SDL=0 -include functional -fexceptions)
    target_link_options(infinipaint PRIVATE --emrun -sUSE_HARFBUZZ=1 -sUSE_SDL=0 -sFULL_ES2 -sUSE_WEBGL2=1 -sFETCH --embed-file data -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=ccall -sEXPORTED_FUNCTIONS=[_main,_malloc,_free] -sMAXIMUM_MEMORY=4GB -sSINGLE_FILE -lidbfs.js -sINITIAL_MEMORY=67108864 -fexceptions)
    if(EMSCRIPTEN_SINGLE_HTML_FILE)
        target_link_options(infinipaint PRIVATE --shell-file emscripteninstall/singlefilereleaseshell.html)
        set(CMAKE_EXECUTABLE_SUFFIX ".html")
    else()
        set(CMAKE_EXECUTABLE_SUFFIX ".js")
    endif()
    message("In Emscripten Mode")
endif()

if(USE_ASAN)
    target_compile_options(infinipaint PRIVATE -fsanitize=address)
    target_link_options(infinipaint PRIVATE -fsanitize=address)
    message("Using ASAN")
endif()
if(USE_UBSAN)
    target_compile_options(infinipaint PRIVATE -fsanitize=undefined)
    target_link_options(infinipaint PRIVATE -fsanitize=undefined)
    message("Using UBSan")
endif()
if(USE_LEAKSAN)
    target_compile_options(infinipaint PRIVATE -fsanitize=leak)
    target_link_options(infinipaint PRIVATE -fsanitize=leak)
    message("Using LeakSan")
endif()
if(USE_THREADSAN)
    target_compile_options(infinipaint PRIVATE -fsanitize=thread)
    target_link_options(infinipaint PRIVATE -fsanitize=thread)
    message("Using ThreadSan")
endif()
if(USE_MEMORYSAN)
    target_compile_options(infinipaint PRIVATE -fsanitize=memory)
    target_link_options(infinipaint PRIVATE -fsanitize=memory)
    message("Using MemSan")
endif()

if(NOT WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        target_compile_options(infinipaint PRIVATE -g -Wall -fdiagnostics-color)
        message("In Debug Mode")
    else()
        target_compile_options(infinipaint PRIVATE -O3 -Wall -fdiagnostics-color)
        target_link_options(infinipaint PRIVATE -O3) # Other things to consider: -ffast-math and -march=native
        message("In Release Mode")
    endif()
endif()

target_include_directories(infinipaint PRIVATE "include")

target_compile_definitions(infinipaint PRIVATE IS_CLIENT)
if(WIN32)
    target_compile_definitions(infinipaint PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if(CONFIG_NEXT_TO_EXECUTABLE)
    target_compile_definitions(infinipaint PRIVATE CONFIG_NEXT_TO_EXECUTABLE)
    message("Place config next to executable")
else()
    message("Place config in global config folder")
endif()

if(NOT EMSCRIPTEN AND ((GRAPHICS_BACKEND STREQUAL "OpenGL3.3") OR (GRAPHICS_BACKEND STREQUAL "OpenGLES3.0") OR (GRAPHICS_BACKEND STREQUAL "OpenGL2.1")))
    find_package(OpenGL REQUIRED)
    if(GRAPHICS_BACKEND STREQUAL "OpenGLES3.0")
        target_link_libraries(infinipaint OpenGL::GLES3)
    else()
        target_link_libraries(infinipaint OpenGL::GL)
    endif()
endif()

if((NOT EMSCRIPTEN) AND (NOT APPLE))
    if(NO_CONAN_BUILD)
        target_link_libraries(infinipaint TBB::tbb)
    else()
        target_link_libraries(infinipaint onetbb::onetbb)
    endif()
endif()

target_link_libraries(infinipaint zstd::libzstd_static)

if((NOT EMSCRIPTEN) AND WIN32)
    target_link_libraries(infinipaint dwrite)
    # Clip library for retrieving images from windows clipboard
    set(CLIP_EXAMPLES OFF CACHE BOOL "Compile clip examples")
    set(CLIP_TESTS OFF CACHE BOOL "Compile clip tests")
    set(CLIP_ENABLE_IMAGE ON CACHE BOOL "Compile with image clipboard support")
    add_subdirectory(deps/clip)
    target_link_libraries(infinipaint clip)
endif()

if(EMSCRIPTEN)
    add_subdirectory(deps/datachannel-wasm EXCLUDE_FROM_ALL)
    target_link_libraries(infinipaint datachannel-wasm)
else()
    target_link_libraries(infinipaint LibDataChannel::LibDataChannel CURL::libcurl)
endif()

if(NO_CONAN_BUILD)
    target_link_libraries(infinipaint ${SKIA_SVG_LIB} ${SKIA_RESOURCES_LIB} ${SKIA_LIB} ${SKIA_UNICODE_CORE_LIB} ${SKIA_UNICODE_ICU_LIB} ${SKIA_UNICODE_CORE_LIB} ${SKIA_PARAGRAPH_LIB} ${SKIA_SHAPER_LIB})
    target_include_directories(infinipaint PRIVATE ${SKIA_INCLUDE})
    target_link_libraries(infinipaint SDL3::SDL3 ICU::uc Fontconfig::Fontconfig Freetype::Freetype PNG::PNG ZLIB::ZLIB)
else()
    target_link_libraries(infinipaint skia-infinipaint::skia-infinipaint sdl::sdl icu-infinipaint::icu-infinipaint)
endif()

if(NOT EMSCRIPTEN)
    target_link_libraries(infinipaint harfbuzz::harfbuzz)
endif()

if((NOT (GRAPHICS_BACKEND STREQUAL "Vulkan")) AND (NOT EMSCRIPTEN) AND LINUX)
    target_link_libraries(infinipaint OpenGL::EGL)
endif()

if(GRAPHICS_BACKEND STREQUAL "Vulkan")
    target_compile_definitions(infinipaint PRIVATE USE_BACKEND_VULKAN SK_VULKAN)
    if(USE_GRAPHITE)
        target_compile_definitions(infinipaint PRIVATE SK_GRAPHITE USE_SKIA_BACKEND_GRAPHITE)
        message("Using Vulkan Graphite Backend")
    else()
        target_compile_definitions(infinipaint PRIVATE USE_SKIA_BACKEND_GANESH)
        message("Using Vulkan Ganesh Backend")
    endif()
else()
    if(GRAPHICS_BACKEND STREQUAL "OpenGLES3.0")
        target_compile_definitions(infinipaint PRIVATE USE_BACKEND_OPENGLES_3_0)
        message("Using OpenGL ES 3.0")
    elseif(GRAPHICS_BACKEND STREQUAL "OpenGL2.1")
        target_compile_definitions(infinipaint PRIVATE USE_BACKEND_OPENGL_2_1)
        message("Using OpenGL 2.1")
    else()
        target_compile_definitions(infinipaint PRIVATE USE_BACKEND_OPENGL_3_3)
        message("Using OpenGL 3.3")
    endif()
    target_compile_definitions(infinipaint PRIVATE USE_BACKEND_OPENGL USE_SKIA_BACKEND_GANESH)
endif()

target_compile_definitions(infinipaint PRIVATE BOOST_MP_STANDALONE EIGEN_MPL2_ONLY)

include(CheckCCompilerFlag)
check_c_compiler_flag(-Wdeprecated-declarations HAS_DEPRECATED_DECLARATIONS)

if(HAS_DEPRECATED_DECLARATIONS)
    target_compile_options(infinipaint PRIVATE -Wno-deprecated-declarations)
endif()

if(APPLE OR WIN32)
    install(TARGETS infinipaint DESTINATION .)

    set(CPACK_PACKAGE_NAME "infinipaint")
    set(CPACK_PACKAGE_VERSION_MAJOR "0")
    set(CPACK_PACKAGE_VERSION_MINOR "4")
    set(CPACK_PACKAGE_VERSION_PATCH "1")
    set(CPACK_PACKAGE_VENDOR "ErrorAtLine0")
    set(CPACK_PACKAGE_CONTACT "erroratline0@gmail.com")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()

if(APPLE)
    # Nothing specific here for now
elseif(WIN32)
    install(DIRECTORY data DESTINATION .)

    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
	include(InstallRequiredSystemLibraries)
	install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION .)

    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DISPLAY_NAME "InfiniPaint")
    set(CPACK_NSIS_PACKAGE_NAME "InfiniPaint")
    set(CPACK_NSIS_UNINSTALL_NAME "InfiniPaint Uninstall")
    set(CPACK_NSIS_WELCOME_TITLE "Welcome to InfiniPaint Installer")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
    set(CPACK_NSIS_URL_INFO_ABOUT "https://infinipaint.com")
    set(CPACK_NSIS_MENU_LINKS "infinipaint.exe" "InfiniPaint")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "infinipaint.exe")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/windowsinstall/icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/windowsinstall/icon.ico")
    # A backslash character uses 4 backslashes, since we have to escape twice
    # Double quote uses 3 backslashes, since we have to escape the backslash (two slashes), and escape the double quote (one slash)
	set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
	     WriteRegStr HKCR '.infpnt' '' 'InfiniPaintCanvas'
         WriteRegStr HKCR 'InfiniPaintCanvas' '' 'InfiniPaint Canvas File'
         WriteRegStr HKCR 'InfiniPaintCanvas\\\\shell' '' 'open'
         WriteRegStr HKCR 'InfiniPaintCanvas\\\\DefaultIcon' '' '\\\"$INSTDIR\\\\infinipaint.exe\\\",0'
         WriteRegStr HKCR 'InfiniPaintCanvas\\\\shell\\\\open\\\\command' '' '\\\"$INSTDIR\\\\infinipaint.exe\\\" \\\"%1\\\"'
         WriteRegStr HKCR 'InfiniPaintCanvas\\\\shell\\\\edit' '' 'Edit InfiniPaint Canvas File'
         WriteRegStr HKCR 'InfiniPaintCanvas\\\\shell\\\\edit\\\\command' '' '\\\"$INSTDIR\\\\infinipaint.exe\\\" \\\"%1\\\"'
         System::Call 'Shell32::SHChangeNotify(i 0x8000000, i 0, i 0, i 0)'
	")
	
	set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
	     DeleteRegKey HKCR '.infpnt'
         DeleteRegKey HKCR 'InfiniPaintCanvas'
     ")

     set(CPACK_PACKAGE_INSTALL_DIRECTORY "infinipaint")
else()
    install(DIRECTORY data DESTINATION bin)
    install(TARGETS infinipaint DESTINATION bin)
    install(FILES linuxinstall/com.infinipaint.infinipaint.desktop DESTINATION share/applications)
    install(FILES logo.svg DESTINATION share/icons/hicolor/scalable/apps RENAME com.infinipaint.infinipaint.svg)
    install(FILES logo.svg DESTINATION share/icons/hicolor/scalable/mimetypes RENAME com.infinipaint.infinipaint-application-infpnt.svg)
    install(FILES linuxinstall/com.infinipaint.infinipaint.metainfo.xml DESTINATION share/metainfo)
    install(FILES linuxinstall/com.infinipaint.infinipaint.xml DESTINATION share/mime/packages)
endif()

if(APPLE OR WIN32)
    include(CPack)
endif()
