@echo off
echo Building AutoClicker for Windows with MinGW...

x86_64-w64-mingw32-g++ -static -O2 -std=c++17 -pthread ^
    autoclicker_win.cpp -o autoclicker.exe ^
    -luser32 -lgdi32 -lwinmm

if exist autoclicker.exe (
    echo Build successful! autoclicker.exe created
    dir autoclicker.exe
) else (
    echo Build failed - make sure MinGW is installed
    echo Download from: https://www.mingw-w64.org/
)
