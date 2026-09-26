#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <math.h>
#include <string.h>
#include "SynthConfig.h"
#include "HardwarePins.h"
#include "ModulationModel.h"
#include "MenuModel.h"
#include "MixerModel.h"
#include "SequencerModel.h"
#include "SynthMath.h"
#include "ControlSurface.h"
#include "SerialDebug.h"

// =======================
// DISPLAY
// =======================

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
// Keep link diagnostics available from the first boot frame. Pairing failures
// happen before the user can reach Utility > Serial Diagnostics. Once the
// handshake is stable this is switched off by AppLifecycle; Utility can then
// enable it explicitly for a live monitor.
volatile bool serialDiagnosticsEnabled = true;
bool serialDiagnosticsAutoMuteArmed = true;

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
namespace DaisyBridge { bool isConnected(); }
bool daisyBootBypassed = false;
bool isDaisyPairingReady() { return DaisyBridge::isConnected(); }
bool isDaisyBootBypassed() { return daisyBootBypassed; }


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
int volume = DEFAULT_OUTPUT_VOLUME_PERCENT;
AdsrParameters envelopeParameters[ENVELOPE_COUNT] = {
  { 10, 80, 68, 320 },
  { 10, 80, 68, 320 },
  { 10, 80, 68, 320 }
};
bool envelopeEnabled[ENVELOPE_COUNT] = { true, false, false };
int envelopeAmountPercent[ENVELOPE_COUNT] = { 100, 100, 100 };
int envelopeVelocitySensitivityPercent[ENVELOPE_COUNT] = { 0, 0, 0 };
int envelopeRetriggerMode[ENVELOPE_COUNT] = { 1, 1, 1 };
ModRouteSettings modulationRoutes[MOD_SOURCE_COUNT][MOD_DESTINATION_COUNT] = {};
// Compatibility aliases keep the existing control-surface page focused on
// ENV1 while the TFT menu can select either independent envelope.
int& attack = envelopeParameters[ENVELOPE_ENV1].attackMs;
int& decay = envelopeParameters[ENVELOPE_ENV1].decayMs;
int& sustain = envelopeParameters[ENVELOPE_ENV1].sustainPercent;
int& releaseMs = envelopeParameters[ENVELOPE_ENV1].releaseMs;
int waveFold = 0;
int wavePhase = 0;
bool filterEnabled = false;
int filterType = 0;
int filterCutoff = 2400;
int filterResonance = 15;
int synthNoiseLevel = 0;
int synthPan = 0;
int delayMix = 0;
int delayTimeMs = 280;
int delayFeedback = 35;
int reverbMix = 0;
int reverbRoom = 65;
int presetSlot = 1;
bool presetOperationOk = true;
const char* presetOperationText = "SELECT";
bool transportRunning = false;
bool sequencerEnabled = false;
int sequencerBpm = 120;
int sequencerLength = SEQUENCER_DEFAULT_LENGTH;
int activeSequencerVoice = 0;
int activeSequencerTrack = SEQUENCER_TRACK_VOICE_1;
// Editing selection is optional. Playback has its own playhead and adding a
// note must not leave a second, apparently frozen cursor on that step.
int sequencerSelectedStep = -1;
volatile int sequencerCurrentStep = 0;
int sequencerPage = 0;
bool sequencerAutoScroll = true;
int sequencerStepPitches[WAVE_VOICE_COUNT][SEQUENCER_STEP_COUNT] = {};
bool sequencerStepEnabled[WAVE_VOICE_COUNT][SEQUENCER_STEP_COUNT] = {};
bool kickStepEnabled[SEQUENCER_STEP_COUNT] = {};
uint16_t kickTriggerSequence = 0;
bool oscillatorEnabled[SYNTH_OSCILLATOR_COUNT] = { true, false, false };
int waveVoiceLevels[SYNTH_OSCILLATOR_COUNT] = { 100, 100, 0 };
int waveVoiceShape[SYNTH_OSCILLATOR_COUNT] = { 0, 1, 0 };
int waveVoiceTune[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
int waveVoiceFold[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
int waveVoiceDetune[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
int oscillatorOctave[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
int oscillatorPhasePercent[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
int oscillatorPulseWidthPercent[SYNTH_OSCILLATOR_COUNT] = { 50, 50, 50 };
int oscillatorPanPercent[SYNTH_OSCILLATOR_COUNT] = { 0, 0, 0 };
int oscillatorKeyTrackingPercent[SYNTH_OSCILLATOR_COUNT] = { 100, 100, 100 };
int oscillatorModulationAmountPercent[SYNTH_OSCILLATOR_COUNT] = { 100, 100, 100 };
LfoControlParameters lfoParameters[SYNTH_LFO_COUNT] = {
  { false, LFO_WAVE_SINE, 100, 0, true, 0, false, true, false, 0, MOD_DEST_OSC1_PITCH },
  { false, LFO_WAVE_TRIANGLE, 100, 0, true, 0, false, true, false, 0, MOD_DEST_FILTER_CUTOFF }
};
int kickLevel = 0;
int kickPitch = 48;
int kickDecay = 420;
int kickSweep = 120;
int kickAttack = 55;
int kickDrive = 20;

MenuAction getSelectedAction();
bool isSynthAction(MenuAction action);
void resetSequencerClock();
void triggerKickVoice();
int getSequencerLength();
int getSequencerPage();
int getActiveSequencerTrack();
const char* getSequencerTrackLabel(int track);
void clampSequencerIndexes();
bool isKickTrackAudible();
bool isKickStepEnabled(int stepIndex);
void printSerialDiagnosticsSnapshot();
void printConnectionDiagnosticsSnapshot();
uint8_t getConnectionProblemCount();
void initializeDefaultModulationRouting();

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
int activeEnvelopeIndex = ENVELOPE_ENV1;
int activeModDestinationIndex = MOD_DEST_AMP_LEVEL;

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
int lastSurfaceMode = -1;
int lastSurfacePage = -1;
uint16_t lastMcpRawPorts = 0;
long lastMcpEncoderCount[4] = { 0, 0, 0, 0 };
int lastTransportRunning = -1;
int lastSequencerEnabled = -1;
int lastSequencerBpm = -1;
int lastSequencerLength = -1;
int lastSequencerVoice = -1;
int lastSequencerTrack = -1;
int lastSequencerSelectedStep = -1;
int lastSequencerCurrentStep = -1;
int lastDaisyCursorSynchronized = -1;
int lastDaisyConnected = -1;
int lastSequencerPage = -1;
int lastSequencerAutoScroll = -1;
int lastSequencerNoteChecksum = -1;
int lastWaveVoiceChecksum = -1;
int lastWaveVoiceParamChecksum = -1;
int lastKickLevel = -1;
int lastKickParamChecksum = -1;

const uint32_t bootDurationMs = 3600;
const uint32_t bootCompleteHoldMs = 240;
uint32_t bootStartMs = 0;
uint32_t bootCompletedAtMs = 0;
bool bootReady = false;
int lastBootProgress = -1;

#include "PresetStore.h"

#include "AppControl.h"

#include "SynthRuntime.h"

#include "ControlSurfaceBridgeImpl.h"

#include "DisplayCore.h"

#include "DaisyBridge.h"

#include "ConnectionDiagnostics.h"

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
