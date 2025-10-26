#ifndef OFFICIAL_SCRIPT_H
#define OFFICIAL_SCRIPT_H

// Official omarchy-cmd-screensaver content for restore default - stored in separate header to avoid complex escape sequences in main C file
const char *ORIGINAL_OFFICIAL_SCRIPT_CONTENT =
    "#!/bin/bash\n"
    "\n"
    "screensaver_in_focus() {\n"
    "  hyprctl activewindow -j | jq -e '.class == \"Screensaver\"' >/dev/null 2>&1\n"
    "}\n"
    "\n"
    "exit_screensaver() {\n"
    "  hyprctl keyword cursor:invisible false\n"
    "  pkill -x tte 2>/dev/null\n"
    "  pkill -f \"alacritty --class Screensaver\" 2>/dev/null\n"
    "  exit 0\n"
    "}\n"
    "\n"
    "trap exit_screensaver SIGINT SIGTERM SIGHUP SIGQUIT\n"
    "\n"
    "hyprctl keyword cursor:invisible true &>/dev/null\n"
    "\n"
    "while true; do\n"
    "  effect=$(tte 2>&1 | grep -oP '{\\K[^}]+' | tr ',' ' ' | tr ' ' '\\n' | sed -n '/^beams$/,$p' | sort -u | shuf -n1)\n"
    "  tte -i ~/.config/omarchy/branding/screensaver.txt \\\n"
    "    --frame-rate 240 --canvas-width 0 --canvas-height $(($(tput lines) - 2)) --anchor-canvas c --anchor-text c \\\n"
    "    \"$effect\" &\n"
    "\n"
    "  while pgrep -x tte >/dev/null; do\n"
    "    if read -n 1 -t 3 || ! screensaver_in_focus; then\n"
    "      exit_screensaver\n"
    "    fi\n"
    "  done\n"
    "done\n";

#endif // OFFICIAL_SCRIPT_H
