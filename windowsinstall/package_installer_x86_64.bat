@echo off
cd %~dp0
cd ..\build-x86_64\build
call .\generators\conanbuild.bat
cpack -G NSIS
call .\generators\deactivate_conanbuild.bat
cd %~dp0
for %%f in ("..\build-x86_64\build\infinipaint-*.exe") do (
    copy /y "%%f" "infinipaint-x.y.z-win-x86_64-installer.exe"
)