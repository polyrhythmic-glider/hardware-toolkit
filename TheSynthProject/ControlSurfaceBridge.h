#pragma once

#include <Arduino.h>

namespace ControlSurfaceBridge {

enum Page : uint8_t {
  PAGE_VOICE_1 = 0,
  PAGE_VOICE_2,
  PAGE_TUNING,
  PAGE_ENVELOPE,
  PAGE_ENVELOPE_CLOCK,
  PAGE_MIXER,
  PAGE_FILTER_FX,
  PAGE_DELAY_REVERB,
  PAGE_KICK,
  PAGE_KICK_SHAPE,
  PAGE_SEQUENCER,
  PAGE_TRANSPORT,
  PAGE_COUNT
};

const char* pageName(uint8_t page);
const char* parameterLabel(uint8_t page, uint8_t encoderIndex);
void formatParameterValue(uint8_t page, uint8_t encoderIndex, char* output, size_t outputSize);
void adjustParameter(uint8_t page, uint8_t encoderIndex, int delta);
void activatePageAction(uint8_t page);
uint32_t stateHash();
bool isTransportRunning();

}  // namespace ControlSurfaceBridge
