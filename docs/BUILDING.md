## Building
This program is written in C++, and uses [conan](https://conan.io) to fetch dependencies.
## Linux
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
## macOS
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
## Windows
The windows version of this program is built on Visual Studio 2022's compiler.

After cloning the repository, `cd` into the repo, then update the git submodules to get the `clip` library:
```
git submodule update --init --recursive
```
Then run:
```
.\conan\export_libs.bat
conan install . --build=missing -pr=conan/profiles/win-x86_64
cd build
.\generators\conanbuild.bat
cmake .. -DCMAKE_TOOLCHAIN_FILE="generators\conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
.\generators\deactivate_conanbuild.bat
```
After building the project, you should place the `data` folder located in the root of the repository next to the `infinipaint.exe` executable before running it.

You can create an NSIS installer of the application by running:
```
cpack -G NSIS
```
## Emscripten
You can use Emscripten to build a web version of this program. Keep in mind that this version might be more buggy, and is missing a few features. In addition, I have only tried building it on a Linux machine.

After cloning the repository, `cd` into the repo, then update the git submodules to get `datachannel-wasm`:
```
git submodule update --init --recursive
```
Then run:
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
These commands will generate a javascript file containing the entire program. An example of a website that can run and display this program can be found in `emscripteninstall/index.html`. To try this out, place `infinipaint.js`, `emscripteninstall/index.html`, and `emscripteninstall/loading.gif` in a folder, and host a webserver from that folder. The server must set these two HTTP headers:
```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```
## Android
Building the android APK has only been tested on a Linux machine. It could probably work on Mac as well, but it won't work on Windows as is because of some issues with conan packages. If you're doing this on Windows, you might need to work in WSL or a Linux VM to get this working, but this hasn't been tested.
Note that the debug version will not be able to access the project files stored in the release version. For now, the only way to transfer files between the two versions is to share them from the release version to an intermediate application, and then import them through the debug version. In the future, I'll add a feature to save InfiniPaint canvases to an external folder accessible by all applications, so hopefully this should also be more manageable.

After cloning the repository, `cd` into the repo, then update the git submodules:
```
git submodule update --init --recursive
```
Then run:
```
./conan/export_libs.sh
```
You'll need Android Studio and the NDK to compile the APK. Follow these steps:
- Download and install Android Studio from https://developer.android.com/studio
- In Android Studio, open android-project from the InfiniPaint repository
- Download the NDK required to build InfiniPaint through android studio by going to Tools > SDK Manager > Languages & Frameworks > Android SDK > SDK Tools. Check "Show Package Details", and check version "30.0.16248370" from "NDK (Side by side)"
- The android NDK should now be installed. Get the path to the NDK folder by going to the path listed as "Android SDK Location" in the Android SDK window we just opened, go to the folder "ndk", and then the folder "30.0.16248370". Keep note of this path. It could be something like "/home/USER_NAME/Android/Sdk/ndk/30.0.16248370" on Linux
- Copy the android conan profile file located in the infinipaint repository at conan/profiles/android, to conan's default profile directory. On Linux, this could be at /home/USER_NAME/.conan2/profiles
- Open the copied profile, and paste the path to the NDK we took note of before in the field "tools.android:ndk_path"
- After this, you can run the application in any way you want through Android Studio. To build the APK, you can go to Build > Generate App Bundles or APKs > Generate APKs
