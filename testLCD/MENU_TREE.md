# testLCD Menu Tree

## Structure Rules

- `Wave Engine` owns the two synth voices, global pitch, filter access, and audio test.
- Each voice opens a submenu where the base wave can be selected and then shaped with tune, fold, phase, and scope.
- `Mixer` owns master level, per-voice volume, kick volume, and the kick designer submenu.
- `Sequencer` owns one active voice sequencer at a time, transport, and per-track pattern utilities.
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
  - Kick Volume
  - Kick Designer
    - Kick Pitch
    - Kick Decay
    - Kick Sweep
    - Kick Click
    - Kick Drive
  - Delay Mix
  - Delay Time
  - Delay Feedback
  - Reverb Mix
  - Reverb Room

- Envelope
  - ADSR View
  - Attack
  - Decay
  - Sustain
  - Release

- Sequencer
  - Synth View
  - Transport
    - Start
    - Stop
  - Sequencer Power
  - Seq BPM
  - Seq Voice / Active Sequencer
  - Seq Step
  - Seq Note
  - Step Shape
  - Seq Length
  - Seq Direction
  - Seq Gate
  - Seq Transpose
  - Step On
  - Random Track
  - Clear Track
  - Rotate Track

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
  - Connection
  - Device Name
  - Audio Output

- Utility
  - Inputs
  - Display
  - About
  - Preset Slot
  - Load Preset
  - Save Preset

- Hardware Test
  - Display
  - Inputs
  - Test Audio
  - Wave Scope
  - Analyzer
  - About
