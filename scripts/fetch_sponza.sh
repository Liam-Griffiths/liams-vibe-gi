#!/bin/bash
# Fetch the glTF Sponza test scene from the Khronos glTF-Sample-Assets repo into
# models/Sponza/ (Sponza.gltf + Sponza.bin + textures). Uses a sparse, blobless clone
# so only the Sponza folder is downloaded, not the whole (large) sample-assets repo.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/models/Sponza"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ -f "$DEST/Sponza.gltf" ]; then
    echo "glTF Sponza already present at $DEST/Sponza.gltf"
    exit 0
fi

echo "Sparse-cloning glTF Sponza from Khronos glTF-Sample-Assets..."
git clone --depth 1 --filter=blob:none --sparse \
    https://github.com/KhronosGroup/glTF-Sample-Assets.git "$TMP/assets"
git -C "$TMP/assets" sparse-checkout set Models/Sponza/glTF

SRC="$TMP/assets/Models/Sponza/glTF"
if [ ! -f "$SRC/Sponza.gltf" ]; then
    echo "ERROR: Sponza.gltf not found in the cloned assets ($SRC)." >&2
    exit 1
fi

mkdir -p "$DEST"
cp -v "$SRC"/* "$DEST"/
echo "Done. glTF Sponza is at $DEST/Sponza.gltf"
echo "Select 'Sponza (glTF PBR)' in the scene dropdown to load it."
