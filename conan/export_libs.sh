#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

conan export "$(dirname "$0")/conan-skia/recipes/skia/all/conanfile.py" --version=143.20251028.0
conan export "$(dirname "$0")/sdl3/3.x/conanfile.py" --version=3.2.26
conan export "$(dirname "$0")/icu/all/conanfile.py" --version=77.1
#conan create . -o use_system_expat=False -o use_system_freetype=False -o use_system_harfbuzz=False -o use_system_icu=False -o use_system_libjpeg_turbo=False -o use_system_libpng=False -o use_system_libwebp=False -o use_system_zlib=False --build=missing --version=134.20250327.0
#cd ../../../..
