# TheSynthProject Menu Tree

## Structure Rules

- `Wave Engine` owns the two synth voices, global pitch, and filter access.
- Each voice opens a submenu where the base wave can be selected and then shaped with tune, fold, detune, and scope.
- `Mixer` owns per-voice volume, delay, and reverb.
- `Kick Voice` owns kick level, kick tone design, and the dedicated kick pattern lane.
- `Envelope` contiene due ADSR indipendenti; ogni sorgente può pilotare più destinazioni.
- Il routing usa una tabella sorgente/destinazione condivisa con il DSP, predisposta per nuove sorgenti.
- `Sequencer` apre direttamente la Synth View e raggruppa transport, pattern e impostazioni.
- `Utility` includes global output volume, diagnostics, and hardware tests,
  including the only `Test Audio` entry.

## Wave Types

- SINE
- SQUARE
- TRI
- SAW
- RAMP
- PULSE
- SUB
- ORGAN
- EVEN
- ODD
- SUPER
- PWM
- STAIR
- CLIP
- NOISE
- FORMNT

## Tree

- Wave Engine
  - Voice 1
    - Wave Type
    - Voice Tune
    - Voice Fold
    - Voice Detune
    - Voice Scope
  - Voice 2
    - Wave Type
    - Voice Tune
    - Voice Fold
    - Voice Detune
    - Voice Scope
  - Pitch
  - Filter
    - Filter Power
    - Filter Type
    - Filter Cutoff
- Mixer
  - Voice 1 Volume
  - Voice 2 Volume
  - Delay Mix
  - Delay Time
  - Delay Feedback
  - Reverb Mix
  - Reverb Room

- Kick Voice
  - Kick Level
  - 909 Tune
  - 909 Attack
  - 909 Decay
  - 909 Sweep
  - 909 Drive

- Envelope
  - ENV1
    - Attack
    - Decay
    - Sustain
    - Release
    - Destinations
  - ENV2
    - Attack
    - Decay
    - Sustain
    - Release
    - Destinations

Ogni menu `Destinations` contiene le stesse route:

- Amp Level
- Filter Cutoff
- Filter Resonance
- Oscillator 1 Pitch
- Oscillator 2 Pitch
- Oscillator Mix
- PWM
- FM Amount
- Wavefold Amount
- Drive
- Pan
- LFO1 Rate
- LFO1 Depth
- LFO2 Rate
- LFO2 Depth
- FX Mix
- Delay Feedback
- Reverb Mix

Aprendo una destinazione sono disponibili `Enabled` (`On/Off`) e `Amount`
(`-100%..+100%`). ENV1 parte assegnato ad `Amp Level` al `+100%`, per
conservare il comportamento del synth precedente; ENV2 parte senza route.

- Sequencer
  - Synth View (visuale principale)
  - Transport
    - Start
    - Stop
    - Seq BPM
  - Pattern
    - Seq Track / V1, V2, Kick
    - Seq Step
    - Seq Note / Kick Hit
    - Step On
  - Settings
    - Seq Length / 16, 32, 48, 64
    - Seq Page
    - Page Auto Scroll

- Utility
  - Inputs
  - Display
  - Control Surface
  - OLED Debug
  - MCP Debug
  - NeoKey Debug
  - Serial Diagnostics
  - Connection Status
  - Output Volume
  - Test Audio
  - About
  - Preset Slot
  - Load Preset
  - Save Preset
