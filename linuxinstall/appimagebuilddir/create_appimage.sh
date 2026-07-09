#!/bin/bash

cd "$(dirname "$0")"
rm -rf AppDir
./create_appimage_appdir.sh
./appimagetool.AppImage AppDir
