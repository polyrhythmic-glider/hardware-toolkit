#pragma once

#include <Arduino.h>

namespace ControlSurfaceBridge {

enum Page : uint8_t {
  PAGE_VOICE_1 = 0,
  PAGE_VOICE_1_PITCH,
  PAGE_VOICE_1_SHAPE,
  PAGE_VOICE_2,
  PAGE_VOICE_2_PITCH,
  PAGE_VOICE_2_SHAPE,
  PAGE_VOICE_3,
  PAGE_VOICE_3_PITCH,
  PAGE_VOICE_3_SHAPE,
  PAGE_V2_OSC_1,
  PAGE_V2_OSC_1_PITCH,
  PAGE_V2_OSC_1_SHAPE,
  PAGE_V2_OSC_2,
  PAGE_V2_OSC_2_PITCH,
  PAGE_V2_OSC_2_SHAPE,
  PAGE_V2_OSC_3,
  PAGE_V2_OSC_3_PITCH,
  PAGE_V2_OSC_3_SHAPE,
  PAGE_ENV_1,
  PAGE_ENV_1_DYNAMICS,
  PAGE_ENV_1_MODULATION,
  PAGE_ENV_2,
  PAGE_ENV_2_DYNAMICS,
  PAGE_ENV_2_MODULATION,
  PAGE_ENV_3,
  PAGE_ENV_3_DYNAMICS,
  PAGE_ENV_3_MODULATION,
  PAGE_LFO_1,
  PAGE_LFO_1_SHAPE,
  PAGE_LFO_1_BEHAVIOUR,
  PAGE_LFO_2,
  PAGE_LFO_2_SHAPE,
  PAGE_LFO_2_BEHAVIOUR,
  PAGE_MIXER,
  PAGE_MIXER_LEVELS,
  PAGE_MIXER_STEREO,
  PAGE_FILTER_FX,
  PAGE_FILTER_FX_NOISE_PAN,
  PAGE_FILTER_FX_MODULATION,
  PAGE_DELAY_REVERB,
  PAGE_DELAY_REVERB_REVERB,
  PAGE_DELAY_REVERB_ROUTING,
  PAGE_KICK,
  PAGE_KICK_SHAPE,
  PAGE_KICK_DRIVE,
  PAGE_COUNT
};

uint8_t pageBase(uint8_t page);
uint8_t pageSubpage(uint8_t page);
const char* subpageName(uint8_t page);
void activateParameterAction(uint8_t page, uint8_t encoderIndex);
const char* pageName(uint8_t page);
const char* pageMenuName(uint8_t page);
bool isOscillatorPage(uint8_t page);
int getOscillatorForPage(uint8_t page);
int getVoiceForPage(uint8_t page);
uint8_t getOscillatorSubpage(uint8_t page);
const char* parameterLabel(uint8_t page, uint8_t encoderIndex);
void formatParameterValue(uint8_t page, uint8_t encoderIndex, char* output, size_t outputSize);
bool isOscillatorSummaryPage(uint8_t page);
void formatOscillatorSummaryTitle(char* output, size_t outputSize);
void formatOscillatorSummaryLine(uint8_t line, char* output, size_t outputSize);
void adjustParameter(uint8_t page, uint8_t encoderIndex, int delta);
void activatePageAction(uint8_t page);
uint32_t stateHash();
bool isTransportRunning();

}  // namespace ControlSurfaceBridge
