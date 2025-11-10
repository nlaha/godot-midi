#!/bin/bash
echo "Building Godot Engine..."

# detect OS and set platform variable
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="osx"
elif [[ "$OSTYPE" == "cygwin" ]]; then
    PLATFORM="windows"
elif [[ "$OSTYPE" == "msys" ]]; then
    PLATFORM="windows"
elif [[ "$OSTYPE" == "win32" ]]; then
    PLATFORM="windows"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "Cloning Godot Engine repository..."
git clone https://github.com/godotengine/godot.git
git checkout 4.4
echo "Building Godot Engine..."
cd godot
scons platform=$PLATFORM dev_build=yes debug_symbols=yes tools=yes
echo "Godot Engine built successfully."