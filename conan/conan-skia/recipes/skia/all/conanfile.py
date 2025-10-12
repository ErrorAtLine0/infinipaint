from conan import ConanFile
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, copy, rename, mkdir, rmdir, save, replace_in_file
from conan.tools.build.flags import cppstd_flag
from conan.tools.build import check_min_cppstd
from conan.tools.microsoft import is_msvc
from conan.tools.microsoft.visual import msvc_runtime_flag
from conan.tools.apple import is_apple_os, XCRun
from conan.tools.gnu import AutotoolsToolchain
from conan.tools.env import VirtualBuildEnv
from conan.errors import ConanInvalidConfiguration

from os.path import join

import os
import re
import json

class ConanSkia(ConanFile):
    name = "skia"
    package_type = "library"

    # Optional metadata
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of skia package here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    _skia_options = {
        # configurable dependencies 
        "use_expat" : [True, False],
        "use_freetype" : [True, False],
        "use_harfbuzz" : [True, False],
        "use_icu" : [True, False],
        "use_libjpeg_turbo_encode" : [True, False],
        "use_libjpeg_turbo_decode" : [True, False],
        "use_libpng_encode" : [True, False],
        "use_libpng_decode" : [True, False],
        "use_libwebp_encode" : [True, False],
        "use_libwebp_decode" : [True, False],
        "use_zlib" : [True, False],
        # system library options 
        "use_system_expat" : [True, False],
        "use_system_freetype": [True, False],
        "use_system_harfbuzz" : [True, False],
        "use_system_icu" :[True, False],
        "use_system_libjpeg_turbo" : [True, False],
        "use_system_libpng" : [True, False],
        "use_system_libwebp" : [True, False],
        "use_system_zlib" : [True, False],
        # conan library options 
        "use_conan_expat" : [True, False],
        "use_conan_freetype": [True, False],
        "use_conan_fontconfig": [True, False],
        "use_conan_harfbuzz" : [True, False],
        "use_conan_icu" :[True, False],
        "use_conan_libjpeg_turbo" : [True, False],
        "use_conan_libpng" : [True, False],
        "use_conan_libwebp" : [True, False],
        "use_conan_zlib" : [True, False],
        # backend options
        "use_angle" : [True, False],
        "use_gl" : [True, False],
        "use_vulkan" : [True, False],
        "use_metal" : [True, False],
        "use_webgl" : [True, False],
        "use_webgpu" : [True, False],
        "use_dawn" : [True, False],
        "use_direct3d" :[True, False],
        "use_piex" : [True, False],
        "use_wuffs" : [True, False],
        "use_libavif" : [True, False],
        "use_libjxl_decode" : [True, False],
        "use_ndk_images" : [True, False],
        "use_x11" : [True, False],
        "use_xps" : [True, False],
        "use_dng_sdk" : [True, False],
        "use_fontconfig" : [True, False],
        "use_fonthost_mac" : [True, False],
        "use_perfetto" : [True, False],
        # backend toggles
        "enable_ganesh" : [True, False],
        "enable_graphite" : [True, False],
        "enable_pdf" : [True, False],
        "enable_winuwp" : [True, False],
        "enable_fontmgr_android" : [True, False],
        "enable_fontmgr_custom_directory" : [True, False],
        "enable_fontmgr_custom_embedded" : [True, False],
        "enable_fontmgr_custom_empty" : [True, False],
        "enable_fontmgr_fontconfig" : [True, False],
        "enable_fontmgr_win" : [True, False],
        "enable_fontmgr_win_gdi" : [True, False],
        "enable_fontmgr_FontConfigInterface" : [True, False],
        # modules
        "enable_svg" : [True, False],
        "enable_skottie" : [True, False],
        "enable_skunicode" : [True, False],
        "enable_skshaper" : [True, False],
        "enable_bentleyottmann" : [True, False],
        "enable_skparagraph" : [True, False],
        # canvaskit
        "canvaskit_enable_alias_font" :  [True, False],
        "canvaskit_enable_canvas_bindings" :  [True, False],
        "canvaskit_enable_effects_deserialization" :  [True, False],
        "canvaskit_enable_embedded_font" :  [True, False],
        "canvaskit_enable_font" :  [True, False],
        "canvaskit_enable_matrix_helper" :  [True, False],
        "canvaskit_enable_pathops" :  [True, False],
        "canvaskit_enable_rt_shader" :  [True, False],
        "canvaskit_enable_skp_serialization" :  [True, False],
        "canvaskit_enable_sksl_trace" :  [True, False],
        "canvaskit_enable_paragraph" :  [True, False],
        "canvaskit_enable_webgpu" : [True, False],
        "canvaskit_enable_webgl" : [True, False],
        # other options
        "enable_precompile" : [True, False],
        "enable_optimize_size" : [True, False],
        "enable_api_available_macro" : [True, False],
        "gl_standard" : ["gl", "gles", "webgl", ""],
    }

    _skia_default_options = {
        # configurable dependencies 
        "use_icu" : True,
        "use_libjpeg_turbo_encode" : True,
        "use_libjpeg_turbo_decode" : True,
        "use_libpng_encode" : True,
        "use_libpng_decode" : True,
        "use_libwebp_encode" : True,
        "use_libwebp_decode" : True,
        "use_zlib" : True,
        # system library options 
        "use_system_expat" : True,
        "use_system_freetype": True,
        "use_system_harfbuzz" : True,
        "use_system_icu" : True,
        "use_system_libjpeg_turbo" : True,
        "use_system_libpng" : True,
        "use_system_libwebp" : True,
        "use_system_zlib" : True,
        # conan library options 
        "use_conan_expat" : True,
        "use_conan_freetype": True,
        "use_conan_fontconfig": True,
        "use_conan_harfbuzz" : True,
        "use_conan_icu" : True,
        "use_conan_libjpeg_turbo" : True,
        "use_conan_libpng" : True,
        "use_conan_libwebp" : True,
        "use_conan_zlib" : True,
        # backend options
        "use_angle" : False,
        "use_gl" : True,
        "use_vulkan" : False,
        "use_metal" : False,
        "use_dawn" : False,
        "use_direct3d" : False,
        "use_xps" : True,
        "use_wuffs" : True,
        "use_libavif" : False,
        "use_libjxl_decode" : False,
        # backend toggles 
        "enable_ganesh" : True,
        "enable_graphite" : False,
        "enable_winuwp" : False,
        # modules
        "enable_skshaper" : True,
        "enable_bentleyottmann" : True,
        # canvaskit
        "canvaskit_enable_alias_font" : True,
        "canvaskit_enable_canvas_bindings" : True,
        "canvaskit_enable_effects_deserialization" : True,
        "canvaskit_enable_embedded_font" : True,
        "canvaskit_enable_font" : True,
        "canvaskit_enable_matrix_helper" : True,
        "canvaskit_enable_pathops" : True,
        "canvaskit_enable_rt_shader" : True,
        "canvaskit_enable_skp_serialization" : True,
        "canvaskit_enable_sksl_trace" : True,
        "canvaskit_enable_webgpu " : False,
        "canvaskit_enable_webgl " : False,
        # other options
        "enable_precompile" : True,
        "enable_optimize_size" : False,
        "enable_api_available_macro" : True,
    }

    options = {
        "shared" : [True, False],
        "fPIC" : [True, False],
    } | _skia_options

    default_options = {
        "shared": False,
        "fPIC": True,
    } | _skia_default_options

    def _is_ios_variant(self):
        os = self.settings.os
        return (os == "iOS" or os == "watchOS" or os == "tvOS" or os == "visionOS")

    def _is_ios_variant_simulator(self):
        os = self.settings.os
        if os == "iOS" and os.sdk == "iphonesimulator":
            return True
        elif os == "watchOS" and os.sdk == "watchsimulator":
            return True
        elif os == "tvOS" and os.sdk == "appletvsimulator":
            return True
        elif os == "visionOS" and os.sdk == "xrsimulator":
            return True
        return False

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")


    def validate(self):
        if self.settings.compiler.cppstd:
            check_min_cppstd(self, 17)

        if self.settings.arch == "x86":
            pass
        elif self.settings.arch == "x86_64":
            pass
        elif self.settings.arch == "wasm":
            pass
        elif re.match(r'armv7.*', self.settings.arch.value):
            pass
        elif re.match(r'armv8.*', self.settings.arch.value):
            pass
        else: 
            raise ConanInvalidConfiguration("Unsupported settings.arch")

    def configure(self):
        os = self.settings.os
        arch = self.settings.arch

        if self.options.shared:
            self.options.rm_safe("fPIC")

        if self.options.use_harfbuzz == None:
            # Disabled on Emscripten due to false-positive of HAVE_MPROTECT.
            # Will revisit once 9.0.0 is added to conancenter.
            enabled = (os != "Emscripten")
            self.options.use_harfbuzz = enabled

        if self.options.use_expat == None:
            enabled = (arch != "wasm")
            self.options.use_expat = enabled

        if self.options.use_freetype == None:
            enabled = (os == "Android" or os == "Linux" or arch == "wasm")
            self.options.use_freetype = enabled

        if self.options.use_webgl == None:
            enabled = (arch == "wasm")
            self.options.use_webgl = enabled

        if self.options.use_webgpu == None:
            enabled = (arch == "wasm")
            self.options.use_webgpu = enabled

        if self.options.use_x11 == None:
            enabled = (os == "Linux")
            self.options.use_x11 = enabled
        
        if self.options.use_fontconfig == None:
            enabled = (os == "Linux")
            self.options.use_fontconfig = enabled

        if self.options.use_fonthost_mac == None:
            enabled = (os == "Macos" or self._is_ios_variant())
            self.options.use_fonthost_mac = enabled

        if self.options.use_perfetto == None:
            enabled = (os == "Macos" or os == "Android" or os == "Linux")
            self.options.use_perfetto = enabled

        if self.options.enable_pdf == None:
            enabled = arch != "wasm"
            self.options.enable_pdf = enabled    

        if self.options.enable_fontmgr_android == None:
            enabled = self.options.use_freetype and self.options.use_expat
            self.options.enable_fontmgr_android = enabled

        if self.options.enable_fontmgr_custom_directory == None:
            enabled = self.options.use_freetype and arch != "wasm"
            self.options.enable_fontmgr_custom_directory = enabled

        if self.options.enable_fontmgr_custom_embedded == None:
            enabled = self.options.use_freetype
            self.options.enable_fontmgr_custom_embedded = enabled

        if self.options.enable_fontmgr_custom_empty == None:
            enabled = self.options.use_freetype
            self.options.enable_fontmgr_custom_empty = enabled

        if self.options.enable_fontmgr_fontconfig == None:
            enabled = self.options.use_freetype and self.options.use_fontconfig
            self.options.enable_fontmgr_fontconfig = enabled

        if self.options.enable_fontmgr_win == None:
            enabled = (os == "Windows") 
            self.options.enable_fontmgr_win = enabled

        if self.options.enable_fontmgr_win_gdi == None:
            enabled = (os == "Windows") and not self.options.enable_winuwp
            self.options.enable_fontmgr_win_gdi = enabled

        if self.options.enable_fontmgr_FontConfigInterface == None:
            enabled = self.options.use_freetype and self.options.use_fontconfig
            self.options.enable_fontmgr_FontConfigInterface = enabled

        if self.options.use_piex == None:
            enabled = os != "Windows" and arch != "wasm"
            self.options.use_piex = enabled

        if self.options.use_dng_sdk == None:
            enabled = arch != "wasm" and self.options.use_libjpeg_turbo_decode and self.options.use_zlib
            self.options.use_dng_sdk = enabled 

        if self.options.use_ndk_images == None:
            enabled = os == "Android" and int(os.api_level.value) >= 30
            self.options.use_ndk_images = enabled    

        if self.options.enable_svg == None:
            enabled = not self.options.shared
            self.options.enable_svg = enabled

        if self.options.enable_skottie == None:
            enabled = not self.options.shared
            self.options.enable_skottie = enabled

        if self.options.enable_skunicode == None:
            enabled = self.options.use_icu
            self.options.enable_skunicode = enabled

        if self.options.enable_skparagraph == None:
            enabled = self.options.enable_skshaper and self.options.enable_skunicode and self.options.use_harfbuzz
            self.options.enable_skparagraph = enabled

        if self.options.canvaskit_enable_paragraph == None:
            enabled = self.options.enable_skparagraph
            self.options.canvaskit_enable_paragraph = enabled

        if self.options.gl_standard == None:
            if os == "Macos":
                self.options.gl_standard = "gl"
            elif self._is_ios_variant():
                self.options.gl_standard = "gles"
            elif arch == "wasm":
                self.options.gl_standard = "webgl"
            else:
                self.options.gl_standard = ""

        # Remove unnecessary system library options.

        if self.options.use_expat == False:
            self.options.rm_safe("use_system_expat")
        elif not self.options.use_system_expat: 
            self.options.rm_safe("use_conan_expat")

        if self.options.use_freetype == False:
            self.options.rm_safe("use_system_freetype")
        elif not self.options.use_system_freetype: 
            self.options.rm_safe("use_conan_freetype")

        if self.options.use_icu == False:
            self.options.rm_safe("use_system_icu")
        elif not self.options.use_system_icu: 
            self.options.rm_safe("use_conan_icu")

        if self.options.use_harfbuzz == False:
            self.options.rm_safe("use_system_harfbuzz")
        elif not self.options.use_system_harfbuzz: 
            self.options.rm_safe("use_conan_harfbuzz")

        if self.options.use_libjpeg_turbo_encode == False and self.options.use_libjpeg_turbo_decode == False:
            self.options.rm_safe("use_system_libjpeg_turbo")
        elif not self.options.use_system_libjpeg_turbo: 
            self.options.rm_safe("use_conan_libjpeg_turbo")

        if self.options.use_libpng_encode == False and self.options.use_libpng_decode == False:
            self.options.rm_safe("use_system_libpng")
        elif not self.options.use_system_libpng: 
            self.options.rm_safe("use_conan_libpng")

        if self.options.use_libwebp_encode == False and self.options.use_libwebp_decode == False:
            self.options.rm_safe("use_system_libwebp")
        elif not self.options.use_system_libwebp: 
            self.options.rm_safe("use_conan_libwebp")

        if self.options.use_zlib == False:
            self.options.rm_safe("use_system_zlib")
        elif not self.options.use_system_zlib: 
            self.options.rm_safe("use_conan_zlib")

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], destination=self.source_folder, strip_root=True)
        apply_conandata_patches(self)

    def requirements(self):
        self.tool_requires("depot_tools/[>=20200407]")
        self.tool_requires("ninja/[>=1.11.0]")

        if self.options.use_harfbuzz and self.options.use_system_harfbuzz and self.options.use_conan_harfbuzz:
            self.requires("harfbuzz/10.4.0", options = {"with_subset":True, "with_glib":False})

        if self.options.use_freetype and self.options.use_system_freetype and self.options.use_conan_freetype:
            self.requires("freetype/2.13.2")

        if self.options.use_fontconfig and self.options.use_conan_fontconfig:
            self.requires("fontconfig/[>=2.14.2]")

        if self.options.use_expat and self.options.use_system_expat and self.options.use_conan_expat:
            self.requires("expat/[>=2.5.0]")

        if self.options.use_icu and self.options.use_system_icu and self.options.use_conan_icu:
            if self.settings.os == "Emscripten":
                self.requires("icu/[74.2]")
            else:
                self.requires("icu/[>=74.2]")

        if (self.options.use_libpng_decode or self.options.use_libpng_encode) and self.options.use_system_libpng and self.options.use_conan_libpng:
            self.requires("libpng/[>=1.6.32]")

        if (self.options.use_libjpeg_turbo_decode or self.options.use_libjpeg_turbo_encode) and self.options.use_system_libjpeg_turbo and self.options.use_conan_libjpeg_turbo:
            self.requires("libjpeg-turbo/[>=3.0.0]")

        if (self.options.use_libwebp_decode or self.options.use_libwebp_encode) and self.options.use_system_libwebp and self.options.use_conan_libwebp:
            self.requires("libwebp/[>=1.2.4]", options = {"swap_16bit_csp":True})

        if self.options.use_zlib and self.options.use_system_zlib and self.options.use_conan_zlib:
            self.requires("zlib/[>=1.2.11]")

    def layout(self):
        src_folder = "src"
        build_folder = src_folder  
        generators_folder = self.folders.generators or "conan"
        subproject = self.folders.subproject
        self.folders.source = src_folder if not subproject else join(subproject, src_folder)
        self.folders.build = build_folder if not subproject else join(subproject, build_folder)
        self.folders.generators = join(self.folders.build, generators_folder)

    def _get_lower_bool_str(self, cond):
        return "true" if cond else "false"

    def _collect_link_libs(self, dependency, components):
        libs = []
        libs += dependency.cpp_info.libs
        libs += dependency.cpp_info.system_libs

        # The order of components affects link order.
        # TODO: Sould we handle inter-component dependencies?
        for component in components:
            libs += dependency.cpp_info.components[component].libs
            libs += dependency.cpp_info.components[component].system_libs

        for transitiveDependency in dependency.dependencies.host.values():
            libs += transitiveDependency.cpp_info.libs
            libs += transitiveDependency.cpp_info.system_libs

            # TODO: Is there proper way to determine link order of those components?
            for dep in transitiveDependency.cpp_info.components.values():
                libs += dep.libs
                libs += dep.system_libs

        return libs

    def _collect_link_frameworks(self, dependency, components):
        frameworks = []
        frameworks += dependency.cpp_info.frameworks

        for component in components:
            frameworks += dependency.cpp_info.components[component].frameworks

        for transitiveDependency in dependency.dependencies.host.values():
            frameworks += transitiveDependency.cpp_info.frameworks

            for dep in transitiveDependency.cpp_info.components.values():
                frameworks += dep.frameworks

        return frameworks

    def _link_libs(self, name, components=[]):
        libs = []
        libs += self._collect_link_libs(self.dependencies[name], components)

        ext = ""
        if self.settings.os == "Windows":
            ext = ".lib"

        return [lib + ext for lib in libs]

    def _link_frameworks(self, name, components=[]):
        frameworks = self._collect_link_frameworks(self.dependencies[name], components)
        return [fw + ".framework" for fw in frameworks]

    def build(self):
        # activate-emsdk fails on Windows for some reason.
        os.environ["GIT_SYNC_DEPS_SKIP_EMSDK"] = "1"
        self.run("python3 tools/git-sync-deps")
        del os.environ["GIT_SYNC_DEPS_SKIP_EMSDK"]

        if self.options.use_expat and self.options.use_system_expat and self.options.use_conan_expat:
            replace_in_file(self, join(self.source_folder, "third_party", "expat", "BUILD.gn"),
                            "libs = [ \"expat\" ]", f"libs = {json.dumps(self._link_libs('expat'))}",
                            strict=False)

        if self.options.use_harfbuzz and self.options.use_system_harfbuzz and self.options.use_conan_harfbuzz:
            replace_in_file(self, join(self.source_folder, "third_party", "harfbuzz", "BUILD.gn"),
                            "libs = [ \"harfbuzz\" ]",
                            f"libs = {json.dumps(self._link_libs('harfbuzz'))}\n    " +
                            f"frameworks = {json.dumps(self._link_frameworks('harfbuzz'))}",
                            strict=False)
            replace_in_file(self, join(self.source_folder, "third_party", "harfbuzz", "BUILD.gn"),
                            "include_dirs = [ \"/usr/include/harfbuzz\" ]", "include_dirs = [ ]", 
                            strict=False)
            replace_in_file(self, join(self.source_folder, "third_party", "harfbuzz", "BUILD.gn"),
                            "libs += [ \"harfbuzz-subset\" ]", "libs += [ ]", 
                            strict=False)

        if self.options.use_freetype and self.options.use_system_freetype and self.options.use_conan_freetype:
            replace_in_file(self, join(self.source_folder, "third_party", "freetype2", "BUILD.gn"),
                            "libs = [ skia_system_freetype2_lib ]", f"libs = {json.dumps(self._link_libs('freetype'))}",
                            strict=False)
            replace_in_file(self, join(self.source_folder, "third_party", "freetype2", "BUILD.gn"),
                            "include_dirs = [ skia_system_freetype2_include_path ]", "include_dirs = [ ]", 
                            strict=False)

        if self.options.use_icu and self.options.use_system_icu and self.options.use_conan_icu:
            replace_in_file(self, join(self.source_folder, "third_party", "icu", "BUILD.gn"),
                            # icu-data must come after other components, otherwise it leaves an undefined symbol in shared library. 
                            "libs = [ \"icuuc\" ]", f"libs = {json.dumps(self._link_libs('icu', components=['icu-uc','icu-data']))}",
                            strict=False)

        if (self.options.use_libjpeg_turbo_encode or self.options.use_libjpeg_turbo_decode) and self.options.use_system_libjpeg_turbo and self.options.use_conan_libjpeg_turbo:
            replace_in_file(self, join(self.source_folder, "third_party", "libjpeg-turbo", "BUILD.gn"),
                            "libs = [ \"jpeg\" ]", f"libs = {json.dumps(self._link_libs('libjpeg-turbo', components=['jpeg']))}",
                            strict=False)

        if (self.options.use_libpng_encode or self.options.use_libpng_decode) and self.options.use_system_libpng and self.options.use_conan_libpng:
            replace_in_file(self, join(self.source_folder, "third_party", "libpng", "BUILD.gn"),
                            "libs = [ \"png\" ]", f"libs = {json.dumps(self._link_libs('libpng'))}",
                            strict=False)

        if self.options.use_zlib and self.options.use_system_zlib and self.options.use_conan_zlib:
            replace_in_file(self, join(self.source_folder, "third_party", "zlib", "BUILD.gn"),
                            "libs = [ \"z\" ]", f"libs = {json.dumps(self._link_libs('zlib'))}",
                            strict=False)

        if (self.options.use_libwebp_decode or self.options.use_libwebp_encode) and self.options.use_system_libwebp and self.options.use_conan_libwebp:
            replace_in_file(self, join(self.source_folder, "third_party", "libwebp", "BUILD.gn"),
                            "    libs = [\n      \"webp\",\n      \"webpdemux\",\n      \"webpmux\",\n    ]",
                            f"    libs = {json.dumps(self._link_libs('libwebp',components=['webp','webpmux','webpdemux','sharpyuv']))}",
                            strict=False)

        if self._is_ios_variant():
            if self.settings.arch == "armv8":
                replace_in_file(self, join(self.source_folder, "gn", "skia", "BUILD.gn"), 
                    "\"-arch\",\n        \"arm64e\",",
                    "#\"-arch\",\n        #\"arm64e\",",
                    strict=False)
            elif self.settings.arch == "armv8.3":
                replace_in_file(self, join(self.source_folder, "gn", "skia", "BUILD.gn"), 
                    "\"-arch\",\n        \"arm64\",",
                    "#\"-arch\",\n        #\"arm64\",",
                    strict=False)

        args = ""
        args += "is_official_build=true\n"
        args += f"is_component_build={self._get_lower_bool_str(self.options.shared)}\n"

        buildenv = VirtualBuildEnv(self).vars()

        if cc := buildenv.get("CC", default=None):
            args += f"cc=\"{cc}\"\n"
        if cxx := buildenv.get("CXX", default=None):
            args += f"cxx=\"{cxx}\"\n"
        if ar := buildenv.get("AR", default=None):
            args += f"ar=\"{ar}\"\n"

        if self.settings.os == "Emscripten":
            if emsdk := buildenv.get("EMSDK", default=None):
                args += f"skia_emsdk_dir=\"{emsdk}\"\n"

        if self.settings.os == "Windows":
            winsdk_version = self.conf.get("tools.microsoft:winsdk_version", default=None)
            if winsdk_version != None:
                args += f"win_sdk_version=\"{winsdk_version}\"\n"

        if self.settings.os == "Android":
            ndk_path = self.conf.get("tools.android:ndk_path", default=None)
            if ndk_path != None: 
                args += f"ndk = \"{ndk_path}\"\n"
            args += f"ndk_api = {self.settings.os.api_level}\n"

        if self.settings.os == "iOS":
            args += f"target_os = \"ios\"\n"
        elif self.settings.os == "tvOS":
            args += f"target_os = \"tvos\"\n"
        elif self._is_ios_variant():
            args += f"target_os = \"ios\"\n"

        if self._is_ios_variant_simulator():
            args += f"ios_use_simulator = true\n"

        if self.settings.arch == "x86":
            args += "target_cpu = \"x86\"\n"
        elif self.settings.arch == "x86_64":
            args += "target_cpu = \"x64\"\n"
        elif self.settings.arch == "wasm":
            args += "target_cpu = \"wasm\"\n"
        elif self.settings.arch == "armv7":
            args += "target_cpu = \"arm\"\n"
        elif self.settings.arch == "armv8":
            args += "target_cpu = \"arm64\"\n"
        elif self._is_ios_variant() and self.settings.arch == "armv8.3":
            args += "target_cpu = \"arm64\"\n"
        else:
            raise RuntimeError("Unexpected settings.arch")

        if self.settings.os == "Emscripten":
            args += f"skia_canvaskit_enable_alias_font = false\n"
            args += f"skia_canvaskit_enable_bidi = false\n"
            args += f"skia_canvaskit_enable_canvas_bindings = false\n"
            args += f"skia_canvaskit_enable_debugger = false\n"
            args += f"skia_canvaskit_enable_effects_deserialization = false\n"
            args += f"skia_canvaskit_enable_embedded_font = false\n"
            args += f"skia_canvaskit_enable_font = false\n"
            args += f"skia_canvaskit_enable_matrix_helper = false\n"
            args += f"skia_canvaskit_enable_paragraph = false\n"
            args += f"skia_canvaskit_enable_pathops = false\n"
            args += f"skia_canvaskit_enable_rt_shader = false\n"
            args += f"skia_canvaskit_enable_skp_serialization = false\n"
            args += f"skia_canvaskit_enable_sksl_trace = false\n"
            args += f"skia_canvaskit_enable_webgl = false\n"
            args += f"skia_canvaskit_enable_webgpu = false\n"
            args += f"skia_canvaskit_force_tracing = false\n"
            args += f"skia_canvaskit_include_viewer = false\n"
            args += f"skia_canvaskit_legacy_draw_vertices_blend_mode = false\n"
            args += f"skia_canvaskit_profile_build = false\n"

        cflags = []
        ldflags = []
        asmflags = []
        for dep in self.dependencies.host.values():
            cflags += [f"-I{dir}" for dir in dep.cpp_info.includedirs]
            cflags += [f"-D{define}" for define in dep.cpp_info.defines]
            if is_msvc(self):
                ldflags += [f"/LIBPATH:{dir}" for dir in dep.cpp_info.libdirs]
            else:
                ldflags += [f"-L{dir}" for dir in dep.cpp_info.libdirs]

            for component in dep.cpp_info.components.values():
                cflags += [f"-I{dir}" for dir in component.includedirs]
                cflags += [f"-D{define}" for define in component.defines]
                if is_msvc(self):
                    ldflags += [f"/LIBPATH:{dir}" for dir in component.libdirs]
                else:
                    ldflags += [f"-L{dir}" for dir in component.libdirs]

        if is_msvc(self):
            cflags += [f"/{msvc_runtime_flag(self)}"]

        if not self.options.shared and self.settings.compiler != "msvc":
            fpic = self.options.get_safe("fPIC")
            if fpic != None:
                cflags += ["-fPIC"]

        if is_apple_os(self):
            min_version_flag = AutotoolsToolchain(self).apple_min_version_flag
            cflags += [min_version_flag]
            ldflags += [min_version_flag]
            asmflags += [min_version_flag]

        cflags += [f"-D{define}" for define in self.conf.get("tools.build:defines", default=[], check_type=list)]
        ldflags += self.conf.get("tools.build:sharedlinkflags", default=[], check_type=list)
        ldflags += self.conf.get("tools.build:exelinkflags", default=[], check_type=list)

        linker_scripts = self.conf.get("tools.build:linker_scripts", default=[], check_type=list)
        ldflags += ["-T'" + linker_script + "'" for linker_script in linker_scripts]

        cflags += self.conf.get("tools.build:cflags", default=[], check_type=list)
        cflags_c = []
        cflags_cc = self.conf.get("tools.build:cxxflags", default=[], check_type=list)
        cflags_cc += [cppstd_flag(self)]

        args += f"extra_cflags={json.dumps(cflags)}\n"
        args += f"extra_cflags_c={json.dumps(cflags_c)}\n"
        args += f"extra_cflags_cc={json.dumps(cflags_cc)}\n"
        args += f"extra_ldflags={json.dumps(ldflags)}\n"
        args += f"extra_asmflags={json.dumps(asmflags)}\n"

        if self.settings.os == "Macos" or self._is_ios_variant():
            sdk_path = self.conf.get("tools.apple:sdk_path", default=None) 
            if not sdk_path: 
                sdk_path = XCRun(self).sdk_path
            if sdk_path:
                args += f"xcode_sysroot=\"{sdk_path}\"\n"

        if self._is_ios_variant():
            args += f"skia_ios_use_signing = false\n"

        for key in self._skia_options.keys():
            value = self.options.get_safe(key)
            if value != None and not key.startswith("use_conan"):
                b =  self._get_lower_bool_str(value)
                k = "use_system_freetype2" if key == "use_system_freetype" else key
                args += f"skia_{k}={b}\n"

        rmdir(self, "out/conan")
        mkdir(self, "out/conan")
        save(self, "out/conan/args.gn", args)

        if self.settings.os == "Windows":
            self.run("bin\gn gen out/conan")
        else:
            self.run("bin/gn gen out/conan")

        self.run("ninja -C out/conan skia svg skresources skcms skunicode skshaper skparagraph skottie bentleyottmann")

    def package(self):
        source_folder_include = join(self.source_folder, "include")
        source_folder_modules = join(self.source_folder, "modules")
        package_folder_include = join(self.package_folder, "include")
        package_folder_include_modules = join(package_folder_include, "modules")
        package_folder_lib = join(self.package_folder, "lib")
        package_folder_bin = join(self.package_folder, "bin")
        build_folder_out = join(self.build_folder, "out", "conan")

        # Skia headers have to be included relative to root of Skia repository (for example, #include <include/core/SkCanvas.h>).
        # So we have to create another 'include' folder inside include directory...

        # Include the entire skia root folder as an include (only copy includes)

        copy(self, "*.h", self.source_folder, package_folder_include)
        
        #copy(self, "*.h", source_folder_include, join(package_folder_include, "include"))

        ## removed sksg from this list
        #for mod in ["skottie", "skresource", "sksg", "skshaper", "skunicode", "svg"]:
        #    copy(self, "*.h", join(source_folder_modules, mod, "include"), join(package_folder_include_modules, mod, "include"))

        ## skcms doesn't have include folder for some reason.
        #copy(self, "*.h", join(source_folder_modules, "skcms"), join(package_folder_include_modules, "skcms"))

        if self.settings.os == "Windows":
            if self.options.shared:
                copy(self, "*.dll", build_folder_out, package_folder_bin, keep_path=False)
                copy(self, "*.dll.lib", build_folder_out, package_folder_lib, keep_path=False)
                # rename: *.dll.lib -> *.lib
                for filename in os.listdir(package_folder_lib):
                    if filename.endswith(".dll.lib"):
                        new_filename = filename.replace(".dll.lib", ".lib")
                        rename(self, join(package_folder_lib, filename), join(package_folder_lib, new_filename))
            else:
                copy(self, "*.lib", build_folder_out, package_folder_lib, keep_path=False)
        else:
            if self.options.shared:
                copy(self, "*.so", build_folder_out, package_folder_lib, keep_path=False)
                copy(self, "*.dylib", build_folder_out, package_folder_lib, keep_path=False)
            else:
                copy(self, "*.a", build_folder_out, package_folder_lib, keep_path=False)

        #if self.settings.os == "Emscripten" and self.settings.arch == "wasm":
        #    copy(self, "canvaskit.js", build_folder_out, package_folder_bin)
        #    copy(self, "canvaskit.wasm", build_folder_out, package_folder_bin)

    def package_info(self):

        # As of Conan 2.3.1, CMake generators cannot propagate transitive dependencies when you use components.
        # So we have to put evetything into root cpp_info...
        # https://github.com/conan-io/conan/issues/14888

        # component: skia

        self.cpp_info.defines += ["SK_CODEC_DEFINES_BMP"]
        self.cpp_info.defines += ["SK_CODEC_DEFINES_WBMP"]
        self.cpp_info.defines += ["SK_DISABLE_TRACING"] # assuming is_official_build

        if self.options.use_gl:
            self.cpp_info.defines += ["SK_GL"]

        if self.options.use_dawn:
            self.cpp_info.defines += ["SK_DAWN"]

        if self.options.use_direct3d:
            self.cpp_info.defines += ["SK_ENABLE_SPIRV_CROSS"]
            self.cpp_info.defines += ["SK_DIRECT3D"]
            self.cpp_info.system_libs += ["d3d12", "dxgi", "d3dcompiler"]

        if self.options.use_vulkan:
            self.cpp_info.defines += ["SK_VULKAN"]

        if self.options.use_metal:
            self.cpp_info.defines += ["SK_METAL"]

        if self.options.shared:
            self.cpp_info.defines += ["SKIA_DLL"]

        if self.settings.os == "Linux":
            self.cpp_info.defines += ["SK_R32_SHIFT=16"]

        if self.settings.os == "Android":
            self.cpp_info.system_libs += ["log"]

        if self.options.enable_optimize_size:
            self.cpp_info.defines += ["SK_ENABLE_OPTIMIZE_SIZE"]

        if self.options.enable_precompile:
            self.cpp_info.defines += ["SK_ENABLE_PRECOMPILE"]

        if self.options.enable_ganesh:
            self.cpp_info.defines += ["SK_GANESH"]

        if self.options.enable_graphite:
            self.cpp_info.defines += ["SK_GRAPHITE"]

        if self.options.use_perfetto:
            self.cpp_info.defines += ["SK_USE_PERFETTO"]

        if self.settings.os == "Macos" or self._is_ios_variant():
            if self.options.enable_api_available_macro:
                self.cpp_info.defines += ["SK_ENABLE_API_AVAILABLE"]
            else:
                self.cpp_info.cflags += ["-Wno-unguarded-availability"]

        if self.options.gl_standard == "gl":
            self.cpp_info.defines += ["SKIA_ASSUME_GL=1"]

        if self.options.gl_standard == "gles":
            self.cpp_info.defines += ["SKIA_ASSUME_GL_ES=1"]

        if self.options.gl_standard == "webgl":
            self.cpp_info.defines += ["SKIA_ASSUME_WEBGL=1"]

        if self.options.enable_pdf:
            if self.options.use_zlib and self.options.use_libjpeg_turbo_decode and self.options.use_libjpeg_turbo_encode:
                self.cpp_info.defines += ["SK_SUPPORT_PDF"]

        if self.options.use_xps:
            self.cpp_info.defines += ["SK_SUPPORT_XPS"]

        if self.options.use_wuffs and not self.options.shared:
            self.cpp_info.defines += ["SK_HAS_WUFFS_LIBRARY"]
            self.cpp_info.defines += ["SK_CODEC_DECODES_GIF"]
            self.cpp_info.libs += ["wuffs"]

        if self.options.use_piex and not self.options.shared:
            self.cpp_info.libs += ["piex"]

        if self.options.use_dng_sdk and self.options.use_libjpeg_turbo_decode and self.options.use_piex and not self.options.shared:
            self.cpp_info.defines += ["qDNGBigEndian=0"]
            self.cpp_info.libs += ["dng_sdk"]

        if self.options.use_libpng_decode:
            self.cpp_info.defines += ["SK_CODEC_DECODES_ICO"]
            self.cpp_info.defines += ["SK_CODEC_DECODES_PNG"]

        if self.options.use_libpng_decode:
            self.cpp_info.defines += ["SK_CODEC_DECODES_WEBP"]

        if self.options.use_libavif:
            self.cpp_info.defines += ["SK_CODEC_DECODES_AVIF"]

        if self.options.use_libjpeg_turbo_decode:
            self.cpp_info.defines += ["SK_CODEC_DECODES_JPEG"]

        if self.options.use_libjxl_decode:
            self.cpp_info.defines += ["SK_CODEC_DECODE_JPEGXL"]

        if self.options.use_ndk_images:
            self.cpp_info.defines += ["SK_ENABLE_NDK_IMAGES"]

        if self.options.use_expat:
            self.cpp_info.defines += ["SK_XML"]

        if self.options.use_dng_sdk and self.options.use_libjpeg_turbo_decode and self.options.use_piex:
            self.cpp_info.defines += ["SK_CODEC_DECODES_RAW"]

        if self.options.use_fonthost_mac:
            self.cpp_info.defines += ["SK_TYPEFACE_FACTORY_CORETEXT", "SK_FONTMGR_CORETEXT_AVAILABLE"]
            if self.settings.os == "Macos":
                self.cpp_info.frameworks += ["AppKit", "ApplicationServices"]
            elif self._is_ios_variant():
                self.cpp_info.frameworks += ["CoreFoundation", "CoreGraphics", "CoreText", "UIKit"]

        if self.options.enable_fontmgr_android:
            self.cpp_info.defines += ["SK_FONTMGR_ANDROID_AVAILABLE"]

        if self.options.enable_fontmgr_custom_directory:
            self.cpp_info.defines += ["SK_FONTMGR_FREETYPE_DIRECTORY_AVAILABLE"]

        if self.options.enable_fontmgr_custom_embedded:
            self.cpp_info.defines += ["SK_FONTMGR_FREETYPE_EMBEDDED_AVAILABLE"]

        if self.options.enable_fontmgr_custom_empty:
            self.cpp_info.defines += ["SK_FONTMGR_FREETYPE_EMPTY_AVAILABLE"]

        if self.options.enable_fontmgr_fontconfig:
            self.cpp_info.defines += ["SK_FONTMGR_FONTCONFIG_AVAILABLE"]

        if self.options.enable_fontmgr_win:
            self.cpp_info.defines += ["SK_TYPEFACE_FACTORY_DIRECTWRITE", "SK_FONTMGR_DIRECTWRITE_AVAILABLE"]

        if self.options.enable_fontmgr_win_gdi:
            self.cpp_info.defines += ["SK_FONTMGR_GDI_AVAILABLE"]
            self.cpp_info.system_libs += ["gdi32"]

        # component: pathkit
        # not in program's build, so it'll be commented for now

        #if not self.options.shared:
        #    self.cpp_info.libs += ["pathkit"]

        # component: skcms

        if not self.options.shared:
            self.cpp_info.libs += ["skcms"]

        # component: sksg

        # sksg doesn't exist in the program's build, so it'll be commented for now
        # if not self.options.shared:
        #     self.cpp_info.libs += ["sksg"]

        
        # component: svg

        if self.options.enable_svg and self.options.use_expat:
            self.cpp_info.defines += ["SK_SVG"]
            self.cpp_info.libs += ["svg"]

        # component: skresource

        if not self.options.shared:
            self.cpp_info.libs += ["skresources"]
            
        self.cpp_info.libs += ["skia"] # Placed this after some components due to library order being important on some systems

        # component: skunicode

        if self.options.enable_skunicode:
            self.cpp_info.defines += ["SK_UNICODE_AVAILABLE"]
            self.cpp_info.defines += ["SK_UNICODE_ICU_IMPLEMENTATION"]
            self.cpp_info.libs += ["skunicode_icu", "skunicode_core"] # Swapped order

        # component: skparagraph

        if self.options.enable_skparagraph:
            if not (self.settings.os == "Windows" and self.options.shared):
                self.cpp_info.libs += ["skparagraph"]
            self.cpp_info.defines += ["SK_ENABLE_PARAGRAPH"]

        # component: skshaper

        if self.options.enable_skshaper:
            self.cpp_info.libs += ["skshaper"]
            self.cpp_info.defines += ["SK_SHAPER_PRIMITIVE_AVAILABLE"]
            if self.options.use_fonthost_mac:
                self.cpp_info.defines += ["SK_SHAPER_CORETEXT_AVAILABLE"]
            if self.options.use_harfbuzz:
                self.cpp_info.defines += ["SK_SHAPER_HARFBUZZ_AVAILABLE"]
            if self.options.shared:
                self.cpp_info.defines += ["SK_SHAPER_UNICODE_AVAILABLE"]

        # component: skottie

        if self.options.enable_skottie:
            self.cpp_info.libs += ["skottie"]
            self.cpp_info.defines += ["SK_ENABLE_SKOTTIE", "SK_ENABLE_SKSLEFFECT"]

        # component: bentleyottmann

        if self.options.enable_bentleyottmann:
            if not (self.settings.os == "Windows" and self.options.shared):
                self.cpp_info.libs += ["bentleyottmann"]
