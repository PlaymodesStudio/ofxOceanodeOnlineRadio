#!/bin/bash

# Installation script for ofxOnlineRadio dependencies
echo "Installing ofxOnlineRadio dependencies..."

# Check if VLC is installed
if [ ! -d "/Applications/VLC.app" ]; then
    echo "VLC not found. Installing via Homebrew..."
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    # Install VLC
    brew install --cask vlc
else
    echo "VLC already installed"
fi

# Verify VLC installation
if [ -f "/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib" ]; then
    echo "VLC libraries found"
else
    echo "Error: VLC libraries not found"
    exit 1
fi

echo "Installation complete!"