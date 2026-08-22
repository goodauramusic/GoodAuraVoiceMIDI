#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")"

cmake -S . -B build -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release --target GoodAuraVoiceMIDI_VST3 -j 4
cmake --build build --config Release --target GoodAuraVoiceMIDI_Standalone -j 4

echo
echo "Build finished."
echo "Searching for VST3:"
find build -name "Good Aura Voice MIDI.vst3" -print || true
