@echo off
cd %~dp0
cd ..\build-arm64\build
call .\generators\conanbuild.bat
cmake ..\.. -DCMAKE_TOOLCHAIN_FILE="generators\conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release -DCONFIG_NEXT_TO_EXECUTABLE=OFF -DGRAPHICS_BACKEND=Vulkan
cmake --build . --config Release
call .\generators\deactivate_conanbuild.bat