#!/bin/bash

cd ~/autoclicker_cpp

if [ ! -f build/autoclicker ]; then
    cd build && cmake .. && make -j$(nproc) && cd ..
fi

echo "Starting AutoClicker as normal user..."
echo "If you see BadAccess error, run: killall flameshot gnome-screenshot spectacle"
echo ""
./build/autoclicker
