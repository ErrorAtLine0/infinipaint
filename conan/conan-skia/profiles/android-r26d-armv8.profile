include(default)

[settings]
os=Android
os.api_level=28
arch=armv8
compiler=clang
compiler.version=12
compiler.libcxx=c++_static
compiler.cppstd=17

[tool_requires]
android-ndk/r26d
make/4.4.1
