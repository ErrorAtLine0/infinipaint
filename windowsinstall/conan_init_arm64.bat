@echo off
cd %~dp0
cd ..
call .\conan\export_libs.bat
conan install . -of=build-arm64 --build=missing -pr=conan/profiles/win-arm64