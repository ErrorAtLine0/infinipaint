from conan import ConanFile
from conan.tools.cmake import cmake_layout

class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        if self.settings.os == "Emscripten":
            self.requires("skia/134.20250327.0", options = {
                "enable_svg": True,
                "use_expat": True,
                "use_harfbuzz": True,
                "use_system_harfbuzz":False,
                "use_conan_harfbuzz":True,
                "enable_skshaper": True
            })
        else:
            self.requires("skia/134.20250327.0", options = {
                "use_system_expat":False,
                "use_system_freetype":False,
                "use_system_harfbuzz":False,
                "use_system_icu":False,
                "use_system_libjpeg_turbo":False,
                "use_system_libpng":False,
                "use_system_libwebp":False,
                "use_system_zlib":False,
                "enable_svg": True,
                "enable_skottie": False,
                "enable_skunicode": True,
                "enable_skshaper": True,
                "enable_bentleyottmann": True, # for some reason, setting this to False results in an error when creating the project
                "enable_skparagraph": True,
                "use_freetype": True
            })

        
        if self.settings.os == "Linux":
            self.requires("sdl/3.2.20", options = {
                "wayland": False,
                "pulseaudio": False,
                "alsa": False,
                "sndio": False,
                "vulkan": False,
                "opengles": False
            })
        else:
            self.requires("sdl/3.2.20")

        if self.settings.os != "Emscripten" and self.settings.os != "Macos":
            self.requires("onetbb/2022.0.0")

        if self.settings.os != "Emscripten":
            self.requires("libdatachannel/0.23.1")
            self.requires("libcurl/8.15.0")
            
        self.requires("zstd/1.5.7")
        
    def build_requirements(self):
        self.tool_requires("cmake/3.27.0")

    def layout(self):
        cmake_layout(self)
