# Good Aura Voice MIDI

A JUCE VST3/Standalone prototype that converts a monophonic sung or hummed melody into MIDI.

## Current features

- Live monophonic pitch detection from audio input
- Audio -> MIDI note output
- Velocity derived from vocal level
- Stability filtering to reduce ghost notes
- Silence detection and automatic note-off
- Key + scale locking
- Transpose
- MIDI capture
- Export captured melody to `.mid`
- Optional audio monitoring
- VST3 + Standalone targets

## Important limitations in v0.1

This is a first working architecture, not yet a Dubler-level transcription engine.
It is designed for one note at a time. It does not detect chords/polyphonic singing.
Pitch bends, vibrato interpretation, drag-and-drop MIDI, tempo-following quantization,
offline audio-file transcription, AI melody enhancement, and harmony generation are
planned for later versions.

## Build on macOS

Requirements:
- Xcode command line tools / Xcode
- CMake 3.22+
- Internet access on first configure (CMake fetches JUCE 9.0.1)

Run:

```bash
chmod +x build-mac.sh
./build-mac.sh
```

The VST3 should be created under the build products directory and, because
`COPY_PLUGIN_AFTER_BUILD` is enabled, JUCE will attempt to copy it to your normal
user plug-in location.

Typical VST3 location:

`~/Library/Audio/Plug-Ins/VST3/Good Aura Voice MIDI.vst3`

## FL Studio routing

Voice-to-MIDI plug-ins require both an audio input and a MIDI destination.
Host routing differs between FL Studio versions, so confirm that the plugin's
generated MIDI is routed to the instrument you want to play.

For initial testing, the Standalone target is useful for confirming that the microphone
and pitch detection are working before troubleshooting DAW routing.

## macOS Gatekeeper

A locally-built unsigned plugin may require ad-hoc signing during development.
For distribution to other Macs, use a Developer ID certificate and notarization.
