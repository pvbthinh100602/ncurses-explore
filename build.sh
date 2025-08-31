#!/bin/bash
set -e  # exit on first error

# Config
BUILD_DIR="build"
APP_NAME="my_app"

# Step 1: Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

# Step 2: Go into build dir and configure
cd "$BUILD_DIR"
cmake ..

# Step 3: Build
cmake --build .

# Step 4: Run if executable exists
if [ -f "$APP_NAME" ]; then
    echo "Running ./$APP_NAME"
    echo "------------------------"
    ./$APP_NAME
else
    echo "Build finished, but $APP_NAME not found."
fi
