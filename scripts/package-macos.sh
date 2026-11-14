#!/bin/bash
# Build dist/GrooveForgeBass-<version>-macos-universal.dmg containing an
# installer package (VST3, AU, Standalone app; each optional), a short guide
# and an uninstall script.
#   usage: scripts/package-macos.sh [artefacts-dir]
# The binaries are ad-hoc signed only (no Apple Developer ID / notarisation).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ART="${1:-$ROOT/build/macos/GrooveForgeBass_artefacts/Release}"
VERSION="$(sed -n 's/^project(GrooveForgeBass VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
NAME="GrooveForge Bass"
WORK="$ROOT/build/macos-pkg"
DIST="$ROOT/dist"
rm -rf "$WORK"; mkdir -p "$WORK" "$DIST"

for b in "$ART/VST3/$NAME.vst3" "$ART/AU/$NAME.component" "$ART/Standalone/$NAME.app"; do
  [ -d "$b" ] || { echo "missing $b (build first)"; exit 1; }
  codesign --force --deep --sign - "$b"
  codesign --verify --deep --strict "$b"
  lipo -info "$(find "$b/Contents/MacOS" -type f | head -1)"
done

stage() { # id src dest-dir
  local id="$1" src="$2" dest="$3"
  mkdir -p "$WORK/root-$id$dest"
  ditto "$src" "$WORK/root-$id$dest/$(basename "$src")"
  pkgbuild --analyze --root "$WORK/root-$id" "$WORK/$id.plist" >/dev/null
  # never let Installer relocate the bundle to another copy with the same id
  /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$WORK/$id.plist"
  pkgbuild --root "$WORK/root-$id" --component-plist "$WORK/$id.plist" \
           --identifier "com.grooveforge.bass.$id" --version "$VERSION" \
           --install-location / "$WORK/$id.pkg"
}
stage vst3 "$ART/VST3/$NAME.vst3" "/Library/Audio/Plug-Ins/VST3"
stage au   "$ART/AU/$NAME.component" "/Library/Audio/Plug-Ins/Components"
stage app  "$ART/Standalone/$NAME.app" "/Applications"

mkdir -p "$WORK/resources" "$WORK/dmg"
cp "$ROOT/installer/macos/welcome.html" "$WORK/resources/"
cp "$ROOT/LICENSE-THIRD-PARTY.md" "$WORK/resources/license.txt"
sed "s/@VERSION@/$VERSION/g" "$ROOT/installer/macos/distribution.xml" > "$WORK/distribution.xml"
productbuild --distribution "$WORK/distribution.xml" --resources "$WORK/resources" \
             --package-path "$WORK" "$WORK/dmg/Installa GrooveForge Bass.pkg"

cp "$ROOT/installer/macos/LEGGIMI.txt" "$WORK/dmg/LEGGIMI.txt"
cp "$ROOT/installer/macos/uninstall.command" "$WORK/dmg/Disinstalla GrooveForge Bass.command"
chmod +x "$WORK/dmg/Disinstalla GrooveForge Bass.command"

DMG="$DIST/GrooveForgeBass-$VERSION-macos-universal.dmg"
rm -f "$DMG"
hdiutil create -volname "GrooveForge Bass $VERSION" -srcfolder "$WORK/dmg" -fs HFS+ -format UDZO -ov "$DMG"
hdiutil verify "$DMG"
shasum -a 256 "$DMG" | sed "s#  .*/#  #" > "$DMG.sha256"
echo "DMG: $DMG"; cat "$DMG.sha256"
