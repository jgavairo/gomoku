#!/bin/bash

# Script d'installation de l'icône et du lanceur Gomoku
# Ce script configure Gomoku pour être accessible depuis le bureau et le menu applications

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
MSG_INSTALL="${GREEN}[INSTALL]${RESET}"

# ============================= CONFIGURATION ================================ #
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DESKTOP_DIR="$SCRIPT_DIR"
BIN_PATH="$PROJECT_DIR/bin/Gomoku"
ICON_DEST="$DESKTOP_DIR/gomicon.png"
DESKTOP_TEMPLATE="$DESKTOP_DIR/gomoku.desktop.template"
DESKTOP_FILE="$DESKTOP_DIR/gomoku.desktop"

printf "${MSG_INSTALL} Setting up desktop integration\n"

# Vérifier que le template existe
if [ ! -f "$DESKTOP_TEMPLATE" ]; then
    printf "${MSG_ERROR} Template not found at ${BOLD}$DESKTOP_TEMPLATE${RESET}\n"
    exit 1
fi

# Vérifier que l'exécutable existe
if [ ! -f "$BIN_PATH" ]; then
    printf "${MSG_ERROR} Executable not found at ${BOLD}$BIN_PATH${RESET}\n"
    printf "${MSG_INFO} Please compile first with ${BOLD}'make'${RESET}\n"
    exit 1
fi

# Vérifier que l'icône existe
if [ ! -f "$ICON_DEST" ]; then
    printf "${MSG_ERROR} Icon file not found at ${BOLD}$ICON_DEST${RESET}\n"
    printf "${MSG_INFO} Make sure gomicon.png is in desktop/ directory\n"
    exit 1
fi

printf "${MSG_SUCCESS} Icon found: ${BOLD}$ICON_DEST${RESET}\n"

# Générer le fichier .desktop depuis le template
printf "${MSG_INFO} Generating launcher from template...\n"
sed "s|{{BIN_PATH}}|$BIN_PATH|g; s|{{ICON_PATH}}|$ICON_DEST|g; s|{{PROJECT_DIR}}|$PROJECT_DIR|g" \
    "$DESKTOP_TEMPLATE" > "$DESKTOP_FILE"

# Rendre le fichier .desktop exécutable
chmod +x "$DESKTOP_FILE"

printf "${MSG_SUCCESS} Desktop file generated: ${BOLD}$DESKTOP_FILE${RESET}\n"

# Copier le fichier .desktop vers les emplacements appropriés
USER_DESKTOP_DIR="$HOME/Desktop"
USER_APPLICATIONS_DIR="$HOME/.local/share/applications"

# Créer le répertoire applications s'il n'existe pas
mkdir -p "$USER_APPLICATIONS_DIR"

# Copier vers le répertoire des applications
cp "$DESKTOP_FILE" "$USER_APPLICATIONS_DIR/" && installed_to_applications=1 || installed_to_applications=0
printf "${MSG_INSTALL} Launcher installed to applications menu\n"

# Copier vers le bureau si le répertoire existe
if [ -d "$USER_DESKTOP_DIR" ]; then
    cp "$DESKTOP_FILE" "$USER_DESKTOP_DIR/" && installed_to_desktop=1 || installed_to_desktop=0
    chmod +x "$USER_DESKTOP_DIR/gomoku.desktop"
    printf "${MSG_INSTALL} Launcher added to desktop\n"
fi

# Mettre à jour la base de données des applications
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$USER_APPLICATIONS_DIR"
    printf "${MSG_INFO} Application database updated\n"
fi

# Final summary: report where the launcher was installed
installed_to_applications=${installed_to_applications:-0}
installed_to_desktop=${installed_to_desktop:-0}
if [ $installed_to_applications -eq 1 ] || [ $installed_to_desktop -eq 1 ]; then
    printf "${MSG_SUCCESS} Installation completed successfully\n"
else
    printf "${MSG_INFO} Installation completed but nothing was installed to user locations.\n"
fi
