#!/bin/bash

# Script de désinstallation du lanceur Gomoku

set -e

# ================================= COLORS =================================== #
RESET='\033[0m'
BOLD='\033[1m'
RED='\033[31m'
GREEN='\033[32m'
YELLOW='\033[33m'
BLUE='\033[34m'
MAGENTA='\033[35m'
CYAN='\033[36m'

# =============================== MESSAGES ================================== #
MSG_INFO="${BLUE}[INFO]${RESET}"
MSG_SUCCESS="${GREEN}[SUCCESS]${RESET}"
MSG_ERROR="${RED}[ERROR]${RESET}"
MSG_UNINSTALL="${YELLOW}[UNINSTALL]${RESET}"

# ============================= CONFIGURATION ================================ #
USER_DESKTOP_DIR="$HOME/Desktop"
USER_APPLICATIONS_DIR="$HOME/.local/share/applications"
DESKTOP_FILE="gomoku.desktop"

# Flags to track what we removed
removed_from_applications=0
removed_from_desktop=0

# Supprimer du menu des applications
if [ -f "$USER_APPLICATIONS_DIR/$DESKTOP_FILE" ]; then
    rm "$USER_APPLICATIONS_DIR/$DESKTOP_FILE"
    removed_from_applications=1
    printf "${MSG_SUCCESS} Launcher removed from applications menu\n"
fi

# Supprimer du bureau
if [ -f "$USER_DESKTOP_DIR/$DESKTOP_FILE" ]; then
    rm "$USER_DESKTOP_DIR/$DESKTOP_FILE"
    removed_from_desktop=1
    printf "${MSG_SUCCESS} Launcher removed from desktop\n"
fi

# Mettre à jour la base de données des applications
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$USER_APPLICATIONS_DIR"
fi

# Final summary
if [ $removed_from_applications -eq 1 ] || [ $removed_from_desktop -eq 1 ]; then
    printf "${MSG_SUCCESS} Uninstallation completed\n"
fi
