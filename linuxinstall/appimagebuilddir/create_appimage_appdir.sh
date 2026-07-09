#!/bin/bash

cd "$(dirname "$0")"
mkdir AppDir
mkdir AppDir/usr
cp -r builddir/bin AppDir/usr
cp -r builddir/lib AppDir/usr

mkdir AppDir/usr/share
mkdir AppDir/usr/share/icons
mkdir AppDir/usr/share/icons/hicolor
mkdir AppDir/usr/share/icons/hicolor/scalable
mkdir AppDir/usr/share/icons/hicolor/scalable/apps
cp ../../logo.svg AppDir/usr/share/icons/hicolor/scalable/apps/infinipaint.svg

mkdir AppDir/usr/share/applications
cp ../appimage_infinipaint.desktop AppDir/usr/share/applications/infinipaint.desktop

cp ../AppRun AppDir/AppRun

cd AppDir
ln -s usr/share/applications/infinipaint.desktop infinipaint.desktop
ln -s usr/share/icons/hicolor/scalable/apps/infinipaint.svg infinipaint.svg

# Renaming and cleaning up lib directory
cd usr
rm -rf lib/debug
#   - /Skia
#   - /include
rm -rf lib/pkgconfig
rm -rf lib/cmake
rm -rf lib/libupb.a
rm -rf bin/proto*
rm -rf lib/*.a
rm -rf lib/libicui18n*
# rm -rf lib/libicudata* # Should just be a stubfile, check the size of the library (should be small, doesn't actually contain data)
rm -rf bin/zstd*
rm -rf bin/unzstd
