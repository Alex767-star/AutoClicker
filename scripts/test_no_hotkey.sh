#!/bin/bash

cd ~/autoclicker_cpp/build

# Run without hotkey grabbing for testing
cat > test_nograb.cpp << 'TEST_EOF'
#include <iostream>
#include <thread>
#include <chrono>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

int main() {
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;
    
    std::cout << "AutoClicker Test - No hotkeys" << std::endl;
    std::cout << "Clicking every 500ms for 5 seconds..." << std::endl;
    
    for (int i = 0; i < 10; i++) {
        XTestFakeButtonEvent(dpy, 1, True, CurrentTime);
        XTestFakeButtonEvent(dpy, 1, False, CurrentTime);
        XFlush(dpy);
        std::cout << "Click " << i+1 << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    XCloseDisplay(dpy);
    std::cout << "Done" << std::endl;
    return 0;
}
TEST_EOF

g++ -o test_nograb test_nograb.cpp -lX11 -lXtst -pthread
./test_nograb
rm test_nograb test_nograb.cpp
