#!/bin/bash

# BeforeLight Screensaver Installer for Arch Linux
# This script installs all dependencies and compiles the screensavers,
# then copies them to the Omarchy screensaver directory

set -e  # Exit on any error

echo "🚀 BeforeLight Screensaver Installer for Arch Linux"
echo "=================================================="

# Check if we're on Arch Linux
if ! command -v pacman &> /dev/null; then
    echo "❌ Error: This installer is designed for Arch Linux systems with pacman."
    echo "Please use the manual installation instructions in README.md for other distributions."
    exit 1
fi

echo "📦 Installing required packages..."

# Install all dependencies in one go
sudo pacman -S --needed --noconfirm \
    sdl2 \
    sdl2_image \
    sdl2_ttf \
    ttf-dejavu \
    ttf-liberation \
    gnu-free-fonts \
    libx11 \
    ncurses \
    gcc \
    make

echo "✅ Dependencies installed successfully!"

echo "🔨 Building BeforeLight screensavers..."
make clean
make all

echo "✅ Compilation completed!"

# Create the Omarchy screensaver directory
SCREEN_DIR="$HOME/.config/omarchy/branding/screensaver"
echo "📁 Creating Omarchy screensaver directory: $SCREEN_DIR"
mkdir -p "$SCREEN_DIR"

echo "📋 Copying screensavers to Omarchy directory..."

# Copy all compiled screensavers
cp build/* "$SCREEN_DIR" 2>/dev/null || {
    echo "❌ Error: No compiled screensavers found in build/ directory"
    echo "Make sure the compilation was successful."
    exit 1
}

echo "✅ Installation completed successfully!"
echo ""
echo "🎉 Your BeforeLight screensavers are now installed!"
echo ""
echo "📍 Location: $SCREEN_DIR"
echo "🖥️  Available screensavers:"
ls -1 "$SCREEN_DIR" | sed 's/^/   - /'
echo ""
echo "💡 To use with Omarchy, configure your screensaver to use these applications."
echo "📚 See README.md for detailed usage instructions and command line options."
