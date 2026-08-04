# TheSynthProject Menu Tree

## Structure Rules

- `Wave Engine` owns the two synth voices, global pitch, filter access, and audio test.
- Each voice opens a submenu where the base wave can be selected and then shaped with tune, fold, detune, and scope.
- `Mixer` owns master level, per-voice volume, delay, and reverb.
- `Kick Voice` owns kick level, kick tone design, and the dedicated kick pattern lane.
- `Envelope` contiene due ADSR indipendenti; ogni sorgente può pilotare più destinazioni.
- Il routing usa una tabella sorgente/destinazione condivisa con il DSP, predisposta per nuove sorgenti.
- `Sequencer` owns one active voice sequencer at a time and its basic transport controls.
- `Utility` includes diagnostics and hardware tests without duplicating entries from the other areas.

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
  - Test Audio

- Mixer
  - Master Level
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
  - Synth View
  - Transport
    - Start
    - Stop
  - Sequencer Power
  - Seq BPM
  - Seq Track / V1, V2, Kick
  - Seq Step
  - Seq Note / Kick Hit
  - Seq Length
  - Step On

- Utility
  - Inputs
  - Display
  - Control Surface
  - OLED Debug
  - MCP Debug
  - NeoKey Debug
  - Serial Diagnostics
  - Connection Status
  - About
  - Preset Slot
  - Load Preset
  - Save Preset
