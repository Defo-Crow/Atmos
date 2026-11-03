# Atmos

VST3 synthesizer for ambient, drone and industrial sounds.

- 2 oscillators (sine / triangle / saw / square morph), up to 7 unison voices each, sub oscillator, noise
- ring modulation, FM, analog-style drift, glide, stereo spread
- multimode filter (LP12, LP24, HP, BP, notch), filter and amp ADSR up to 20-30 s, key tracking
- 2 LFOs (sine, triangle, saw, square, random) with a target each
- grit: drive, wavefold, bit crush, sample-rate reduction
- chorus, stereo delay, long feedback-delay-network reverb with octave-up shimmer
- 8 voices, 7 factory presets, mod wheel opens the filter, pitch bend +/-2 semitones, sustain pedal

## Build

Requires CMake 3.22+ and a C++17 compiler. JUCE 8.0.8 is downloaded automatically.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Result: build/Atmos_artefacts/Release/VST3/Atmos.vst3

JUCE is licensed under AGPLv3 or a commercial license.
