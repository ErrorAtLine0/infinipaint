@echo off
cd %~dp0
rmdir /S /Q portable_package
del infinipaint-x.y.z-win-arm64-portable.zip
mkdir portable_package
copy /y "..\build-arm64\build\Release\infinipaint.exe" portable_package
copy /y "dlls-arm64\*.dll" portable_package
robocopy "..\assets\data" "portable_package\data" /E
cd portable_package
tar -a -c -f "../infinipaint-x.y.z-win-arm64-portable.zip" "*"
cd ..
rmdir /S /Q portable_package