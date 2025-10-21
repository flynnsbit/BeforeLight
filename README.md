# 📦 BeforeLight: After Dark Screensaver Revival in C

![BeforeLight Logo](https://via.placeholder.com/800x200?text=BeforeLight+-%20After+Dark+Revival)

> **Nostalgic Computing Classic**: Fully authentic reproductions of After Dark's legendary screensavers, rewritten from CSS/HTML to high-performance C + SDL2

## 🎮 Available Screensavers

### 🦑 Fish Aquarium
*Authentic underwater ecosystem animation*

- **10 species** of animated fish with realistic swimming patterns
- **Dynamic bubbles** rising from the seafloor
- **Multi-directional movement** (left/right with flip animations)
- **Oceanic seabed** background tiling
- **Runtime Controls**: `--t` (fish count), `--f` (fullscreen toggle)

### 🌧️ Hard Rain
*Mesmerizing droplet cascade with physics*

- **30 raindrop particles** growing and shrinking
- **Staggered animations** for realistic wave patterns
- **Dynamic RGB coloring** with smooth color transitions
- **Pixel-perfect circular rendering**
- **Runtime Controls**: Speed multiplier (`--s`)

### 🏀 Bouncing Balls
*Full physics particle simulation*

- **10 colorful balls** with elastic collisions
- **Realistic momentum conservation** and kinetic energy preservation
- **Random initial velocities and positions**
- **Wall bounce mechanics** with energy dampening
- **Runtime Controls**: Speed scaling (`--s`)

### 🍞 Flying Toasters
*Classic 90s toast rebellion*

- **Original After Dark toasters** with authentic mechanics
- **Flying animations** and particle effects
- **Iconic before-dark rust aesthetic**

## 🚀 Quick Start

### Prerequisites
- Linux/macOS (or WSL)
- SDL2 development libraries
- GCC compiler

```bash
# Ubuntu/Debian dependencies
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-gfx-dev
```

### Build All
```bash
make all  # Builds fishsaver, hardrain, bouncingball, toastersaver
```

### Individual Builds
```bash
# Fish Aquarium
gcc -Wall -Wextra `sdl2-config --cflags --libs` -lSDL2_image -o fishsaver main_fish.c

# Hard Rain
gcc -Wall -Wextra `sdl2-config --cflags --libs` -lSDL2_image -o hardrain main_hard_rain.c

# Bouncing Balls
gcc -Wall -Wextra `sdl2-config --cflags --libs` -lSDL2_image -o bouncingball main_bouncing_ball.c

# Flying Toasters
gcc -Wall -Wextra `sdl2-config --cflags --libs` main.c -o toastersaver
```

### Run

```bash
# Start your favorite screensaver
./fishsaver -f 1    # Fullscreen aquarium
./hardrain -s 2     # Fast rain
./bouncingball      # Windowed physics balls

# Exit: Press ESC, Q, or click inside window
```

## 🎨 Technology & Implementation

### Core Technologies
- **C99** - Optimized, low-level performance
- **SDL2** - Cross-platform graphics, input, audio
- **SDL2_Image** - PNG/GIF texture loading
- **Mathematics** - Trigonometric animations, physics calculations
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
