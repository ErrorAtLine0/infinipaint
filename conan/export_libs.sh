#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

conan export "$(dirname "$0")/libjuice/all/conanfile.py" --version=1.6.1
conan export "$(dirname "$0")/libdatachannel/all/conanfile.py" --version=0.23.1
conan export "$(dirname "$0")/mpg123/all/conanfile.py" --version=1.32.10
conan export "$(dirname "$0")/conan-skia/recipes/skia/all/conanfile.py" --version=134.20250327.0
conan export "$(dirname "$0")/sdl3/3.x/conanfile.py" --version=3.2.24
#conan create . -o use_system_expat=False -o use_system_freetype=False -o use_system_harfbuzz=False -o use_system_icu=False -o use_system_libjpeg_turbo=False -o use_system_libpng=False -o use_system_libwebp=False -o use_system_zlib=False --build=missing --version=134.20250327.0
#cd ../../../..
