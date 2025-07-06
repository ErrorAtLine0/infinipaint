cmake_minimum_required(VERSION 3.27.0)
set(CMAKE_CXX_STANDARD 23)
project(canvasprogramserver)

set(sources "servermain.cpp"
            "MainServer.cpp"
            "ClientData.cpp"
            "../../include/Helpers/Networking/NetServer.cpp"
            "../../include/Helpers/Networking/NetClient.cpp"
            "../../include/Helpers/Networking/ByteStream.cpp"
            "../../include/Helpers/Random.cpp"
            "../DrawComponents/DrawBrushStroke.cpp"
            "../DrawComponents/DrawComponent.cpp"
            "../DrawComponents/DrawEllipse.cpp"
            "../DrawComponents/DrawRectangle.cpp"
            "../DrawComponents/DrawTextBox.cpp"
            "../DrawComponents/DrawImage.cpp"
        )


find_package(GameNetworkingSockets)
add_executable(canvasprogramserver ${sources})
target_compile_definitions(canvasprogramserver PRIVATE IS_SERVER)

if(WIN32)
    set_property(TARGET canvasprogramserver PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_include_directories(canvasprogramserver PRIVATE "../../include" "../..")
    target_link_libraries(canvasprogramserver GameNetworkingSockets::shared)
else()
    target_include_directories(canvasprogramserver PRIVATE "../../include" "../.." "../../../GameNetworkingSockets/include")
    target_compile_options(canvasprogramserver PRIVATE -O3)
    target_link_options(canvasprogramserver PRIVATE -O3)
    target_link_libraries(canvasprogramserver GameNetworkingSockets::shared)
endif()

install(TARGETS canvasprogramserver DESTINATION bin)
