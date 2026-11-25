#ifndef HARDWARE_H
#define HARDWARE_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "AppConfig.h"

struct Encoder {
  int pinA;
  int pinB;
  int pinSW;

  volatile long value;
  volatile int8_t accumulator;
  volatile uint8_t lastState;
  volatile unsigned long lastMicros;

  int lastButton;
  int currentButton;
  bool clickEvent;
  unsigned long lastButtonChange;
};

extern Adafruit_SSD1306 display;
extern bool displayHardwareReady;
extern bool pca9548aFound;
extern uint8_t activePca9548aAddress;
extern uint8_t activeOledAddress;

extern Encoder enc1;
extern Encoder enc2;
extern Encoder enc3;
extern Encoder enc4;

void setupDisplay();
void setupDirectOledTest();
void drawPcaChannelTestScreen(uint8_t channel, bool oledPresent, bool displayStarted);
void selectPca9548aChannel(uint8_t channel);
void selectDisplay(uint8_t displayIndex);
void disablePca9548aChannels();
bool findPca9548aAddress();
bool isI2cDevicePresent(uint8_t address);
void printI2cDiagnostics();
void printActiveI2cScan(const char* label);

void IRAM_ATTR updateEncoder(Encoder* enc);
void IRAM_ATTR readEnc1();
void IRAM_ATTR readEnc2();
void IRAM_ATTR readEnc3();
void IRAM_ATTR readEnc4();

void setupEncoderPins(Encoder& enc);
void setupAllEncoders();
void attachEncoderInterrupts();

uint8_t readEncoderState(Encoder& enc);

void readButtons();
void readSingleButton(Encoder& enc);
void clearClickEvents();

void resetEncoderValue(Encoder& enc);

#endif
