@echo off
cd %~dp0
cd ..
call .\conan\export_libs.bat
conan install . -of=build-x86_64 --build=missing -pr=conan/profiles/win-x86_64