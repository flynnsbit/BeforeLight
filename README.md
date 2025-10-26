# 📦 BeforeLight: After Dark Screensaver Revival in C

![BeforeLight Logo](https://via.placeholder.com/800x200?text=BeforeLight+-%20After+Dark+Revival)

> **Nostalgic Computing Classic**: **16 fully authentic reproductions** of After Dark's legendary screensavers, rewritten from CSS/HTML to high-performance C + SDL2

## 🎮 Available Screensavers

### 🦑 Fish Aquarium
*Authentic underwater ecosystem animation*

- **10 species** of animated fish with realistic swimming patterns
- **Dynamic bubbles** rising from the seafloor
- **Multi-directional movement** (left/right with flip animations)
- **Oceanic seabed** background tiling
- **Runtime Controls**: `-t` (fish count), `-f` (fullscreen toggle)

### 🌧️ Hard Rain
*Mesmerizing droplet cascade with physics*

- **30 raindrop particles** growing and shrinking
- **Staggered animations** for realistic wave patterns
- **Dynamic RGB coloring** with smooth color transitions
- **Pixel-perfect circular rendering**
- **Runtime Controls**: `-s` speed multiplier, `-f` fullscreen toggle

### 🌍 Earth Globe
*Bouncing spinning planet Earth*

- **240px animated globe** bouncing across screen edges
- **Spinning world map** with 21-frame rotation animation
- **Bounces in all directions** (right-left, bottom-top)
- **Sprite-sheet rendering** for smooth planetary rotation
- **Runtime Controls**: `-s` speed multiplier, `-f` fullscreen toggle

### 🏀 Bouncing Balls
*Full physics particle simulation*

- **10 colorful balls** with elastic collisions
- **Realistic momentum conservation** and kinetic energy preservation
- **Random initial velocities and positions**
- **Wall bounce mechanics** with energy dampening
- **Runtime Controls**: `-s` speed scaling, `-f` fullscreen toggle

### ✨ Stardust Warp
*Cosmic tunnel starfield effect*

- **Multi-layered star fields** with different speeds
- **Depth of field simulation** through opacity changes
- **Smooth scaling transitions** for 3D tunnel effect
- **17 layered star field** animations with timed delays
- **Runtime Controls**: `-s` speed multiplier, `-f` fullscreen toggle

### 📝 Scrolling Messages
*Animated marquee text display*

- **Typewriter-text scrolling** across screen with physics
- **Random inspirational quotes** from online API
- **Alternating vertical positioning** for varied movement
- **TTF font rendering** with fallbacks for maximum compatibility
- **Runtime Controls**: `-s` speed, `-f` fullscreen, `-t` custom text, `-r` random quotes

### 🔄 Messages 2
*Bouncing marquee text with physics*

- **Horizontal scrolling text** combined with vertical bouncing
- **Realistic physics** for text panel movement and collision
- **Identical message options** as Messages but with added vertical motion
- **Wall bouncing mechanics** with elastic properties
- **Runtime Controls**: `-s` speed, `-f` fullscreen, `-t` custom text, `-r` random quotes

### 🍞 Flying Toasters
*Classic 90s toast rebellion*

- **Original After Dark toasters** with authentic mechanics
- **Flying animations** and particle effects**
- **Iconic before-dark rust aesthetic**
- **Multi-toaster orchestration** with toast pieces
- **Runtime Controls**: `-s` speed, `-f` fullscreen, `-t` toaster count, `-m` toast count

### 🎨 Logo Morph
*Transforming logo animation*

- **Scaling and rotation** morphing effects over time
- **Elastic physics** for bouncing logo movement
- **50-second animation cycles** with smooth keyframes
- **Transform mixing** (scale, rotate, translate simultaneously)
- **Runtime Controls**: `-s` speed multiplier, `-f` fullscreen toggle

### ⛈️ Rainstorm
*Weather simulation with lightning*

- **Angled rain physics** with 15-degree slant simulation
- **Variable rain length** and dynamic speed changes
- **Lightning flash effects** with black screen intermissions
- **150 raindrop particles** for density depth
- **Runtime Controls**: `-s` speed multiplier, `-f` fullscreen toggle

### 🔦 Maglite Spotlight
*Moving spotlight on desktop capture*

- **Real screen capture** using X11 integration
- **Smooth spotlight movement** with edge boundary constraints
- **Radial gradient beam** with adjustable radius and brightness
- **Dynamic velocity changes** for realistic spotlight mechanics
- **Runtime Controls**: `-s` speed, `-f` fullscreen

### 🌟 Life Forms
*Star constellation creature formation*

- **4 distinct life forms** (bear, fish, bird, human) using star patterns
- **Multi-phase animation** (scatter → connect → hold → dissolve)
- **Constellation creation** through 35+ vertex coordinate systems
- **Smooth star interpolation** for realistic formation timelines
- **Runtime Controls**: `-s` animation speed, `-f` display mode

### 🏙️ Cityscape
*Authentic yellow buildings with blinking apartment windows*

- **15 yellow buildings** with randomized widths and heights
- **4x4 pixel apartment windows** that light up randomly
- ** Staggered blinking animation** maintaining 20-40% illumination
- **0.5-2 second toggle cycles** for realistic apartment lighting
- **Mustard yellow facades** (#FFC107) exactly matching reference
- **Runtime Controls**: `-s` simulation speed, `-f` fullscreen toggle

### 🌑 Fade Out
*Expanding black hole screen absorption*

- **Screen capture integration** with desktop content absorption
- **Radial expansion** starting from screen center point
- **Dual fading mechanism** (circular wipe + opacity reduction)
- **Dynamic tendrils** adding visual complexity to absorption
- **Runtime Controls**: `-s` expansion velocity, `-f` display preferences

### 💊 The Matrix
*Iconic digital rain character streams*

- **190+ simultaneous streams** of cascading character columns
- **Mixed character sets** (Japanese katakana, symbols, alphanumeric)
- **Individual stream physics** with variable speed and positioning
- **Brightness fade effects** for authentic falling text appearance
- **Runtime Controls**: `-s` precipitation density, `-f` screen coverage mode

### 🎲 Randomizer
*Automated screensaver cycling system*

- **Automatic sequencing** through complete BeforeLight ecosystem
- **Subprocess orchestration** using fork/exec process management
- **Transition identification** displays current active screensaver
- **Dynamic cycle periods** ranging from 10 to 45 second intervals
- **Runtime Controls**: `-s` transition duration, `-d` screen time allocation, `-f` display styling, `-r` name visibility

## 🚀 Quick Start

### Prerequisites
- Linux (Ubuntu, Arch, or distributions with SDL2 support)
- GCC compiler
- SDL2 development libraries with additional components:
  - SDL2 (core graphics)
  - SDL2_image (for PNG/GIF textures)
  - SDL2_ttf (for text-based screensavers: matrix, messages, messages2, randomizer)
- X11 libraries (for screen capture screensavers: spotlight, fadeout)
- ncurses (for configuration tool)
- Monospace fonts (DejaVu Sans Mono, Liberation Mono, or FreeMono for matrix screensaver)

#### Ubuntu/Debian Installation
```bash
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libx11-dev libncurses-dev
# Font packages (for matrix screensaver)
sudo apt-get install fonts-dejavu fonts-liberation fonts-freefont-ttf
```

#### Arch Linux Installation
```bash
# Base dependencies (required for all screensavers)
sudo pacman -S sdl2 sdl2_image

# Text rendering (required for matrix, messages, messages2, randomizer)
sudo pacman -S sdl2_ttf ttf-dejavu ttf-liberation gnu-free-fonts

# Screen capture (required for spotlight, fadeout screensavers)
sudo pacman -S libx11

# Configuration tool
sudo pacman -S ncurses
```

**Platform Notes:**
- The matrix screensaver requires monospace fonts. Install at least one of: `ttf-dejavu` (recommended), `ttf-liberation`, or `gnu-free-fonts`.
- Screen capture screensavers (spotlight, fadeout) require X11 and won't work on pure Wayland systems like Hyprland without Xwayland compatibility layer.

### Build All
```bash
make all  # Builds all 16 screensavers to build/ directory
```

### Individual Build Examples
```bash
# Core Classics
make fishsaver      # Aquarium with swimming fish
make hardrain       # Rainfall physics simulation
make bouncingball   # Newtonian physics balls
make globe          # Spinning bouncing Earth
make warp           # Starfield tunnel effect

# Enhanced Features
make messages       # Scrolling text marquee
make toastersaver   # Classic flying toasters
make logo           # Morphing logo animation
make rainstorm      # Weather with lightning
make spotlight      # Desktop screen capture
make cityscape      # Yellow buildings with blinking windows

# Special Effects
make lifeforms      # Star constellation formation
make fadeout        # Expanding black hole
make matrix         # Iconic digital rain
make randomizer     # Cycles through all savers
```

### Run Examples

```bash
# Classic After Dark Reproductions
./build/fishsaver -f 1    # Fullscreen underwater aquarium
./build/hardrain -s 2     # Fast animated rainfall
./build/bouncingball      # Windowed Newtonian physics
./build/globe -f 1        # Bouncing spinning Earth
./build/toastersaver -f 1 # Classic flying toasters

# Enhanced Text Effects
./build/messages -r -f 1  # Random inspirational quotes
./build/messages2 -t "Hello World" -f 1  # Bouncing custom text

# Advanced Visual Effects
./build/spotlight -f 1    # Spotlight on desktop capture
./build/fadeout -s 1.5 -f 1  # Slow black hole expansion
./build/matrix -f 1       # Digital character rain
./build/lifeforms -f 1    # Constellation formations

# Atmospheric Screensavers
./build/warp -f 1         # Starfield tunnel effect
./build/rainstorm -f 1    # Weather with lightning
./build/logo -f 1         # Morphing logo animation
./build/cityscape -f 1    # Yellow buildings with blinking windows

# Meta Screensaver
./build/randomizer -d 30 -f 1  # Cycles all screensavers every 30s

# Exit: Press ESC, Q, or click inside any window
```

## 🎨 Technology & Implementation

### Core Technologies
- **C99** - Optimized, low-level performance
- **SDL2** - Cross-platform graphics, input, audio
- **SDL2_Image & SDL2_ttf** - PNG/GIF texture loading, TrueType font rendering
- **Mathematics** - Trigonometric animations, physics calculations
- **X11 Integration** - Native Linux screen capture capabilities
- **SDL_Renderer** - Hardware-accelerated graphics pipeline

### Code Architecture
- **Entity-Component System**: Reusable animation/state management
- **Event-Driven Loop**: 60 FPS smooth rendering with sync
- **Asset Integration**: Direct texture loading and sprite animation
- **Physics Engine**: Real-time collision detection and response
- **Modular Design**: Clean separation of screensaver implementations

### Performance Features
- **Framerate Independence**: Physics updates decoupled from rendering
- **Memory Optimized**: Minimal allocations, efficient texture reuse
- **GPU Acceleration**: Hardware-blended graphics rendering
- **Thread-Safe**: Single-threaded design for stability

## 📜 History & Inspiration

**After Dark** was a groundbreaking screensaver addon from the 1990s that revolutionized computer entertainment. This project faithfully reproduces those classics in modern code, preserving the spirit of original Macintosh desktop design while leveraging contemporary programming techniques.

### Screen References
- **Fish Aquarium**: [Original After Dark CSS](https://www.bryanbraun.com/after-dark-css/scarebox/fish/) by Bryan Braun
- **Hard Rain**: [CSS Drop Particles](https://www.bryanbraun.com/after-dark-css/scarebox/hard-rain/)
- **Bouncing Ball**: Motion study of Newtonian frictionless systems
- **Flying Toasters**: Beloved kitchen appliance rebellion animation

## 🛠️ Development

### Contributing
1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Adding New Screensavers
1. Create `main_new_screensaver.c` following the existing patterns
2. Implement entity system, animation loops, rendering
3. Update Makefile with new build target
4. Test thoroughly across multiple resolutions

## 📄 Licensing

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

All After Dark assets and concepts remain property of their original creators. This is a faithful educational reproduction.

## 🙏 Acknowledgments

- **Berkeley Systems** - Creators of After Dark
- **Bryan Braun** - CSS reference implementations
- **SDL Development Community** - Amazing cross-platform graphics library
- **Macintosh Users** - Keepers of early computing culture

---

⭐ **Star this repo** if you enjoy retro computing vibes! Enjoy your screensaver revival! 💾🖥️✨
