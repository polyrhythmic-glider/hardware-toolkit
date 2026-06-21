# testLCD Menu Tree

## Structure Rules

- `Wave Engine` owns the four synth voices, global pitch, filter access, and audio test.
- Each voice opens a submenu where the base wave can be selected and then shaped with tune, fold, phase, and scope.
- `Mixer` owns master level, per-voice volume, kick volume, and the kick designer submenu.
- `Sequencer` owns transport and pattern utilities.
- `Envelope`, `Spectrum Lab`, `Bluetooth RX`, `Utility`, and `Hardware Test` stay separated by task.

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
    - Voice Phase
    - Voice Scope
  - Voice 2
    - Wave Type
    - Voice Tune
    - Voice Fold
    - Voice Phase
    - Voice Scope
  - Voice 3
    - Wave Type
    - Voice Tune
    - Voice Fold
    - Voice Phase
    - Voice Scope
  - Voice 4
    - Wave Type
    - Voice Tune
    - Voice Fold
    - Voice Phase
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
  - Voice 3 Volume
  - Voice 4 Volume
  - Kick Volume
  - Kick Designer
    - Kick Pitch
    - Kick Decay
    - Kick Sweep
    - Kick Click
    - Kick Drive

- Envelope
  - ADSR View
  - Attack
  - Decay
  - Sustain
  - Release

- Sequencer
  - Start
  - Stop
  - Seq BPM
  - Seq Step
  - Seq Note
  - Seq Length
  - Seq Direction
  - Seq Gate
  - Seq Transpose
  - Step On
  - Randomize
  - Clear Seq
  - Rotate Seq

- Spectrum Lab
  - Analyzer
  - BT FFT
  - FFT Range
  - Window
  - Smoothing
  - Peak Hold
  - FFT Bins

- Bluetooth RX
  - Bluetooth
  - Receiver
  - Pair Name
  - PCM5102 Bus
  - Line Output

- Utility
  - Inputs
  - Display
  - Stage

- Hardware Test
  - Display
  - Inputs
  - Test Audio
  - Wave Scope
  - Analyzer
  - Stage
