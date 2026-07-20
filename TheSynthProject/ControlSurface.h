#pragma once

#include <Arduino.h>

namespace ControlSurface {

enum Mode : uint8_t {
  SYNTH_CONTROL = 0,
  ENCODER_VIEW,
  WAVEFORM,
  PONG,
  DIAGNOSTICS,
  SPLASH,
  MODE_COUNT
};

void begin();
void tick();

bool isReady();
bool isPcaReady();
bool isEncoderExpanderReady();

// Diagnostica utilizzabile dal TFT principale anche quando nessun OLED riesce
// a inizializzarsi. Ogni scansione aggiorna i risultati e li stampa a 115200.
void runOledDiagnostics();
bool isMainSdaHigh();
bool isMainSclHigh();
uint8_t getMainBusDeviceCount();
bool isMainBusAddressPresent(uint8_t address);
bool isNeoKeyAddressPresent(uint8_t address);
uint8_t getPcaAddress();
uint8_t getOledDetectedMask();
uint8_t getOledReadyMask();
uint8_t getOledChannelAddress(uint8_t channel);

// Diagnostica del bus indipendente D19/D15 e del MCP23017. Se il modulo non era
// presente al boot, una nuova scansione prova anche a inizializzarlo senza
// richiedere il riavvio della ESP32.
void runMcpDiagnostics();
bool isEncoderSdaHigh();
bool isEncoderSclHigh();
uint8_t getEncoderBusDeviceCount();
bool isEncoderBusAddressPresent(uint8_t address);
bool isMcpRawPortsValid();
uint16_t getMcpRawPorts();
bool getEncoderPinHigh(uint8_t encoderIndex, uint8_t signalIndex);
long getEncoderCount(uint8_t encoderIndex);

Mode getMode();
void setMode(Mode mode);
void stepMode(int delta);
const char* getModeName();
const char* getModeName(Mode mode);

uint8_t getSynthPage();
const char* getSynthPageName();
const char* getLastParameterLabel();
void formatLastParameterValue(char* output, size_t outputSize);
uint32_t getLastParameterChangeMs();

// The original four-OLED waveform editor keeps its own four oscillator
// algorithms. The main audio task calls these two functions so that only one
// I2S owner exists in the combined firmware.
bool isLegacyWaveformAudioActive();
float renderLegacyWaveformSample(float phase);

void markDirty();

}  // namespace ControlSurface
