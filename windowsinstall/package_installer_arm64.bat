@echo off
cd %~dp0
cd ..\build-arm64\build
call .\generators\conanbuild.bat
cpack -G NSIS
call .\generators\deactivate_conanbuild.bat
cd %~dp0
for %%f in ("..\build-arm64\build\infinipaint-*.exe") do (
    copy /y "%%f" "infinipaint-x.y.z-win-arm64-installer.exe"
)