#!/bin/bash

# Configuration
SDL_VERSION="2.30.0"
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-devel-${SDL_VERSION}-mingw.tar.gz"
VENDOR_DIR="vendor"
TARGET_DIR="${VENDOR_DIR}/sdl2-win"
TMP_DIR="temp_sdl_download"

# Ensure vendor directory exists
mkdir -p "$VENDOR_DIR"

# Check if already installed
if [ -d "$TARGET_DIR" ]; then
    echo "SDL2 for Windows appears to be already installed in $TARGET_DIR"
    exit 0
fi

echo "Downloading SDL2 ${SDL_VERSION} for MinGW..."
mkdir -p "$TMP_DIR"
curl -L -o "$TMP_DIR/sdl2.tar.gz" "$SDL_URL"

if [ $? -ne 0 ]; then
    echo "Failed to download SDL2"
    rm -rf "$TMP_DIR"
    exit 1
fi

echo "Extracting..."
tar -xzf "$TMP_DIR/sdl2.tar.gz" -C "$TMP_DIR"

# The tarball contains SDL2-VERSION/x86_64-w64-mingw32 among others
# We want the 64-bit version
SOURCE_PATH="$TMP_DIR/SDL2-${SDL_VERSION}/x86_64-w64-mingw32"

if [ -d "$SOURCE_PATH" ]; then
    echo "Installing to $TARGET_DIR..."
    mv "$SOURCE_PATH" "$TARGET_DIR"
    echo "Success!"
else
    echo "Error: Could not find expected directory structure in archive."
    ls -R "$TMP_DIR"
    exit 1
fi

# Npcap SDK Setup
NPCAP_SDK_VERSION="1.13"
NPCAP_SDK_URL="https://npcap.com/dist/npcap-sdk-${NPCAP_SDK_VERSION}.zip"
NPCAP_TARGET_DIR="${VENDOR_DIR}/npcap-sdk"

if [ -d "$NPCAP_TARGET_DIR" ]; then
    echo "Npcap SDK appears to be already installed in $NPCAP_TARGET_DIR"
else
    echo "Downloading Npcap SDK ${NPCAP_SDK_VERSION}..."
    mkdir -p "$TMP_DIR"
    curl -L -o "$TMP_DIR/npcap-sdk.zip" "$NPCAP_SDK_URL"

    if [ $? -ne 0 ]; then
        echo "Failed to download Npcap SDK"
        rm -rf "$TMP_DIR"
        exit 1
    fi

    echo "Extracting Npcap SDK..."
    unzip -q "$TMP_DIR/npcap-sdk.zip" -d "$NPCAP_TARGET_DIR"
    
    echo "Npcap SDK installed to $NPCAP_TARGET_DIR"
fi

# Cleanup
rm -rf "$TMP_DIR"

echo "Done. You can now run 'make windows'."
