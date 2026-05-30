cmake_minimum_required(VERSION 3.10)
project(AutoClicker VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

add_executable(autoclicker
    src/main.cpp
    src/UI.cpp
    src/ClickEngine.cpp
    src/ConfigManager.cpp
    src/HotkeyManager.cpp
    src/Statistics.cpp
)

target_link_libraries(autoclicker
    ${X11_LIBRARIES}
    Xtst
    Threads::Threads
)

target_include_directories(autoclicker PRIVATE include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
