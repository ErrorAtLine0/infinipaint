@echo off
cd %~dp0
cd ..\build-x86_64-debug\build
call .\generators\conanbuild.bat
cmake ..\.. -DCMAKE_TOOLCHAIN_FILE="generators\conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Debug -DCONFIG_NEXT_TO_EXECUTABLE=OFF
cmake --build . --config Debug
call .\generators\deactivate_conanbuild.bat