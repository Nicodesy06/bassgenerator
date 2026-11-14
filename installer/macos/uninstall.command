#!/bin/bash
# Removes GrooveForge Bass (VST3, AU, app) installed by the package. Favorites are kept.
echo "Rimozione di GrooveForge Bass (serve la password di amministratore)..."
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/GrooveForge Bass.vst3" \
            "/Library/Audio/Plug-Ins/Components/GrooveForge Bass.component" \
            "/Applications/GrooveForge Bass.app"
for id in vst3 au app; do sudo pkgutil --forget "com.grooveforge.bass.$id" >/dev/null 2>&1; done
echo "Fatto. I preferiti in ~/Library/GrooveForgeBass non sono stati toccati."
read -n 1 -s -r -p "Premi un tasto per chiudere."
