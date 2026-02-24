 # During multiple stages of CMake configuration, the toolchain file is processed and command-line
 # variables may not be always available. The script exits prematurely if essential variables are absent.

 if ( NOT ANDROID_ABI OR NOT CMAKE_BUILD_TYPE )
     return()
 endif()
 if(${ANDROID_ABI} STREQUAL "x86_64")
     include("${CMAKE_CURRENT_LIST_DIR}/build/x86_64/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake")
 elseif(${ANDROID_ABI} STREQUAL "x86")
     include("${CMAKE_CURRENT_LIST_DIR}/build/x86/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake")
 elseif(${ANDROID_ABI} STREQUAL "arm64-v8a")
     include("${CMAKE_CURRENT_LIST_DIR}/build/armv8/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake")
 elseif(${ANDROID_ABI} STREQUAL "armeabi-v7a")
     include("${CMAKE_CURRENT_LIST_DIR}/build/armv7/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake")
 else()
     message(FATAL "Not supported configuration")
 endif()
