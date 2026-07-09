#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")"

if [[ -d builddir ]]; then
  echo "builddir exists, skip"
else
  mkdir builddir
fi

if [[ -d buildarea ]]; then
  echo "buildarea exists, skip"
else
  mkdir buildarea
fi

BUILD_AREA=$(realpath buildarea)
BUILD_DIR=$(realpath builddir)
export LD_LIBRARY_PATH="$BUILD_DIR/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBRARY_PATH="$BUILD_DIR/lib:${LIBRARY_PATH:+:$LIBRARY_PATH}"
export CPATH="$BUILD_DIR/include:${CPATH:+:$CPATH}"
export PKG_CONFIG_PATH="$BUILD_DIR/lib/pkgconfig:${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
cd $BUILD_AREA

# ----- ICU -----

if [[ -d icu ]]; then
  echo "ICU exists, skip"
else
  # download and unpack
  wget https://github.com/unicode-org/icu/releases/download/release-77-1/icu4c-77_1-src.tgz
  tar -xvzf icu4c-77_1-src.tgz
  rm icu4c-77_1-src.tgz
  cd icu

  # compile and install
  cd source
  chmod +x runConfigureICU configure install-sh
  ./runConfigureICU Linux --prefix=$BUILD_DIR --disable-icu-config --disable-icuio --disable-extras --disable-tools --disable-tests --disable-samples --disable-layoutex --disable-layout --with-data-packaging=archive
  make -j$(nproc)
  make install

  # cleanup
  cd $BUILD_AREA
fi


# ----- zstd -----

if [[ -d zstd ]]; then
  echo "zstd exists, skip"
else
  # download
  git clone https://github.com/facebook/zstd.git
  cd zstd
  git checkout f8745da6ff1ad1e7bab384bd1f9d742439278e99

  # compile and install
  cd build/cmake
  cmake -B build -DCMAKE_INSTALL_PREFIX=$BUILD_DIR
  cd build
  make -j$(nproc)
  make install

  # cleanup
  cd $BUILD_AREA
fi

# ----- libdatachannel -----

if [[ -d libdatachannel ]]; then
  echo "libdatachannel exists, skip"
else
  # download
  git clone https://github.com/paullouisageneau/libdatachannel.git
  cd libdatachannel
  git checkout 8c31097ea78f051e857d0aa1b2f6efb26cd12b7e
  git submodule update --init --recursive

  # compile and install
  cmake -B build -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DNO_MEDIA=ON -DCMAKE_BUILD_TYPE=Release
  cd build
  make -j$(nproc)
  make install

  # cleanup
  cd $BUILD_AREA
fi

# ----- oneTBB -----

if [[ -d oneTBB ]]; then
  echo "oneTBB exists, skip"
else
  # download
  git clone https://github.com/uxlfoundation/oneTBB.git
  cd oneTBB
  git checkout 0c0ff192a2304e114bc9e6557582dfba101360ff

  # compile and install
  cmake -B build -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DTBB_TEST=OFF
  cd build
  make -j$(nproc)
  make install

  # cleanup
  cd $BUILD_AREA
fi

# ----- Clipper2 -----

if [[ -d Clipper2 ]]; then
  echo "Clipper2 exists, skip"
else
  # download
  git clone https://github.com/AngusJohnson/Clipper2.git
  cd Clipper2
  git checkout 21ebba05db8894f0c7217ad35ea518080f324946

  # compile and install
  cd CPP
  cmake -B build -DCMAKE_INSTALL_PREFIX=$BUILD_DIR -DCLIPPER2_UTILS=OFF -DCLIPPER2_EXAMPLES=OFF -DCLIPPER2_TESTS=OFF
  cd build
  make -j$(nproc)
  make install

  # cleanup
  cd $BUILD_AREA
fi

# ----- Skia -----
if [[ -d skia-da51f0d60ea2b14e845a823dc11b405dbeef42d8 ]]; then
  echo "skia-da51f0d60ea2b14e845a823dc11b405dbeef42d8 exists, skip"
else
  # download
  wget https://github.com/google/skia/archive/da51f0d60ea2b14e845a823dc11b405dbeef42d8.zip
  unzip da51f0d60ea2b14e845a823dc11b405dbeef42d8.zip
  rm da51f0d60ea2b14e845a823dc11b405dbeef42d8.zip
  cd skia-da51f0d60ea2b14e845a823dc11b405dbeef42d8

  # compile and install
  python3 tools/git-sync-deps
  bin/gn gen out/Static --args='is_official_build=true skia_use_system_expat=false skia_use_system_freetype2=true skia_use_system_harfbuzz=true skia_use_system_icu=true skia_use_system_libjpeg_turbo=false skia_use_system_libpng=true skia_use_system_libwebp=false skia_use_system_zlib=true skia_use_egl=true skia_use_x11=false'
  ninja -C out/Static skia svg skunicode_core skunicode_icu skparagraph skresources skshaper
  rm -rf $BUILD_AREA/Skia
  mkdir $BUILD_AREA/Skia
  mv out $BUILD_AREA/Skia
  mv include $BUILD_AREA/Skia
  mv src $BUILD_AREA/Skia
  mv modules $BUILD_AREA/Skia

  # cleanup
  cd $BUILD_AREA
fi

# ----- InfiniPaint -----

# compile and install
if [[ -d infinipaint-build ]]; then
  echo "infinipaint-build exists, skip mkdir"
else
  mkdir infinipaint-build
fi

cd ../../..
cmake -B $BUILD_AREA/infinipaint-build -DCMAKE_INSTALL_PREFIX=$BUILD_DIR \
  -DCMAKE_PREFIX_PATH=$BUILD_DIR \
  -DNO_CONAN_BUILD=ON \
  -DSKIA_INCLUDE=$BUILD_AREA/Skia \
  -DSKIA_LIB=$BUILD_AREA/Skia/out/Static/libskia.a \
  -DSKIA_SVG_LIB=$BUILD_AREA/Skia/out/Static/libsvg.a \
  -DSKIA_RESOURCES_LIB=$BUILD_AREA/Skia/out/Static/libskresources.a \
  -DSKIA_SHAPER_LIB=$BUILD_AREA/Skia/out/Static/libskshaper.a \
  -DSKIA_UNICODE_ICU_LIB=$BUILD_AREA/Skia/out/Static/libskunicode_core.a \
  -DSKIA_UNICODE_CORE_LIB=$BUILD_AREA/Skia/out/Static/libskunicode_icu.a \
  -DSKIA_PARAGRAPH_LIB=$BUILD_AREA/Skia/out/Static/libskparagraph.a \
  -DCMAKE_BUILD_TYPE=Release \
  -DGRAPHICS_BACKEND=OpenGL3.3
cd $BUILD_AREA/infinipaint-build
make -j$(nproc)
make install

# cleanup
cd $BUILD_AREA
