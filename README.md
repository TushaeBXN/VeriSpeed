# VeriSpeed v1.1

**Tape Varispeed Simulation VST3 Plugin**
by Brian Tushae Thomas

Simulates analog tape deck varispeed behaviour on any stereo signal.
Speed ratio follows the tape formula:

```
speed_ratio = 2^(semitones / 12)
semitones   = 12 * log2(speed_ratio)
```

---

## Building

### Prerequisites

| Tool | Minimum version |
|------|----------------|
| CMake | 3.22 |
| JUCE | 7.0.9 |
| Xcode (macOS) | 14 |
| Visual Studio (Windows) | 2022 |

### Step 1 – Clone JUCE

```bash
cd VeriSpeed
git clone https://github.com/juce-framework/JUCE.git --branch 7.0.12 --depth 1
```

Or point to an existing JUCE install:

```bash
cmake -B build -DJUCE_DIR=/path/to/JUCE
```

### Step 2 – Configure

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### Step 3 – Build

```bash
cmake --build build --config Release --parallel
```

### Step 4 – Output location

| Platform | Path |
|----------|------|
| macOS | `build/VeriSpeed_artefacts/Release/VST3/VeriSpeed.vst3` |
| Windows | `build\VeriSpeed_artefacts\Release\VST3\VeriSpeed.vst3` |

The CMake script copies the `.vst3` to the system plugin folder automatically
(`~/Library/Audio/Plug-Ins/VST3` on macOS, `C:\Program Files\Common Files\VST3` on Windows).

---

## Using in FL Studio

1. Place `VeriSpeed.vst3` in your VST3 folder and rescan.
2. Insert on the **Master** mixer channel.
3. Drag the left knob down or the right knob up to shift pitch.
4. Enable **Tempo On** to see the BPM-scaled tempo displayed in the UI.
5. The **exclude** field accepts a symbol (default `@`) used as a
   shorthand identifier in any preset naming convention you choose.

---

## Parameters

| Parameter | Range | Default |
|-----------|-------|---------|
| Semitone Shift | −24 … +24 st | 0 |
| Tempo On | on / off | off |
| Exclude Symbol | text | `@` |

---

## File Structure

```
VeriSpeed/
├── Source/
│   ├── PluginProcessor.h / .cpp   – DSP + APVTS
│   ├── PluginEditor.h / .cpp      – Main UI
│   ├── VUMeter.h / .cpp           – Analog needle VU meter
│   └── PillButton.h / .cpp        – Gold & steel pill buttons
├── JUCE/                          – JUCE 7 submodule (git clone here)
├── CMakeLists.txt
├── VeriSpeed.jucer                – Projucer backup
└── README.md
```

---

## License

Copyright © 2024 Brian Tushae Thomas. All rights reserved.
