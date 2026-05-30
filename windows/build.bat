@echo off
echo Building AutoClicker for Windows...

if not exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Visual Studio not found. Please install Visual Studio 2022 Community.
    echo Or use: cl /EHsc autoclicker_win.cpp /Fe:autoclicker.exe
    goto :eof
)

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /O2 /MT autoclicker_win.cpp /Fe:autoclicker.exe /link user32.lib

if exist autoclicker.exe (
    echo Build successful! autoclicker.exe created
    echo Run: autoclicker.exe
) else (
    echo Build failed
)
