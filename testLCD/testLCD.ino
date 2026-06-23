#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <string.h>
#include "AudioConfig.h"
#include "HardwarePins.h"
#include "MenuModel.h"
#include "MixerModel.h"
#include "SequencerModel.h"
#include "SynthMath.h"

// =======================
// DISPLAY
// =======================

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
I2SStream btI2s;
BluetoothA2DPSink a2dpSink(btI2s);

const int SCREEN_W = 160;
const int SCREEN_H = 128;

constexpr uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

const uint16_t COL_BG = rgb565(5, 7, 14);
const uint16_t COL_PANEL = rgb565(15, 19, 31);
const uint16_t COL_HEADER = rgb565(21, 25, 40);
const uint16_t COL_LINE = rgb565(42, 47, 62);
const uint16_t COL_MUTED = rgb565(118, 126, 142);
const uint16_t COL_INK = rgb565(238, 235, 220);
const uint16_t COL_NOTE = rgb565(59, 220, 181);
const uint16_t COL_GOLD = rgb565(246, 191, 65);
const uint16_t COL_CORAL = rgb565(255, 91, 76);
const uint16_t COL_VIOLET = rgb565(137, 103, 255);
const uint16_t COL_SKY = rgb565(98, 178, 255);

const uint16_t ITEM_COLORS[] = {
  COL_NOTE,
  COL_GOLD,
  COL_CORAL,
  COL_VIOLET,
  COL_SKY,
  COL_INK
};

const int ITEM_COLOR_COUNT = sizeof(ITEM_COLORS) / sizeof(ITEM_COLORS[0]);

#include "InputController.h"

// =======================
// MENU DATA
// =======================

enum UiMode {
  MODE_BOOT,
  MODE_PREMENU,
  MODE_MENU,
  MODE_EDIT,
  MODE_ABOUT
};

void setMode(UiMode newMode);

UiMode uiMode = MODE_BOOT;
UiMode lastDrawnMode = MODE_ABOUT;

const char* waveformNames[] = {
  "SINE",
  "SQUARE",
  "TRI",
  "SAW",
  "RAMP",
  "PULSE",
  "SUB",
  "ORGAN",
  "EVEN",
  "ODD",
  "SUPER",
  "PWM",
  "STAIR",
  "CLIP",
  "NOISE",
  "FORMNT"
};

const int WAVE_SHAPE_COUNT = sizeof(waveformNames) / sizeof(waveformNames[0]);

int waveform = 0;
int frequency = 220;
int volume = 80;
int attack = 10;
int decay = 80;
int sustain = 68;
int releaseMs = 320;
int waveFold = 12;
int wavePhase = 0;
bool filterEnabled = false;
int filterType = 0;
int filterCutoff = 2400;
int delayMix = 0;
int delayTimeMs = 280;
int delayFeedback = 35;
int reverbMix = 0;
int reverbRoom = 65;
int presetSlot = 1;
bool presetOperationOk = true;
const char* presetOperationText = "SELECT";
int fftRangeIndex = 1;
int fftWindow = 0;
int fftSmoothing = 35;
int fftBinsIndex = 1;
bool fftPeakHold = true;
bool dacI2sStarted = false;
bool bluetoothEnabled = false;
bool btAudioStarted = false;
uint32_t btStartedAtMs = 0;
volatile uint32_t btLastAudioMs = 0;
volatile uint32_t btPacketCount = 0;
volatile int16_t btFftSamples[BT_FFT_SAMPLES];
volatile uint16_t btFftWriteIndex = 0;
volatile uint32_t btFftSampleCount = 0;
uint8_t btFftDecimator = 0;
TaskHandle_t synthAudioTaskHandle = nullptr;
int16_t synthAudioBuffer[SYNTH_AUDIO_BUFFER_FRAMES * 2];
volatile bool synthRetriggerRequested = true;
float synthPhase = 0.0f;
float synthVoicePhases[WAVE_VOICE_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f };
uint32_t synthEnvelopeSample = 0;
float synthEnvelopeSmoothed = 0.0f;
bool synthWasActive = false;
bool transportRunning = true;
bool sequencerEnabled = false;
int sequencerBpm = 120;
int sequencerLength = SEQUENCER_STEP_COUNT;
int sequencerDirection = 0;
int sequencerGate = 70;
int sequencerTranspose = 0;
int sequencerSelectedStep = 0;
volatile int sequencerCurrentStep = 0;
uint32_t sequencerSampleCounter = 0;
int sequencerStepNotes[SEQUENCER_STEP_COUNT] = { 0, 3, 5, 7, 10, 7, 5, 3 };
bool sequencerStepEnabled[SEQUENCER_STEP_COUNT] = { true, true, true, true, true, true, true, true };
bool sequencerPingPongForward = true;
int waveVoiceLevels[WAVE_VOICE_COUNT] = { 100, 0, 0, 0 };
int waveVoiceShape[WAVE_VOICE_COUNT] = { 0, 1, 2, 3 };
int waveVoiceTune[WAVE_VOICE_COUNT] = { 0, 0, 0, 0 };
int waveVoiceFold[WAVE_VOICE_COUNT] = { 12, 12, 12, 12 };
int waveVoicePhase[WAVE_VOICE_COUNT] = { 0, 0, 0, 0 };
int kickLevel = 0;
int kickPitch = 48;
int kickDecay = 420;
int kickSweep = 120;
int kickClick = 20;
int kickDrive = 20;
float kickPhase = 0.0f;
uint32_t kickSampleAge = 0;
bool kickVoiceActive = false;
float audioFrequency = 220.0f;
float audioVolume = 80.0f;
float audioWaveFold = 12.0f;
float audioWavePhase = 0.0f;
float audioWaveVoiceLevels[WAVE_VOICE_COUNT] = { 100.0f, 0.0f, 0.0f, 0.0f };
int audioWaveVoiceShape[WAVE_VOICE_COUNT] = { 0, 1, 2, 3 };
float audioWaveVoiceTune[WAVE_VOICE_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f };
float audioWaveVoiceFold[WAVE_VOICE_COUNT] = { 12.0f, 12.0f, 12.0f, 12.0f };
float audioWaveVoicePhase[WAVE_VOICE_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f };
float audioKickLevel = 0.0f;
float audioKickPitch = 48.0f;
float audioKickDecay = 420.0f;
float audioKickSweep = 120.0f;
float audioKickClick = 20.0f;
float audioKickDrive = 20.0f;
float audioFilterAmount = 0.0f;
float audioFilterCutoff = 2400.0f;
int audioFilterType = 0;
float filterLowState = 0.0f;
float filterBandState = 0.0f;
int16_t delayBuffer[DELAY_BUFFER_SAMPLES] = { 0 };
uint32_t delayWriteIndex = 0;
int16_t reverbBufferA[REVERB_BUFFER_A_SAMPLES] = { 0 };
int16_t reverbBufferB[REVERB_BUFFER_B_SAMPLES] = { 0 };
int16_t reverbBufferC[REVERB_BUFFER_C_SAMPLES] = { 0 };
uint16_t reverbIndexA = 0;
uint16_t reverbIndexB = 0;
uint16_t reverbIndexC = 0;

MenuAction getSelectedAction();
bool isSynthAction(MenuAction action);
void startDacI2s();
void startBluetoothAudio();
void stopBluetoothAudio();
void startSynthAudio();
bool isSynthAudioActive();
void resetSequencerClock();
void triggerKickVoice();
int getSequencerLength();
void clampSequencerIndexes();
void randomizeSequencerPattern();
void clearSequencerPattern();
void rotateSequencerPattern(int direction);
bool isMixerLayerEnabled();
bool isMixerLayerAudible();

int selectedType = 0;
int activeType = 0;
int lastSelectedType = -1;
int topType = 0;
int lastTopType = -1;

int selectedItem = 0;
int lastSelectedItem = -1;

int topItem = 0;
int lastTopItem = -1;
MenuPage currentMenuPage = MENU_PAGE_ROOT;
int activeWaveVoiceIndex = 0;

const int VISIBLE_ROWS = 5;
const int VISIBLE_TYPE_ROWS = 6;
const uint32_t menuStepCooldownMs = 80;
uint32_t lastMenuMoveMs = 0;

bool fullRedraw = true;
bool valueDirty = true;

int lastWaveform = -1;
int lastFrequency = -1;
int lastVolume = -1;
int lastAttack = -1;
int lastDecay = -1;
int lastSustain = -1;
int lastReleaseMs = -1;
int lastWaveFold = -1;
int lastWavePhase = -1;
int lastFilterEnabled = -1;
int lastFilterType = -1;
int lastFilterCutoff = -1;
int lastDelayMix = -1;
int lastDelayTimeMs = -1;
int lastDelayFeedback = -1;
int lastReverbMix = -1;
int lastReverbRoom = -1;
int lastPresetSlot = -1;
int lastFftRangeIndex = -1;
int lastFftWindow = -1;
int lastFftSmoothing = -1;
int lastFftBinsIndex = -1;
int lastFftPeakHold = -1;
int lastTransportRunning = -1;
int lastSequencerEnabled = -1;
int lastSequencerBpm = -1;
int lastSequencerLength = -1;
int lastSequencerDirection = -1;
int lastSequencerGate = -1;
int lastSequencerTranspose = -999;
int lastSequencerSelectedStep = -1;
int lastSequencerCurrentStep = -1;
int lastSequencerNoteChecksum = -1;
int lastWaveVoiceChecksum = -1;
int lastWaveVoiceParamChecksum = -1;
int lastKickLevel = -1;
int lastKickParamChecksum = -1;
bool lastBtStreaming = false;
uint32_t lastBtPacketDrawn = 0;
uint32_t lastBtUiRefreshMs = 0;

bool lastBtConnected = false;
bool btStartVolumeApplied = false;
uint8_t btStartVolumeApplyCount = 0;
uint32_t lastBtStartVolumeMs = 0;

uint32_t lastBtHeaderRefreshMs = 0;
uint32_t lastBtRssiRequestMs = 0;
int8_t btRssiDelta = 0;
bool btRssiValid = false;

const uint32_t bootDurationMs = 3600;
const uint32_t bootCompleteHoldMs = 240;
uint32_t bootStartMs = 0;
uint32_t bootCompletedAtMs = 0;
bool bootReady = false;
int lastBootProgress = -1;

#include "PresetStore.h"

#include "AppControl.h"

#include "DisplayCore.h"

#include "AudioEngine.h"

#include "SignalAnalysis.h"

#include "UiWidgets.h"

#include "UiScreens.h"

#include "AppLifecycle.h"

// =======================
// SETUP
// =======================

void setup() {
  initApp();
}

// =======================
// LOOP
// =======================

void loop() {
  runAppFrame();
}
