/*
 * REFERENCE ONLY: this translation unit is intentionally not part of any
 * Makefile or firmware target in hardware-toolkit.
 *
 * Design reference derived from recursinging/kxmx_bluemchen (MIT), inspected
 * at commit a18b647f82fbdd548093de74d6c2c9e0d0f83878. This is a reduced,
 * newly written example of its board-facade/application split; it is not a
 * copy of the upstream hardware mapping.
 */

#include "daisy_seed.h"

namespace kxmx_design_reference {

using daisy::AudioHandle;
using daisy::DaisySeed;
using daisy::System;

class MinimalBoard {
 public:
  void Init() {
    seed_.Configure();
    seed_.Init();
    ConfigureAudio();
  }

  float AudioSampleRate() {
    return seed_.AudioSampleRate();
  }

  void StartAudio(AudioHandle::AudioCallback callback) {
    seed_.StartAudio(callback);
  }

  void StopAudio() {
    seed_.StopAudio();
  }

  void DelayMs(uint32_t milliseconds) {
    System::Delay(milliseconds);
  }

 private:
  static constexpr size_t kAudioBlockSize = 48;

  void ConfigureAudio() {
    seed_.SetAudioBlockSize(kAudioBlockSize);
  }

  DaisySeed seed_;
};

MinimalBoard board;

// The callback owns only block-level realtime work. Peripheral polling,
// display rendering, MIDI parsing and storage belong in the foreground loop.
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t frame_count) {
  for (size_t frame = 0; frame < frame_count; ++frame) {
    out[0][frame] = in[0][frame];
    out[1][frame] = in[1][frame];
  }
}

}  // namespace kxmx_design_reference

int main(void) {
  using namespace kxmx_design_reference;

  board.Init();

  // A real application would initialize its DSP here with
  // board.AudioSampleRate(), before audio interrupts are enabled.
  (void)board.AudioSampleRate();
  board.StartAudio(AudioCallback);

  while (true) {
    // Slow/non-realtime work would run here.
    board.DelayMs(1);
  }
}
