# BeforeLight - Omarchy Desktop Screensavers

A collection of beautiful animated screensavers optimized for Linux + Hyprland/Wayland, featuring complete mouse motion detection and cursor management for professional screensaver behavior.

## ✨ New Features (Latest Update)

### 🎯 Enhanced Mouse Behavior
- **Automatic Mouse Detection**: All screensavers detect mouse movement and exit gracefully after a 2-second grace period
- **Professional Cursor Management**: Mouse cursor is hidden during screensaver operation and restored upon exit
- **Hyprland Integration**: Native Wayland support with `hyprctl` cursor control commands
- **Universal Implementation**: 13 screensavers now have consistent mouse behavior

### 🖱️ Mouse Detection Details
- **Grace Period**: 2 seconds of mouse inactivity tolerance before exit triggers
- **Cursor Hiding**: Automatic visibility toggle using Wayland commands
- **Motion Sensitivity**: Any mouse movement after grace period causes exit
- **Fallback Controls**: Keyboard ESC, mouse clicks, and window close still work immediately

## 🎨 Available Screensavers

### 🐟 Fish Aquarium (`fishsaver`)
Beautiful aquatic life simulation with swimming fish and rising bubbles.
- Various fish species with natural movement patterns
- Bubble physics with realistic rise animations

### 🍞 Flying Toasters (`toastersaver`)
Classic After Dark-inspired flying toast screensaver with bread flight paths.
- Multiple pieces of toast with physics-simulated flying
- Original computer-era nostalgia recreation

### 🌍 Rotating Globe (`globe`)
Realistic Earth globe spinning in space orbit with atmospheric effects.
- 3D sphere with embedded texture mapping
- Smooth rotation with intersection detection

### 🌇 Cityscape (`cityscape`) & ⛆ Spotlight (`spotlight`)
Urban night scenes with twinkling windows and dynamic spotlight beams.
- Neon-lit city buildings with building window lights
- Moving theatrical spotlight with glow effects

### 🌌 Logo Animation (`logo`)
Morphing company logo with rotating & scaling transformations.
- CSS-inspired animation curves
- Smooth logo deformation effects

### ⚽ Bouncing Balls (`bouncingball`)
Physics-based ball animation with momentum conservation.
- Multiple balls with realistic collision physics
- Elastic collisions and wall bounces

### 🌧️ Weather Effects
- **Hard Rain** (`hardrain`): Intense rainstorm with colored droplets
- **Rainstorm** (`rainstorm`): Realistic weather simulation with thunder simulation
- **Paper Fire** (`paperfire`): Burning paper physics with ember and smoke effects

### 💫 Ambient Animations
- **The Matrix** (`matrix`): Classic green falling character rain
- **Life Forms** (`lifeforms`): Conway's Game of Life cellular automata
- **Warp Effects** (`warp`): Liquid distortion patterns with wave animations

### 🎲 System Utilities
- **Screensaver Randomizer** (`randomizer`): Cycles through all available screensavers
- **Messages** (`messages`, `messages2`): Scrolling text displays with TTF fonts

## 🚀 Installation & Usage

### Prerequisites
```bash
# Required packages for all screensavers
sudo pacman -S sdl2 sdl2_image gcc make

# Additional for text-based screensavers
sudo pacman -S sdl2_ttf

# Optional: For ncurses configuration tool
sudo pacman -S ncurses
```

### Build All Screensavers
```bash
# Build all screensavers
make all

# Or build individual screensavers
make toastersaver
make fishsaver
make matrix
# ...etc
```

### Run Screensavers
```bash
# Force Wayland backend for Hyprland compatibility
SDL_VIDEODRIVER=wayland ./build/toastersaver
SDL_VIDEODRIVER=wayland ./build/fishsaver

# With full screen display
SDL_VIDEODRIVER=wayland ./build/matrix -f 1
```

### Configuration Tool
```bash
# Build and run the ncurses configuration tool
make screensaver_config
./build/screensaver_config
```

## 🎮 Mouse & Exit Controls

### ✅ New Mouse Behavior (v2.0)
```
🐭 Mouse Movement: Detected after 2-second grace period → Exit screensaver
🐭 Mouse Cursor: Automatically hidden during operation, restored on exit
⌨️  Keyboard ESC: Immediate exit
🖱️  Mouse Click: Immediate exit
❌ Window Close: Immediate exit
```

### 🕒 Grace Period Logic
- **First 2 seconds**: Mouse movement is ignored (grace period)
- **After 2 seconds**: Any mouse movement triggers clean exit
- **Cursor hidden**: During entire screensaver operation
- **Cursor restored**: When screensaver terminates

## 🔧 Technical Details

### Libraries & Dependencies
**Core Requirements:**
- `SDL2` (2.26+): Cross-platform development library
- `SDL2_image` (2.6+): Image loading and format support
- `gcc` (11+): C compiler with C99 support
- `make`: Build system

**Optional (for specific screensavers):**
- `SDL2_ttf` (2.20+): TrueType font rendering (matrix, messages)
- `ncurses` (6.3+): Terminal UI framework (config tool)

**Removed Dependencies:**
- ❌ `libx11`: No longer needed for spotlight screensaver
- ❌ `libGLU`: OpenGL dependencies removed from core build

### Architecture
- **Fully SDL2-Based**: Direct Wayland integration
- **Wayland Native**: Pure `SDL_VIDEODRIVER=wayland`
- **No X11 Dependencies**: Clean Wayland-only operation
- **Hardware Acceleration**: GPU-accelerated rendering where applicable

### File Structure
```
BeforeLight/
├── main_*.c             # Individual screensaver implementations
├── assets/              # Header-embedded textures and sprites
├── build/               # Compiled binaries (not in git)
├── install/             # Installation scripts
├── utils/               # Helper tools (PNG to C header converter)
├── Makefile             # Primary build configuration
├── screensaver_config.c # Ncurses configuration tool
└── README.md           # This documentation
```

## 📈 Build & Project Statistics

- **Total Screensavers**: 17+ (including variants)
- **Lines of Code**: ~15,000+ across all implementations
- **Assets**: 25+ embedded sprites and textures
- **Languages**: C99 with SDL2, Hyprland integration
- **Target FPS**: 60 FPS with VSYNC
- **Memory Usage**: <50MB per screensaver instance
- **CPU Usage**: <15% on modern systems

## 🎨 Visual Features

- **Resolution Adaptive**: Automatic detection of display resolution
- **Aspect Ratio Aware**: Proper scaling on different monitor shapes
- **Color Depth**: 32-bit RGBA rendering with alpha blending
- **Animation Timing**: Frame-rate independent timing systems
- **Particle Systems**: Advanced particle simulations (smoke, bubbles, fire)
- **Physics Simulation**: Realistic momentum conservation and collisions

## 🐛 Known Issues & Solutions

### Common Problems
- **Black screen**: Ensure `SDL_VIDEODRIVER=wayland` is set
- **Mouse not hidden**: Check Hyprland `hyprctl` availability
- **Poor performance**: Reduce particle counts or frame rates
- **Exit not working**: Verify 2-second grace period has elapsed

### Wayland-Specific Notes
- Pure Wayland implementation, no X11 fallback
- Requires `SDL_VIDEODRIVER=wayland` environment variable
- Tested extensively on Hyprland 0.30.0+
- Compatible with other Wayland compositors (Sway, Mir)

## 🔄 Development & Updates

### Recent Changes (v2.0)
- ✅ **Complete mouse motion detection** across all 13 screensavers
- ✅ **Professional cursor management** with Wayland integration
- ✅ **Removed unnecessary libx11 dependencies**
- ✅ **2-second grace period** for smooth user interaction
- ✅ **Hyprland optimization** with `hyprctl` cursor control

### Building for Development
```bash
# Clean rebuild
make clean && make all

# Test individual screensavers
make toastersaver && ./build/toastersaver

# Build with verbose output
make V=1 toastersaver
```

### Contributing
- Issue tracker on GitHub
- Pull requests welcome
- SDL2 and Wayland expertise appreciated
- Focus on performance and visual quality

## 📄 Configuration Options

Most screensavers support:
- `-f 0|1`: Windowed (0) or fullscreen (1) mode
- `-s [float]`: Animation speed multiplier (0.1-5.0)
- `-h`: Display help

Text-based screensavers may have additional options:
- `-t [text]`: Display custom scrolling text

## 🤝 Acknowledgments

Inspired by classic screensavers from the After Dark era and modern Linux desktop innovation. Special thanks to the SDL2 and Hyprland communities for their excellent Wayland and graphics implementations.

---

**Experience the nostalgia of classic screensavers with modern Linux reliability!** 🎨✨🚀
