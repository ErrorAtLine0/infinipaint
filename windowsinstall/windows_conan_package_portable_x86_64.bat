:: @echo off
cd %~dp0
rmdir /S /Q portable_package
del infinipaint-x.y.z-win-x86_64-portable.zip
mkdir portable_package
copy /y "..\build-x86_64\build\Release\infinipaint.exe" portable_package
copy /y "dlls-x86_64\*.dll" portable_package
robocopy "..\assets\data" "portable_package\data" /E
cd portable_package
tar -a -c -f "../infinipaint-x.y.z-win-x86_64-portable.zip" "*"
cd ..
rmdir /S /Q portable_package