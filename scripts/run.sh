#!/bin/bash

BINARY="$HOME/autoclicker_cpp/build/autoclicker"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found. Building first..."
    cd ~/autoclicker_cpp/build && make -j$(nproc)
fi

sudo "$BINARY"
