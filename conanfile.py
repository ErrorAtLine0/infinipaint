from conan import ConanFile
from conan.tools.cmake import cmake_layout

class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        if self.settings.os == "Emscripten":
            self.requires("skia/143.20251028.0", options = {
                "enable_svg": True,
                "use_freetype": True,
                "use_system_freetype": True,
                "use_expat": True,
                "use_harfbuzz": True,
                "use_system_harfbuzz": True,
                "use_conan_harfbuzz": False,
                "use_system_icu": True,
                "use_conan_icu": True,
                "enable_skshaper": True,
                "canvaskit_enable_alias_font": False,
                "canvaskit_enable_canvas_bindings": False,
                "canvaskit_enable_effects_deserialization": False,
                "canvaskit_enable_embedded_font": False,
                "canvaskit_enable_font": False,
                "canvaskit_enable_matrix_helper": False,
                "canvaskit_enable_pathops": False,
                "canvaskit_enable_rt_shader": False,
                "canvaskit_enable_skp_serialization": False,
                "canvaskit_enable_sksl_trace": False,
                "canvaskit_enable_webgpu": False,
                "canvaskit_enable_webgl": False,
                "use_system_libwebp": False, # There's a problem compiling libwebp with emscripten in conan
                "use_conan_libwebp": False
            })
        else:
            self.requires("skia/143.20251028.0", options = {
                "use_system_expat": False,
                "use_freetype": True,
                "use_system_freetype": True,
                "use_conan_freetype": True,
                "use_system_harfbuzz": True,
                "use_conan_harfbuzz": True,
                "use_system_icu": True,
                "use_conan_icu": True,
                "use_system_libjpeg_turbo": False,
                "use_system_libpng": False,
                "use_system_libwebp": False,
                "use_system_zlib": False,
                "use_x11": False,
                "use_egl": True,
                "enable_svg": True,
                "enable_skottie": False,
                "enable_bentleyottmann": True # for some reason, setting this to False results in an error when creating the project
            })

        
        if self.settings.os == "Linux":
            self.requires("sdl/3.2.26", options = {
                "wayland": False,
                "x11": True,
                "pulseaudio": False,
                "alsa": False,
                "sndio": False,
                "vulkan": False,
                "opengles": False
            })
        else:
            self.requires("sdl/3.2.26")

        if self.settings.os != "Emscripten" and self.settings.os != "Macos":
            self.requires("onetbb/2022.0.0")

        if self.settings.os != "Emscripten":
            self.requires("libdatachannel/0.23.1")
            self.requires("libcurl/8.16.0")
            
        self.requires("zstd/1.5.7")
        self.requires("icu/77.1")

    def build_requirements(self):
        self.tool_requires("cmake/3.27.0")

    def layout(self):
        cmake_layout(self)
