##Building
This program is written in C++, and uses [conan](https://conan.io) to fetch dependencies.

##Linux
After cloning the repository, `cd` into the repo, then run:
```
./conan/export_libs.sh
conan install . --build=missing -pr=conan/profiles/linux-x86_64
cd build/Release
source generators/conanbuild.sh
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
source generators/deactivate_conanbuild.sh
```
After building the project, you should place the `data` folder located in the root of the repository next to the `infinipaint` executable before running it. What I usually do is create a symbolic link of the data folder and place it in the build directory.

You can also build in Debug mode by setting the `build_type` in the `conan install` command to `Debug`, and also by setting `CMAKE_BUILD_TYPE=Debug` when running CMake.
##macOS
After cloning the repository, `cd` into the repo, then run:
```
./conan/export_libs.sh
conan install . --build=missing -pr=conan/profiles/macOS-arm64
cd build/Release
source generators/conanbuild.sh
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
source generators/deactivate_conanbuild.sh
```
This will build an app bundle that contains all the data necessary to run the program. To create a .dmg installer for the app, run:
```
cpack -G DragNDrop
```
##Windows
The windows version of this program is built on Visual Studio 2022's compiler.

After cloning the repository, `cd` into the repo, then run:
```
.\conan\export_libs.bat
conan install . --build=missing -pr=conan/profiles/win-x86_64
cd build
source generators/conanbuild.sh
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
source generators/deactivate_conanbuild.sh
```
After building the project, you should place the `data` folder located in the root of the repository next to the `infinipaint.exe` executable before running it.

You can create an NSIS installer of the application by running:
```
cpack -G NSIS
```

##Emscripten
You can use Emscripten to build a web version of this program. Keep in mind that this version might be more buggy. In addition, I have only tried building it on a Linux machine.

After cloning the repository, `cd` into the repo, then run:
```
./conan/export_libs.sh
conan install . --profile:host=conan/profiles/emscripten --profile:build=default --build=missing
cd build/Release
ln -s ../../data data
source generators/conanbuild.sh
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
source generators/deactivate_conanbuild.sh
```
These commands will generate a javascript file containing the entire program. An example of a website that can run and display this program can be found in `emscripteninstall/index.html`. To try this out, place `infinipaint.js`, `emscripteninstall/index.html`, and `emscripteninstall/loading.gif` in a folder, and host a webserver from that folder.

Alternatively, you could compile a version of this program that is entirely contained in a single HTML file. This is not ideal for hosting, as the entire HTML file has to be downloaded before anything can be displayed to the user. However, you can open this file in any browser without a need for a web server.

After cloning the repository, `cd` into the repo, then run:
```
./conan/export_libs.sh
conan install . --profile:host=conan/profiles/emscripten --profile:build=default --build=missing
cd build/Release
ln -s ../../data data
ln -s ../../emscripteninstall emscripteninstall
source generators/conanbuild.sh
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN_SINGLE_HTML_FILE=ON
cmake --build .
source generators/deactivate_conanbuild.sh
```
You can open `infinipaint.html` directly in any browser to run the program.