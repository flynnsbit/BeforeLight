/**
 * Screensaver Configuration Tool - TUI Application
 * Hyprland screensaver selector and configurator
 *
 * Compile: gcc -Wall -Wextra -O2 -o build/screensaver_config screensaver_config.c -lncurses -lm
 * Run: ./build/screensaver_config
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ncurses.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include "assets/official_script.h"

// Mouse event handling
MEVENT mouse_event;

// Official omarchy-cmd-screensaver content for restore default is now included from the header.

// Script paths - use dynamic paths based on current user
#define SCRIPT_PATH_ALTERNATE "$HOME/.config/omarchy/branding/screensaver/omarchy-cmd-screensaver"

// Function to get backup script path
char *get_backup_script_path() {
    static char path[512];
    char *home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.cache/omarchy-screensaver-backup", home);
    } else {
        strcpy(path, "/tmp/omarchy-screensaver-backup"); // fallback
    }
    return path;
}

// Screensaver catalog
typedef struct {
    char *name;
    char *emoji;
    char *type;
    char *description; // Detailed description for UI
    char options[256]; // Configured options string
} Screensaver;

// Global screensaver list
Screensaver savers[] = {
    {"starrynight",
     "⭐",
     "Star Field",
     "Dynamic celestial dome with realistic twinkle effects.\n\nFeatures meteor showers and astronomical accuracy.\nSupports configurable speed, density, and rotation modes.",
     ""},
    {"starsclean",
     "⭐",
     "Static Stars",
     "Clean, static starfield with authentic twinkling.\n\nFixed-position stars that simulate atmospheric\ndistortion effects like real celestial observation.",
     ""},
    {"fadeout",
     "🌫️",
     "Clouds",
     "Soft cloud patterns with gentle fade effects.\n\nCreates subtle, misty screen transitions perfect\nfor idle display protection.",
     ""},
    {"hardrain",
     "🌧️",
     "Heavy Rain",
     "Intense, realistic rain droplet animation.\n\nDynamic precipitation patterns with realistic\nwater physics and soothing audio-like effects.",
     ""},
    {"rainstorm",
     "🌧️",
     "Stormy Rain",
     "Dramatic storm effects with multi-layer movement.\n\nCreates intense weather atmosphere with\nmultiple animation layers and dramatic intensity.",
     ""},
    {"fishsaver",
     "🐟",
     "Fish Aquarium",
     "Animated aquatic life in realistic aquarium.\n\nColorful fish swim with natural movement\npatterns across the screen canvas.",
     ""},
    {"globe",
     "🌍",
     "Rotating Globe",
     "3D Earth spinning in orbital space view.\n\nRealistic planetary rotation with detailed\nlandmass rendering and atmospheric effects.",
     ""},
    {"cityscape",
     "🏙️",
     "City Skyline",
     "Urban night landscape with twinkling lights.\n\nCreates evening city view with building\nsilhouettes and authentic night lighting.",
     ""},
    {"spotlight",
     "🔦",
     "Lighting Effect",
     "Dynamic theatrical spotlight beams.\n\nMoving light effects create dramatic\natmospheric scenes across the display.",
     ""},
    {"matrix",
     "⏯️",
     "Digital Rain",
     "Classic green matrix falling characters.\n\nAuthentic digital rain effect with scrolling\nalphanumeric streams in traditional green tint.",
     ""},
    {"messages",
     "💬",
     "Scrolling Text",
     "Animated message display with text scrolling.\n\nConfigurable text notifications and system\nmessages scrolling across the screen.",
     ""},
    {"messages2",
     "💬",
     "Alt Messages",
     "Alternative scrolling text with varied effects.\n\nAlternative messaging system with different\nanimation styles and presentation modes.",
     ""},
    {"randomizer",
     "🔄",
     "Random Effects",
     "Generates various random visual patterns.\n\nCycles through different algorithmic\neffects and random animation styles.",
     ""},
    {"paperfire",
     "🎆",
     "Paper Fire",
     "Realistic fire animation on paper surfaces.\n\nAccurate flame propagation effects with\nburning paper physics simulation.",
     ""},
    {"toastersaver",
     "🍞",
     "Flying Toasters",
     "Nostalgic flying toast screensaver.\n\nClassic computer-era animated bread products\nflying through retro space background.",
     ""},
    {"lifeforms",
     "🦠",
     "Conway's Game",
     "Implementation of Conway's Game of Life.\n\nMathematical cellular automata that evolve\naccording to simple life rules.",
     ""},
    {"logo",
     "🏷️",
     "Logo Display",
     "Static or animated brand logo presentation.\n\nCompany branding display with customizable\nstatic or animated visual elements.",
     ""},
    {"bouncingball",
     "⚽",
     "Bouncing Ball",
     "Physics-based bouncing ball animation.\n\nRealistic gravity simulation with momentum\nand collision effects across display area.",
     ""},
    {"warp",
     "💫🌊",
     "Warp Effects",
     "Fluid distortion and liquid warping effects.\n\nPsychedelic surface distortions with organic\nmovement patterns and wave-like animations.",
     ""}
};

#define NUM_SAVERS (sizeof(savers) / sizeof(Screensaver))

// Global screen dimensions (set in main)
int global_max_y = 0;
int global_max_x = 0;

// Current selections
int selected_index = 0;
int in_preview_mode = 0;
pid_t preview_pid = -1;

// Forward declarations
void draw_menu(WINDOW *list_win, WINDOW *desc_win, int highlight);
void configure_screensaver(int index);
void preview_screensaver(int index);
void select_screensaver(int index);
void restore_default();

// Get executable path for screensaver
char *get_screensaver_path(const char *name) {
    static char path[2048];
    char build_dir[1024];
    char *home = getenv("HOME");
    if (home) {
        int build_len = snprintf(build_dir, sizeof(build_dir), "%s/.config/omarchy/branding/screensaver", home);
        if (build_len < (int)sizeof(build_dir) - 2) {  // -2 for safety with /
            path[0] = '\0';
            strncat(path, build_dir, sizeof(path) - 2);
            strncat(path, "/", sizeof(path) - strlen(path) - 1);
            strncat(path, name, sizeof(path) - strlen(path) - 1);
        } else {
            // Path too long - use fallback
            strcpy(path, "/tmp/screensaver-fallback");
        }
    } else {
        // Fallback if HOME not set
        strcpy(path, "/tmp/screensaver-fallback");
    }
    return path;
}

// Check if file exists
int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

// Extract help options from starrynight
void parse_starrynight_options(void) {
    // Would need to run './build/starrynight -h' and parse output in real app
    // For now, hardcode based on known options
    strcpy(savers[0].options, "-s 1.0 -d 0.5 -m 1.0 -r dynamic");
}

void write_screensaver_script(const char *path, const char *options) {
    // Create directory if it doesn't exist
    char script_path_expanded[512];
    snprintf(script_path_expanded, sizeof(script_path_expanded), "%s/.config/omarchy/branding/screensaver/omarchy-cmd-screensaver", getenv("HOME"));

    char dirname[512];
    strcpy(dirname, script_path_expanded);
    char *last_slash = strrchr(dirname, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(dirname, 0755);  // Create directory if it doesn't exist
    }

    FILE *fp = fopen(script_path_expanded, "w");
    if (!fp) {
        perror("Failed to write screensaver script");
        return;
    }

    fprintf(fp, "#!/bin/bash\n\n");
    fprintf(fp, "# Parse arguments\n");
    fprintf(fp, "LAUNCH_MODE=0\n");
    fprintf(fp, "if [[ \"$1\" == \"launch\" ]]; then\n");
    fprintf(fp, "  LAUNCH_MODE=1\n");
    fprintf(fp, "fi\n\n");
    fprintf(fp, "# Set cursor to invisible while screensaver is running\n");
    fprintf(fp, "hyprctl keyword cursor:invisible true &>/dev/null\n\n");
    fprintf(fp, "# Launch the selected screensaver\n");
    fprintf(fp, "SDL_VIDEODRIVER=wayland %s %s >/dev/null 2>&1 &\n", path, options);
    fprintf(fp, "SAVER_PID=$!\n\n");
    fprintf(fp, "# Function to check if screensaver window is in focus\n");
    fprintf(fp, "screensaver_in_focus() {\n");
    fprintf(fp, "  hyprctl activewindow -j | jq -e '.class == \"Screensaver\"' >/dev/null 2>&1\n");
    fprintf(fp, "}\n\n");
    fprintf(fp, "# Function to exit screensaver: restore cursor and cleanup\n");
    fprintf(fp, "exit_screensaver() {\n");
    fprintf(fp, "  hyprctl keyword cursor:invisible false 2>/dev/null\n");
    fprintf(fp, "  pkill -x tte 2>/dev/null\n");
    fprintf(fp, "  pkill -f \"alacritty --class Screensaver\" 2>/dev/null\n");
    fprintf(fp, "  exit 0\n");
    fprintf(fp, "}\n\n");
    fprintf(fp, "# Set trap to exit cleanly\n");
    fprintf(fp, "trap exit_screensaver INT TERM HUP QUIT\n\n");
    fprintf(fp, "# Monitor screensaver: check if process exists and if window is focused\n");
    fprintf(fp, "while true; do\n");
    fprintf(fp, "  if [[ $LAUNCH_MODE -eq 1 ]]; then\n");
    fprintf(fp, "    # Launched via omarchy-launch-screensaver: only check if saver process exists\n");
    fprintf(fp, "    if ! kill -0 $SAVER_PID 2>/dev/null; then\n");
    fprintf(fp, "      exit_screensaver\n");
    fprintf(fp, "    fi\n");
    fprintf(fp, "  else\n");
    fprintf(fp, "    # Direct launch: check both focus and process\n");
    fprintf(fp, "    if ! screensaver_in_focus || ! kill -0 $SAVER_PID 2>/dev/null; then\n");
    fprintf(fp, "      exit_screensaver\n");
    fprintf(fp, "    fi\n");
    fprintf(fp, "  fi\n");
    fprintf(fp, "  sleep 1\n");
    fprintf(fp, "done\n");

    fclose(fp);
    chmod(script_path_expanded, 0755);
}

void select_screensaver(int index) {
    // Get the screensaver path and options
    char *path = get_screensaver_path(savers[index].name);

    // Write the new screensaver script
    write_screensaver_script(path, savers[index].options[0] ? savers[index].options : "");

    // Create a message window
    WINDOW *select_win = newwin(10, COLS-4, (LINES-10)/2, 2);
    box(select_win, 0, 0);
    mvwprintw(select_win, 1, 2, "Screensaver Selected: %s %s",
             savers[index].emoji, savers[index].name);
    mvwprintw(select_win, 3, 2, "Script updated successfully!");
    mvwprintw(select_win, 4, 2, "Command: SDL_VIDEODRIVER=wayland %s %s", path,
             savers[index].options[0] ? savers[index].options : "");
    mvwprintw(select_win, 6, 2, "The screensaver will now use this selection.");
    mvwprintw(select_win, 8, 2, "Press any key to continue...");
    wrefresh(select_win);
    wgetch(select_win);
    delwin(select_win);
}

void restore_default() {
    char script_path[512];
    snprintf(script_path, sizeof(script_path), "%s/.config/omarchy/branding/screensaver/omarchy-cmd-screensaver", getenv("HOME"));

    FILE *fp = fopen(script_path, "w");
    if (!fp) {
        perror("Failed to restore default screensaver script");
        return;
    }

    fprintf(fp, "%s", ORIGINAL_OFFICIAL_SCRIPT_CONTENT);
    fclose(fp);
    chmod(script_path, 0755);  // Make executable
}

void preview_screensaver(int index) {
    // Kill any existing preview process
    if (preview_pid > 0) {
        kill(preview_pid, SIGTERM);
        waitpid(preview_pid, NULL, WNOHANG);  // Don't block waiting
        preview_pid = -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process - run the screensaver preview
        char *path = get_screensaver_path(savers[index].name);
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "bash -c 'SDL_VIDEODRIVER=wayland timeout 10s %s %s'", path, savers[index].options);
        system(cmd);
        exit(0);
    } else {
        // Parent process for preview - save PID for cleanup
        preview_pid = pid;
        // Don't wait - let it run asynchronously
    }
}

#define MAX_CONFIG_OPTIONS 10

typedef struct {
    char name[32];
    char type; // 'f' for float, 's' for string, 'b' for bool
    char description[64];
    float min_val, max_val, default_val;
    char string_val[256];
    int bool_val;
} ConfigOption;

// Configuration definitions for each screensaver
ConfigOption starrynight_opts[] = {
    {"speed", 'f', "Animation speed multiplier", 0.1, 5.0, 1.0, "", 0},
    {"density", 'f', "Star density (0.0-1.0)", 0.0, 1.0, 0.5, "", 0},
    {"meteors", 'f', "Meteor frequency multiplier", 0.0, 5.0, 1.0, "", 0},
    {"rotation", 'i', "Celestial rotation mode (0=dynamic,1=static,2=none)", 0, 2, 0, "", 0}
};

ConfigOption messages_opts[] = {
    {"text", 's', "Scroll text to display", 0, 0, 0, "", 0}  // String input
};

ConfigOption messages2_opts[] = {
    {"text", 's', "Scroll text to display", 0, 0, 0, "", 0}  // String input
};

void configure_screensaver(int index) {
    // Define options available for each screensaver
    ConfigOption *opts = NULL;
    int num_opts = 0;
    char title[64];

    if (strcmp(savers[index].name, "starrynight") == 0) {
        opts = starrynight_opts;
        num_opts = sizeof(starrynight_opts) / sizeof(ConfigOption);
        sprintf(title, "Configure ⭐ %s", savers[index].name);
    } else if (strcmp(savers[index].name, "messages") == 0) {
        opts = messages_opts;
        num_opts = sizeof(messages_opts) / sizeof(ConfigOption);
        sprintf(title, "Configure 💬 %s", savers[index].name);
    } else if (strcmp(savers[index].name, "messages2") == 0) {
        opts = messages2_opts;
        num_opts = sizeof(messages2_opts) / sizeof(ConfigOption);
        sprintf(title, "Configure 💬 %s", savers[index].name);
    } else {
        // Non-configurable screensaver
        WINDOW *msg_win = newwin(8, 40, (LINES-8)/2, (COLS-40)/2);
        box(msg_win, 0, 0);
        mvwprintw(msg_win, 1, 2, "%s %s", savers[index].emoji, savers[index].name);
        mvwprintw(msg_win, 3, 2, "No configuration options");
        mvwprintw(msg_win, 4, 2, "available for this screensaver.");
        mvwprintw(msg_win, 6, 2, "Press any key to continue...");
        wrefresh(msg_win);
        wgetch(msg_win);
        delwin(msg_win);
        return;
    }

    // Parse current options to initialize values
    for (int i = 0; i < num_opts; i++) {
        if (savers[index].options[0]) {
            // Parse existing options string and set values
            char *opt_str = savers[index].options;
            char search[32];
            sprintf(search, "-%c", opts[i].name[0]);

            if (opts[i].type == 'f' || opts[i].type == 'i') {
                char *pos = strstr(opt_str, search);
                if (pos) {
                    float val = atof(pos + 2);
                    if (opts[i].type == 'f') {
                        opts[i].default_val = val;
                    }
                }
            } else if (opts[i].type == 'c') {
                if (strstr(opt_str, "static")) opts[i].default_val = 1;
                else if (strstr(opt_str, "none")) opts[i].default_val = 2;
                else opts[i].default_val = 0; // dynamic
            }
        }
    }

    // Create configuration window
    WINDOW *config_win = newwin(LINES-4, COLS-4, 2, 2);
    box(config_win, 0, 0);
    mvwprintw(config_win, 1, 2, "%s", title);

    int current_opt = 0, done = 0;
    int ch;

    while (!done) {
        // Clear and redraw options
        werase(config_win);
        box(config_win, 0, 0);
        mvwprintw(config_win, 1, 2, "%s", title);

        for (int i = 0; i < num_opts; i++) {
            if (i == current_opt) wattron(config_win, A_REVERSE);

            int y_pos = 3 + i * 2;
            mvwprintw(config_win, y_pos, 4, "%s:", opts[i].name);

            if (opts[i].type == 'f') {
                mvwprintw(config_win, y_pos, 20, "%.1f (range: %.1f-%.1f)",
                         opts[i].default_val, opts[i].min_val, opts[i].max_val);
            } else if (opts[i].type == 'c') {
                char *mode_names[] = {"dynamic", "static", "none"};
                mvwprintw(config_win, y_pos, 20, "%s", mode_names[(int)opts[i].default_val]);
            } else if (opts[i].type == 's') {
                mvwprintw(config_win, y_pos, 20, "[%s]", opts[i].string_val);
            }

            mvwprintw(config_win, y_pos+1, 6, "%s", opts[i].description);

            if (i == current_opt) wattroff(config_win, A_REVERSE);
        }

        mvwprintw(config_win, LINES-7, 4, "Use ↑↓ to navigate, +/- to adjust values, Enter/text for strings");
        mvwprintw(config_win, LINES-6, 4, "S: Save configuration | Esc/Ctrl+C: Cancel");

        wrefresh(config_win);

        ch = wgetch(config_win);
        float step = 0.1f;
        switch (ch) {
            case KEY_UP:
                if (current_opt > 0) current_opt--;
                break;
            case KEY_DOWN:
                if (current_opt < num_opts - 1) current_opt++;
                break;
            case '+':
            case '=':
                if (opts[current_opt].type == 'f' || opts[current_opt].type == 'i') {
                    opts[current_opt].default_val += step;
                    if (opts[current_opt].default_val > opts[current_opt].max_val)
                        opts[current_opt].default_val = opts[current_opt].max_val;
                } else if (opts[current_opt].type == 'c') {
                    opts[current_opt].default_val = ((int)opts[current_opt].default_val + 1) % 3;
                }
                break;
            case '-':
                if (opts[current_opt].type == 'f' || opts[current_opt].type == 'i') {
                    opts[current_opt].default_val -= step;
                    if (opts[current_opt].default_val < opts[current_opt].min_val)
                        opts[current_opt].default_val = opts[current_opt].min_val;
                } else if (opts[current_opt].type == 'c') {
                    opts[current_opt].default_val = ((int)opts[current_opt].default_val + 2) % 3;
                }
                break;
            case KEY_ENTER:
            case '\n':
                if (opts[current_opt].type == 's') {
                    // Text input mode
                    mvwprintw(config_win, LINES-5, 4, "Enter text (256 chars max):");
                    wmove(config_win, LINES-4, 4);
                    echo();
                    wgetnstr(config_win, opts[current_opt].string_val, 255);
                    noecho();
                    werase(config_win);
                } else {
                    // Accept and continue
                }
                break;
            case 's':
            case 'S':
                // Save configuration
                char cmd_opts[256] = "";
                for (int i = 0; i < num_opts; i++) {
                    char opt_str[64];
                    if (opts[i].type == 'f') {
                        sprintf(opt_str, " -%c %.1f", opts[i].name[0], opts[i].default_val);
                        strcat(cmd_opts, opt_str);
                    } else if (opts[i].type == 'c') {
                        char *mode_names[] = {"dynamic", "static", "none"};
                        sprintf(opt_str, " -r %s", mode_names[(int)opts[i].default_val]);
                        strcat(cmd_opts, opt_str);
                    } else if (opts[i].type == 's' && opts[i].string_val[0]) {
                        // For text options, we might need to store them differently
                        // For now, just include as option if exists
                        sprintf(opt_str, " \"%s\"", opts[i].string_val);
                        strcat(cmd_opts, opt_str);
                    }
                }
                strcpy(savers[index].options, cmd_opts + 1); // Remove leading space
                done = 1;
                break;
            case 27:  // ESC
            case 'q':
            case 'Q':
                // Cancel - don't save changes
                done = 1;
                break;
        }
    }

    delwin(config_win);
}

void draw_menu(WINDOW *list_win, WINDOW *desc_win, int highlight) {
    // Clear the list pane completely
    werase(list_win);

    int y = 1;
    int max_visible = 20; // Show 20 items at once
    int start_idx = (highlight / max_visible) * max_visible;

    // Ensure start_idx doesn't go beyond available items
    if (start_idx + max_visible > (int)NUM_SAVERS) {
        start_idx = (int)NUM_SAVERS - max_visible;
        if (start_idx < 0) start_idx = 0;
    }

    // Title
    mvwprintw(list_win, y++, 2, "Screensaver Configuration Tool");

    // Draw items
    for (int i = start_idx; i < (int)NUM_SAVERS && i < start_idx + max_visible; i++) {
        if (i == highlight) wattron(list_win, A_REVERSE);

        char short_name[32];
        strcpy(short_name, savers[i].name);
        if (strlen(short_name) > 25) strcpy(short_name + 22, "..."); // Leave room for emoji and numbering

        mvwprintw(list_win, y++, 2, "%d. %s %s", i+1, savers[i].emoji, short_name);

        if (i == highlight) wattroff(list_win, A_REVERSE);
    }

    // Consolidate instructions to save space - include vim keys
    mvwprintw(list_win, LINES-3, 2, "Nav: ↑↓hjkl PgUp/PgDn gg/G Ctrl+U/D | Select: ENTER (keyboard only) | Config: C | Preview: P | Restore Default: R | Quit: Q");

    // Draw border last to avoid overwriting by clearing
    box(list_win, 0, 0);
    wrefresh(list_win);

    // Clear the description pane completely
    werase(desc_win);

    // Draw right pane - description of selected item
    box(desc_win, 0, 0);
    char desc_header[80];
    const char *config_status = savers[highlight].options[0] ? " [CONFIGURED]" : "";
    snprintf(desc_header, sizeof(desc_header), "%s %s %s",
             savers[highlight].emoji, savers[highlight].name, savers[highlight].type);
    mvwprintw(desc_win, 0, 2, "%s%s", desc_header, config_status);

    // Word-wrap the description text
    char *desc = savers[highlight].description;
    int desc_y = 2;
    int desc_x = 2;
    int list_width_local = global_max_x * 6 / 10;  // Recalculate for description pane
    int desc_width = global_max_x - list_width_local - 4;  // Available width minus borders and padding

    if (desc_width > 2) {
        char word[256];
        int word_pos = 0;
        char *p = desc;

        while (*p && desc_y < LINES - 5) {  // Leave room for bottom
            if (*p == ' ' || *p == '\n' || *p == '\0') {
                word[word_pos] = '\0';
                word_pos = 0;

                // Handle newline characters specially
                if (*p == '\n') {
                    desc_y++;
                    desc_x = 2;
                    p++;
                    continue;
                }

                if (desc_x + (int)strlen(word) > desc_width) {
                    desc_y++;
                    desc_x = 2;
                }

                if (desc_x == 2 || strlen(word) > 0) {
                    mvwprintw(desc_win, desc_y, desc_x, "%s", word);
                    desc_x += (int)strlen(word) + 1;
                }

                if (*p == '\0') break;
            } else {
                word[word_pos++] = *p;
            }
            p++;
        }

        // Handle any remaining word at end of string
        if (word_pos > 0 && desc_y < LINES - 5) {
            word[word_pos] = '\0';
            if (desc_x + (int)strlen(word) > desc_width) {
                desc_y++;
                desc_x = 2;
            }
            mvwprintw(desc_win, desc_y, desc_x, "%s", word);
        }

        // Show options if configured
        if (savers[highlight].options[0]) {
            mvwprintw(desc_win, desc_y + 2, 2, "Options: %s", savers[highlight].options);
        }

        // Show if configurable
        const char *configurable_text = (strcmp(savers[highlight].name, "starrynight") == 0) ?
                                       "(Configurable with C key)" : "(Non-configurable)";
        mvwprintw(desc_win, LINES - 3, 2, "%s", configurable_text);
    }

    wrefresh(desc_win);
}

int main() {
    // Set locale for UTF-8 emoji support
    setlocale(LC_ALL, "");

    // Check if backup script exists, download official if not
    char *backup_script_path = get_backup_script_path();
    if (!file_exists(backup_script_path)) {
        printf("Downloading official omarchy screensaver backup...\n");
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
                 "curl -s https://raw.githubusercontent.com/basecamp/omarchy/refs/heads/master/bin/omarchy-cmd-screensaver -o %s",
                 backup_script_path);
        system(cmd);

        if (!file_exists(backup_script_path)) {
            printf("Error: Failed to download official screensaver backup!\n");
            return 1;
        } else {
            printf("Official backup downloaded successfully.\n");
        }
    }

    // Initialize ncurses
    initscr();
    raw();  // Use raw() instead of cbreak() for better key handling
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // Enable mouse support
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    attron(COLOR_PAIR(1));

    // Get screen dimensions
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    global_max_y = max_y;
    global_max_x = max_x;
    #define LINES max_y
    #define COLS max_x

    // Verify build directory exists
    char build_dir_expanded[512];
    snprintf(build_dir_expanded, sizeof(build_dir_expanded), "%s/.config/omarchy/branding/screensaver", getenv("HOME"));
    if (!file_exists(build_dir_expanded)) {
        endwin();
        printf("Error: Build directory not found: %s\n", build_dir_expanded);
        return 1;
    }

    // Parse options for configurable screensavers
    parse_starrynight_options();

    // Split screen: 60% list, 40% description
    int list_width = max_x * 6 / 10;  // 60% for list
    int desc_width = max_x - list_width - 1;  // Remainder for description

    // Create two panes
    WINDOW *list_win = newwin(max_y, list_width, 0, 0);
    WINDOW *desc_win = newwin(max_y, desc_width, 0, list_width + 1);

    keypad(stdscr, TRUE); // Enable special keys on stdscr for all input
    keypad(list_win, TRUE);
    keypad(desc_win, TRUE);

    halfdelay(1); // Non-blocking input with 100ms timeout

    int ch, ch2;  // ch2 for secondary character input (vim gg handling)
    while (1) {
        draw_menu(list_win, desc_win, selected_index);

        ch = wgetch(stdscr);  // Get input from stdscr instead of list_win

        // Handle ERR (timeout) - continue normally
        if (ch == ERR) {
            continue;
        }

        // Handle mouse events (may not work in all Wayland terminals)
        if (ch == KEY_MOUSE) {
            if (getmouse(&mouse_event) == OK) {
                // Handle clicks in the list window (left pane) - check absolute screen coordinates
                int list_win_x, list_win_y;
                int list_win_width, list_win_height;
                getbegyx(list_win, list_win_y, list_win_x);
                getmaxyx(list_win, list_win_height, list_win_width);

                if (mouse_event.x >= list_win_x && mouse_event.x < list_win_x + list_win_width &&
                    mouse_event.y >= list_win_y && mouse_event.y < list_win_y + list_win_height &&
                    mouse_event.bstate & BUTTON1_CLICKED) {

                    // Calculate which item was clicked (relative to window)
                    int rel_y = mouse_event.y - list_win_y;

                    // Calculate which item was clicked
                    int max_visible = 20;
                    int start_idx = (selected_index / max_visible) * max_visible;
                    if (start_idx + max_visible > (int)NUM_SAVERS) {
                        start_idx = (int)NUM_SAVERS - max_visible;
                        if (start_idx < 0) start_idx = 0;
                    }

                    // Items start at window y=2 (0-indexed), title is y=1
                    if (rel_y >= 2) {
                        int item_index = (rel_y - 2) + start_idx;
                        if (item_index >= 0 && item_index < (int)NUM_SAVERS) {
                            selected_index = item_index;
                        }
                    }
                }
            }
            // Don't continue - allow normal key processing to proceed
        }

        switch (ch) {
            // Arrow key navigation
            case KEY_UP:
                if (selected_index > 0) selected_index--;
                break;
            case KEY_DOWN:
                if (selected_index < (int)NUM_SAVERS - 1) selected_index++;
                break;
            case KEY_LEFT:
                if (selected_index > 0) selected_index--;  // Same as up
                break;
            case KEY_RIGHT:
                if (selected_index < (int)NUM_SAVERS - 1) selected_index++;  // Same as down
                break;
            case KEY_PPAGE:  // Page Up
                selected_index -= 10;
                if (selected_index < 0) selected_index = 0;
                break;
            case KEY_NPAGE:  // Page Down
                selected_index += 10;
                if (selected_index >= (int)NUM_SAVERS) selected_index = NUM_SAVERS - 1;
                break;

            // Vim-style navigation
            case 'k':  // Vim up
                if (selected_index > 0) selected_index--;
                break;
            case 'j':  // Vim down
                if (selected_index < (int)NUM_SAVERS - 1) selected_index++;
                break;
            case 'h':  // Vim left (same as up)
                if (selected_index > 0) selected_index--;
                break;
            case 'l':  // Vim right (same as down)
                if (selected_index < (int)NUM_SAVERS - 1) selected_index++;
                break;
            case 'g':  // Potential 'gg' start (two 'g's needed)
                // Wait for second 'g'
                ch2 = wgetch(stdscr);
                if (ch2 == 'g') {
                    selected_index = 0;  // Go to top
                }
                break;
            case 'G':  // Vim bottom
                selected_index = NUM_SAVERS - 1;
                break;
            case 21:  // Ctrl+U (0x15) - half page up
                selected_index -= 10;
                if (selected_index < 0) selected_index = 0;
                break;
            case 4:   // Ctrl+D (0x04) - half page down
                selected_index += 10;
                if (selected_index >= (int)NUM_SAVERS) selected_index = NUM_SAVERS - 1;
                break;

            case '\n':
            case KEY_ENTER:
                select_screensaver(selected_index);
                // Show selection confirmation
                mvwprintw(list_win, LINES-2, 2, "Selected: %s - Press Q to exit",
                         savers[selected_index].name);
                wrefresh(list_win);
                napms(1500);
                break;
            case 27:  // ESC key
            case 'q':
            case 'Q':
                goto cleanup;
            case 'c':
            case 'C':
                configure_screensaver(selected_index);
                break;
            case 'p':
            case 'P':
                preview_screensaver(selected_index);
                break;
            case 'r':
            case 'R':
                restore_default();
                mvwprintw(list_win, LINES-2, 2, "Restored: Default screensaver");
                wrefresh(list_win);
                napms(1500);
                break;
            default:
                break;  // Remove debug output to reduce clutter
        }
    }

cleanup:
    // Clean up preview process
    if (preview_pid > 0) {
        kill(preview_pid, SIGTERM);
        waitpid(preview_pid, NULL, 0);
    }

    endwin();
    return 0;
}
