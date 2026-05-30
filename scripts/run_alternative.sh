#!/bin/bash

cd ~/autoclicker_cpp

# Kill potential key grabbers
killall flameshot 2>/dev/null
killall gnome-screenshot 2>/dev/null
killall spectacle 2>/dev/null
killall xfce4-screenshooter 2>/dev/null

echo "Starting AutoClicker with alternative hotkey detection..."
echo "Use Ctrl+Shift+F1, Ctrl+Shift+F2, Ctrl+Shift+F3 for hotkeys"
echo ""

./build/autoclicker
