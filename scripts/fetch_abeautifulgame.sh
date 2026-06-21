#!/bin/bash
# Fetch the glTF "A Beautiful Game" chess scene from the Khronos glTF-Sample-Assets
# repo into models/ABeautifulGame/ (ABeautifulGame.gltf + .bin + textures). The scene
# includes transparent (glass) materials, useful for testing alpha-blended GI. Uses a
# sparse, blobless clone so only that folder is downloaded, not the whole sample repo.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/models/ABeautifulGame"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ -f "$DEST/ABeautifulGame.gltf" ]; then
    echo "A Beautiful Game already present at $DEST/ABeautifulGame.gltf"
    exit 0
fi

echo "Sparse-cloning A Beautiful Game from Khronos glTF-Sample-Assets..."
git clone --depth 1 --filter=blob:none --sparse \
    https://github.com/KhronosGroup/glTF-Sample-Assets.git "$TMP/assets"
git -C "$TMP/assets" sparse-checkout set Models/ABeautifulGame/glTF

SRC="$TMP/assets/Models/ABeautifulGame/glTF"
if [ ! -f "$SRC/ABeautifulGame.gltf" ]; then
    echo "ERROR: ABeautifulGame.gltf not found in the cloned assets ($SRC)." >&2
    exit 1
fi

mkdir -p "$DEST"
cp -v "$SRC"/* "$DEST"/
echo "Done. A Beautiful Game is at $DEST/ABeautifulGame.gltf"
echo "Select 'A Beautiful Game (glTF)' in the scene dropdown to load it."
