# VeriSpeed v1.1 — User Guide

**by Brian Tushae Thomas**

---

## What It Does

VeriSpeed simulates analog tape deck varispeed — the effect you get when a tape machine runs faster or slower than normal. Pitch goes up when you speed up, pitch goes down when you slow down. Use it on the master channel to detune your entire mix like a tape machine.

---

## Installation

### macOS
Copy `VeriSpeed.vst3` to:
```
~/Library/Audio/Plug-Ins/VST3/
```

### Windows
Copy `VeriSpeed.vst3` to:
```
C:\Program Files\Common Files\VST3\
```

Then rescan plugins in FL Studio: **Options → Manage plugins → Find more plugins**

---

## Loading in FL Studio

1. Open the **Mixer** (F9)
2. Click the **Master** channel
3. Click an empty insert slot
4. Find **VeriSpeed** in the plugin list and double-click it

---

## Controls

### Tempo_on (Gold button, top-left)
- **Off** — only pitch is affected
- **On** — displays the BPM scaled to match the speed ratio in the UI

### Left Knob — Pitch Down
Drag down to lower the pitch. Each full rotation = semitone decrease.

### Right Knob — Pitch Up
Drag up to raise the pitch. Each full rotation = semitone increase.

> Both knobs control the same parameter. Use whichever feels natural.

### VU Meter (centre)
Shows the current semitone shift as an analog needle.
- **Centre (0)** = no pitch shift
- **Left** = pitch down
- **Right** = pitch up
- **Red zone** = beyond ±4 semitones (extreme shift)

### Semitone Readout (large number below meter)
Displays the exact shift value. Example: `+2.50 st` or `-1.00 st`.

### RESET (steel button)
Returns the pitch to **0** (no shift) instantly.

### Exclude field
Type any symbol here (default `@`). This is used as a shorthand tag for preset naming — label your presets with `@` to mark them as VeriSpeed presets.

---

## Typical Use Cases

| Scenario | Setting |
|---|---|
| Vintage tape warmth | −0.10 to −0.30 st |
| Lo-fi detuned feel | −1.00 to −2.00 st |
| Pitched-up energetic mix | +0.50 to +1.00 st |
| Full semitone transpose | Any whole number (±1, ±2, etc.) |
| Extreme tape slow-down effect | −6.00 to −12.00 st |

---

## Tips

- **Automate the semitone knob** in FL Studio for dynamic pitch drops mid-track
- **Small values (±0.1 to ±0.3)** sound most like real tape flutter
- **RESET** is mapped to the parameter — you can automate it or assign a keyboard shortcut in FL Studio
- The plugin has **zero latency** — no delay compensation needed

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Plugin not showing in FL Studio | Rescan: Options → Manage plugins → Find more plugins |
| macOS won't load it | Run: `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/VeriSpeed.vst3` |
| No sound change | Make sure it's on the Master channel, not an empty channel |
| Crackling at extreme settings | Reduce shift to within ±12 semitones |
