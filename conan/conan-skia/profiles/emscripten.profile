include(default)

[settings]
os=Emscripten
arch=wasm
compiler=clang
compiler.version=16
compiler.libcxx=libc++
compiler.cppstd=17

[tool_requires]
*: emsdk/3.1.50
