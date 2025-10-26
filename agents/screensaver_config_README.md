# Screensaver Configuration Tool for Omarchy/Hyprland

A TUI (Text User Interface) application that allows users to easily select and configure screensavers for the omarchy desktop environment in Hyprland/Wayland.

## Features

- 🖥️ **Interactive Menu**: Navigate through all available screensavers with arrow keys and page keys
- 📖 **Dual-Pane Layout**: Left pane shows screensaver list, right pane displays detailed descriptions
- ⚙️ **Options Configuration**: Configure command-line options for compatible screensavers
- 👁️ **Preview Mode**: Test screensavers for 10 seconds without changing the default
- 🔄 **Backup & Restore**: Automatically backs up the original script and allows restoration
- 🎨 **Clean TUI Design**: Ncurses-based interface optimized for terminal compatibility
- ⌨️ **Full Keyboard Support**: Arrow keys, Page Up/Down, ESC, and letter shortcuts

## Available Screensavers

| Screensaver | Type | Configurable |
|-------------|------|--------------|
| starrynight | Celestial dome | Yes (-s, -d, -m, -r) |
| starfield_multilayer | Multi-layer stars | No |
| starfield_accumulation | Accumulating stars | No |
| starfield_trails | Moving star trails | No |
| starsclean | Static stars | No |
| static_flickering_stars | Flickering stars | No |
| fadeout | Cloud effects | No |
| hardrain | Heavy rain | No |
| rainstorm | Storm effects | No |
| fishsaver | Fish aquarium | No |
| globe | Rotating Earth | No |
| cityscape | Urban skyline | No |
| spotlight | Lighting effects | No |
| matrix | Digital rain | No |
| messages | Scrolling text | No |
| messages2 | Alternative messages | No |
| randomizer | Random effects | No |
| paperfire | Paper burning | No |
| toastersaver | Flying toasters | No |
| lifeforms | Conway's Game of Life | No |
| logo | Logo display | No |
| bouncingball | Bouncing ball | No |
| warp | Warp effects | No |

## Installation & Usage

### Prerequisites
```bash
# Required packages
sudo pacman -S ncurses gcc make
```

### Building
```bash
# Compile to build directory
gcc -Wall -Wextra -O2 -o build/screensaver_config screensaver_config.c -lncurses -lm

# Make executable
chmod +x ./build/screensaver_config
```

### Running the Configuration Tool
```bash
./build/screensaver_config
```

### Controls
- **↑↓ Arrow Keys**: Navigate through screensaver list
- **Enter**: Select current highlighted screensaver
- **C**: Configure options for configurable screensavers
- **P**: Preview selected screensaver (10-second test run)
- **R**: Restore to original/default screensaver
- **Q**: Quit the application

## How It Works

### Script Management
The tool modifies `/home/shawn/.local/share/omarchy/bin/omarchy-cmd-screensaver`:

1. **Official Backup**: Downloads the original omarchy screensaver script from the official GitHub repo
2. **Selection**: Generates a new bash script that launches the selected screensaver
3. **Environment**: Sets `SDL_VIDEODRIVER=wayland` for proper Wayland integration
4. **Process Management**: Handles proper cleanup and hyprland cursor visibility

### Preview Mode
- Launches screensaver with `timeout 10s` for safe testing
- Independent process management (won't interfere with selection)
- Automatic cleanup after preview period

## Technical Details

### Screen Resolution
- Automatically detects desktop resolution via SDL2
- Works with fullscreen Hyprland windows

### Error Handling
- Validates build directory existence
- Checks for original script presence
- Handles missing screensaver executables

### Limitations
- Currently only `starrynight` has full configuration support
- Preview mode doesn't capture screensaver stdout/stderr
- Ncurses interface (future versions could use graphical equivalent)

## Development Status

This is a functional TUI prototype implementing the core requirements:
- ✅ Screensaver selection with emoji indicators
- ✅ Backup/restore functionality
- ✅ Script modification system
- ✅ Preview capability
- ✅ Basic configuration for starring
- ✅ Ncurses-based TUI interface

## Future Enhancements

- **Dynamic Option Parsing**: Automatically detect and configure options from `-h` help output
- **Graphical Interface**: GUI version using Qt or GTK
- **Profile Management**: Save/load configuration profiles
- **Theme Support**: Different TUI color schemes
- **Batch Operations**: Apply configurations to multiple monitors
- **Integration**: Direct hypridle configuration support

## Troubleshooting

### Common Issues
- **"Build directory not found"**: Ensure `make` has been run to compile screensavers
- **"No original screensaver script"**: Check omarchy installation is complete
- **Terminal artifacts after exit**: Run `reset` command to clear terminal state

### Dependencies
- **ncurses**: Required for TUI interface (`pacman -S ncurses`)
- **SDL2**: Required for screensaver execution
- **Wayland**: Must be running under Hyprland

## Files Created

- `screensaver_config.c`: Main source code
- `build/screensaver_config`: Compiled executable
- `screensaver_config_README.md`: This documentation
- Backup files: `*backup` created automatically

---

**Select your perfect screensaver with ease! 🎯✨**
