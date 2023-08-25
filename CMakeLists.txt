cmake_minimum_required(VERSION 3.20)
project(Network_proj)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

include_directories(${GTK4_INCLUDE_DIRS})
link_directories(${GTK4_LIBRARY_DIRS})

find_package(Boost)
IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
endif ()

set(Boost_USE_STATIC_LIBS ON) # disable dynamic linking
set(Boost_USE_MULTITHREAD ON)  # enable multithreading
find_package(Boost COMPONENTS REQUIRED chrono filesystem serialization)

if (WIN32)
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT
            "<CMAKE_RC_COMPILER> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif()

# 关闭控制台
add_link_options(-mwindows)

add_executable(Network_proj app.rc main.cpp Peer.cpp Peer.h Query.h QueryHit.h Client.cpp Client.h Server.cpp Server.h fileHelper.h DownloadManager.cpp DownloadManager.h ConfigManager.h Controller.cpp Controller.h UI.cpp UI.h UI.cpp UI.h fileHelper.cpp)

# 寻找 libzip 库
target_link_libraries(Network_proj
        ${GTK4_LIBRARIES}
        LibZip
        ${Boost_LIBRARIES} ws2_32 wsock32)


