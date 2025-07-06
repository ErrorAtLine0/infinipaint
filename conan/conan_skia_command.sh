#!/bin/bash

cd conan-skia/recipes/skia/all
conan create . -o use_system_expat=False -o use_system_freetype=False -o use_system_harfbuzz=False -o use_system_icu=False -o use_system_libjpeg_turbo=False -o use_system_libpng=False -o use_system_libwebp=False -o use_system_zlib=False --build=missing --version=134.20250327.0
cd ../../../..
