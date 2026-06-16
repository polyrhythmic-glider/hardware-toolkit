#include "Render.h"

#include <math.h>
#include <stdio.h>

#include "AppConfig.h"
#include "Hardware.h"
#include "Pong.h"

uint8_t activeDisplayIndex = 0;

Encoder& getEncoderForDisplay(uint8_t displayIndex);
unsigned long getClickFlashForDisplay(uint8_t displayIndex);
const char* getEncoderTitle(uint8_t displayIndex);

void drawMenuScreen();
void drawUtilityScreen();
void drawMenuUtilityScreen();
void drawTestUtilityScreen();
void drawEncoderMainScreen();
void drawEncoderUtilityScreen();
void drawClickSummaryScreen();
void drawWaveformUtilityScreen();
void drawDebugUtilityScreen();
void drawStatusUtility(const char* title);
void printEncoderLine(const char* label, Encoder& enc);
void printEncoderPinLine(const char* label, Encoder& enc);
void drawMenuItem(int x, int y, const char* text, bool selected);
const char* getMenuItemLabel(int mode);
int getMenuFirstVisibleIndex();
void drawTitleBar(const char* modeName);
void drawTopInfoBar(const char* modeName, int leftValue, int rightValue);

void drawEncoderScreen();
void drawTickDial(int cx, int cy, int r, long value, bool flash);
void drawTickRing(int cx, int cy, int r, bool flash);
float continuousAngle(long value);
void drawThickNeedle(int x1, int y1, int x2, int y2, float angle, int thickness, int color);
void drawCenteredSignedValue(int cx, int cy, long value);

void drawWaveformScreen();
const char* getWaveformName(int index);
const char* getParameterALabel(int waveform);
const char* getParameterBLabel(int waveform);
void printSignedTiny(int value);
void drawWaveTopBar(int waveform, int paramA, int paramB);
void drawBigWaveform(int waveform, int paramA, int paramB);
void drawSineWave(int x, int y, int w, int h, int driveParam, int warpParam);
void drawTriWave(int x, int y, int w, int h, int skewParam, int curveParam);
void drawStepWave(int x, int y, int w, int h, int dutyParam, int edgeParam);
void drawConstWave(int x, int y, int w, int h, int levelParam, int rippleParam);

void drawPongScreen();
void drawPongTopBar();
void drawPongPaddle(int x, int centerY);
void drawStartupLogoPanel();
void drawStartupGliderMark(int originX, int originY, int cellSize);

float mapFloat(float value, float inMin, float inMax, float outMin, float outMax);
float constrainFloat(float value, float minValue, float maxValue);
float applyCurve(float t, float curve);
float smoothStep(float edge0, float edge1, float x);
float smoothedPulse(float phase, float duty, float edgeWidth);
int normalizedToY(float value, int y, int h);
void drawThickLine(int x1, int y1, int x2, int y2, int thickness);

void drawScreen() {
  if (!displayHardwareReady) {
    return;
  }

  if (DIRECT_OLED_TEST || PCA_MUX_TEST) {
    return;
  }

  if (!pca9548aFound) {
    activeDisplayIndex = 0;

    if (MULTI_SCREEN_UI) {
      drawUtilityScreen();
    } else {
      if (menuOpen) {
        drawMenuScreen();
      } else if (currentMode == MODE_ENCODERS) {
        drawEncoderScreen();
      } else if (currentMode == MODE_WAVEFORM) {
        drawWaveformScreen();
      } else if (currentMode == MODE_PONG) {
        drawPongScreen();
      } else if (currentMode == MODE_TEST) {
        drawTestUtilityScreen();
      }
    }

    display.display();
    return;
  }

  for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
    selectDisplay(i);
    activeDisplayIndex = i;

    if (MULTI_SCREEN_UI) {
      if (currentMode == MODE_PONG && !menuOpen) {
        drawPongScreen();
      } else {
        drawUtilityScreen();
      }
    } else {
      if (menuOpen) {
        drawMenuScreen();
      } else if (currentMode == MODE_ENCODERS) {
        drawEncoderScreen();
      } else if (currentMode == MODE_WAVEFORM) {
        drawWaveformScreen();
      } else if (currentMode == MODE_PONG) {
        drawPongScreen();
      } else if (currentMode == MODE_TEST) {
        drawTestUtilityScreen();
      }
    }

    display.display();
  }

  selectDisplay(0);
}

void drawStartupLogo(unsigned long holdMs) {
  if (!displayHardwareReady) {
    return;
  }

  if (!pca9548aFound) {
    activeDisplayIndex = 0;
    drawStartupLogoPanel();
    display.display();
    delay(holdMs);
    return;
  }

  for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
    selectDisplay(i);
    activeDisplayIndex = i;
    drawStartupLogoPanel();
    display.display();
  }

  selectDisplay(0);
  delay(holdMs);
}

void drawStartupLogoPanel() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.drawFastHLine(8, 8, SCREEN_WIDTH - 16, SSD1306_WHITE);
  display.drawFastHLine(8, SCREEN_HEIGHT - 9, SCREEN_WIDTH - 16, SSD1306_WHITE);

  if (pca9548aFound && MULTI_SCREEN_UI) {
    switch (activeDisplayIndex) {
      case 0:
        display.setTextSize(1);
        display.setCursor(32, 19);
        display.print("MADE WITH");
        display.setCursor(50, 31);
        display.print("LOVE");
        display.setCursor(55, 43);
        display.print("BY");
        break;

      case 1:
        display.setTextSize(2);
        display.setCursor(27, 12);
        display.print("POLY");
        display.setCursor(15, 30);
        display.print("RHYTHMIC");
        display.setTextSize(1);
        display.setCursor(43, 51);
        display.print("GLIDER");
        break;

      case 2:
        drawStartupGliderMark(38, 11, 14);
        break;

      default:
        display.setTextSize(2);
        display.setCursor(22, 22);
        display.print("LOADING");
        break;
    }
  } else {
    display.setTextSize(2);
    display.setCursor(27, 10);
    display.print("POLY");
    display.setCursor(15, 29);
    display.print("RHYTH");
    display.setTextSize(1);
    display.setCursor(43, 51);
    display.print("GLIDER");
  }
}

void drawStartupGliderMark(int originX, int originY, int cellSize) {
  const uint8_t glider[5][5] = {
    {0, 1, 0, 0, 0},
    {0, 0, 1, 0, 0},
    {1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}
  };

  for (uint8_t y = 0; y < 5; y++) {
    for (uint8_t x = 0; x < 5; x++) {
      if (glider[y][x]) {
        display.fillRect(originX + x * cellSize, originY + y * cellSize, cellSize - 2, cellSize - 2, SSD1306_WHITE);
      }
    }
  }
}

Encoder& getEncoderForDisplay(uint8_t displayIndex) {
  switch (displayIndex) {
    case 0: return enc1;
    case 1: return enc2;
    case 2: return enc3;
    default: return enc4;
  }
}

unsigned long getClickFlashForDisplay(uint8_t displayIndex) {
  switch (displayIndex) {
    case 0: return clickFlashUntil1;
    case 1: return clickFlashUntil2;
    case 2: return clickFlashUntil3;
    default: return clickFlashUntil4;
  }
}

const char* getEncoderTitle(uint8_t displayIndex) {
  switch (displayIndex) {
    case 0: return "ENC 1";
    case 1: return "ENC 2";
    case 2: return "ENC 3";
    default: return "ENC 4";
  }
}

// ======================================================
//  MENU + COMMON TOP BAR
// ======================================================

void drawUtilityScreen() {
  if (debugMode && currentMode != MODE_TEST && activeDisplayIndex != 3 && !(currentMode == MODE_PONG && !menuOpen)) {
    drawDebugUtilityScreen();
    return;
  }

  if (menuOpen) {
    if (activeDisplayIndex == 3) {
      drawMenuScreen();
    } else {
      drawMenuUtilityScreen();
    }

    return;
  }

  if (currentMode == MODE_ENCODERS) {
    drawEncoderScreen();
  } else if (currentMode == MODE_WAVEFORM) {
    if (activeDisplayIndex == 0) {
      drawWaveformScreen();
    } else {
      drawWaveformUtilityScreen();
    }
  } else if (currentMode == MODE_PONG) {
    drawPongScreen();
  } else if (currentMode == MODE_TEST) {
    drawTestUtilityScreen();
  }
}

void drawDebugUtilityScreen() {
  display.clearDisplay();

  if (activeDisplayIndex == 1) {
    drawStatusUtility("DEBUG BUS");
    display.setCursor(0, 20);
    display.print("SERIAL ");
    display.println(debugMode ? "ON" : "OFF");
    display.print("I2C ");
    display.print(I2C_CLOCK_HZ / 1000);
    display.println("kHz");
    display.print("PCA ");
    display.println(pca9548aFound ? "OK" : "NO");
    display.print("OLED 0x");
    display.println(activeOledAddress, HEX);
  } else if (activeDisplayIndex == 2) {
    drawStatusUtility("DEBUG PINS");
    display.setCursor(0, 18);
    printEncoderPinLine("E1", enc1);
    printEncoderPinLine("E2", enc2);
    printEncoderPinLine("E3", enc3);
    printEncoderPinLine("E4", enc4);
  } else {
    drawStatusUtility("DEBUG STATE");
    display.setCursor(0, 20);
    display.print("MODE ");
    display.println((int)currentMode);
    display.print("MENU ");
    display.println(menuOpen ? "OPEN" : "CLOSED");
    display.print("SEL ");
    display.println(menuSelection);
    display.print("DISP ");
    display.println(activeDisplayIndex);
  }
}

void drawMenuScreen() {
  display.clearDisplay();

  drawTitleBar("MENU");

  int firstVisible = getMenuFirstVisibleIndex();

  for (int i = 0; i < MENU_VISIBLE_ITEMS; i++) {
    int item = firstVisible + i;

    if (item >= MODE_COUNT) {
      break;
    }

    drawMenuItem(8, 22 + (i * 16), getMenuItemLabel(item), menuSelection == item);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);

  if (firstVisible > 0) {
    display.setCursor(101, 4);
    display.print("^");
  }

  if (firstVisible + MENU_VISIBLE_ITEMS < MODE_COUNT) {
    display.setCursor(116, 4);
    display.print("v");
  }

  display.setTextColor(SSD1306_WHITE);

  if (millis() < clickFlashUntil4) {
    display.drawRect(0, UI_HEADER_HEIGHT, 128, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
  }
}

void drawMenuUtilityScreen() {
  display.clearDisplay();

  if (activeDisplayIndex == 0) {
    drawStatusUtility("ENC 1");
    display.setCursor(0, 20);
    printEncoderLine("E1", enc1);
    display.println("SCREEN 1");
    display.println("LOCAL ENCODER");
  } else if (activeDisplayIndex == 1) {
    drawStatusUtility("BUS");
    display.setCursor(0, 20);
    display.print("I2C ");
    display.print(I2C_CLOCK_HZ / 1000);
    display.println("kHz");
    display.print("PCA ");
    display.println(pca9548aFound ? "FOUND" : "MISSING");
    display.print("OLED 0x");
    display.println(activeOledAddress, HEX);
  } else if (activeDisplayIndex == 2) {
    drawStatusUtility("ENCODERS");
    display.setCursor(0, 20);
    printEncoderLine("E1", enc1);
    printEncoderLine("E2", enc2);
    printEncoderLine("E3", enc3);
    printEncoderLine("E4", enc4);
  }
}

void drawTestUtilityScreen() {
  display.clearDisplay();

  if (activeDisplayIndex == 0) {
    drawStatusUtility("TEST BUS");
    display.setCursor(0, 20);
    display.print("PCA ");
    display.println(pca9548aFound ? "OK" : "NO");
    display.print("ADDR 0x");
    display.println(activePca9548aAddress, HEX);
    display.print("OLED 0x");
    display.println(activeOledAddress, HEX);
    display.print("CLK ");
    display.print(I2C_CLOCK_HZ / 1000);
    display.println("kHz");
  } else if (activeDisplayIndex == 1) {
    drawStatusUtility("TEST PINS");
    display.setCursor(0, 18);
    printEncoderPinLine("E1", enc1);
    printEncoderPinLine("E2", enc2);
    printEncoderPinLine("E3", enc3);
    printEncoderPinLine("E4", enc4);
  } else if (activeDisplayIndex == 2) {
    drawStatusUtility("TEST VALUES");
    display.setCursor(0, 18);
    printEncoderLine("E1", enc1);
    printEncoderLine("E2", enc2);
    printEncoderLine("E3", enc3);
    printEncoderLine("E4", enc4);
  } else {
    drawStatusUtility("TEST MENU");
    display.setCursor(0, 20);
    display.println("E4 PUSH MENU");
    display.print("SERIAL ");
    display.println(debugMode ? "ON" : "OFF");
    display.print("DISP ");
    display.println(DISPLAY_COUNT);
    display.println("OLED CH 0..3");
  }
}

void drawStatusUtility(const char* title) {
  drawTitleBar(title);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void printEncoderLine(const char* label, Encoder& enc) {
  long value;

  noInterrupts();
  value = enc.value;
  interrupts();

  display.print(label);
  display.print(": ");
  display.println(value);
}

void printEncoderPinLine(const char* label, Encoder& enc) {
  display.print(label);
  display.print(" A");
  display.print(digitalRead(enc.pinA));
  display.print(" B");
  display.print(digitalRead(enc.pinB));
  display.print(" S");
  display.println(digitalRead(enc.pinSW));
}

void drawMenuItem(int x, int y, const char* text, bool selected) {
  if (selected) {
    display.fillRect(x - 3, y - 2, 116, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setCursor(x, y);
  display.print(text);

  display.setTextColor(SSD1306_WHITE);
}

const char* getMenuItemLabel(int mode) {
  switch (mode) {
    case MODE_ENCODERS: return "ENCODER VIEW";
    case MODE_WAVEFORM: return "WAVEFORM VIEW";
    case MODE_PONG: return "PONG";
    case MODE_TEST: return "TEST";
    default: return "";
  }
}

int getMenuFirstVisibleIndex() {
  int firstVisible = menuSelection - (MENU_VISIBLE_ITEMS - 1);

  if (firstVisible < 0) {
    firstVisible = 0;
  }

  int maxFirstVisible = MODE_COUNT - MENU_VISIBLE_ITEMS;

  if (firstVisible > maxFirstVisible) {
    firstVisible = maxFirstVisible;
  }

  if (firstVisible < 0) {
    firstVisible = 0;
  }

  return firstVisible;
}

void drawTitleBar(const char* modeName) {
  display.fillRect(0, 0, 128, UI_HEADER_HEIGHT, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);

  display.setCursor(2, 4);
  display.print(modeName);

  display.setTextColor(SSD1306_WHITE);
}

void drawTopInfoBar(const char* modeName, int leftValue, int rightValue) {
  drawTitleBar(modeName);

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);

  display.setCursor(54, 4);
  display.print("A:");
  display.print(leftValue);

  display.setCursor(91, 4);
  display.print("B:");
  display.print(rightValue);

  display.setTextColor(SSD1306_WHITE);
}

// ======================================================
//  ENCODER VIEW
// ======================================================

void drawEncoderMainScreen() {
  long v1;
  long v2;
  long v3;
  long v4;

  noInterrupts();
  v1 = enc1.value;
  v2 = enc2.value;
  v3 = enc3.value;
  v4 = enc4.value;
  interrupts();

  display.clearDisplay();
  drawTitleBar("ENCODERS");

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.print("E1 ");
  display.print(v1);
  display.setCursor(64, 20);
  display.print("E2 ");
  display.print(v2);
  display.setCursor(0, 34);
  display.print("E3 ");
  display.print(v3);
  display.setCursor(64, 34);
  display.print("E4 ");
  display.print(v4);
  display.setCursor(0, 54);
  display.print("E4 PUSH MENU");
}

void drawEncoderUtilityScreen() {
  display.clearDisplay();

  if (activeDisplayIndex == 1) {
    drawEncoderScreen();
  } else if (activeDisplayIndex == 2) {
    drawStatusUtility("BUTTONS");
    display.setCursor(0, 20);
    display.print("SW1 ");
    display.println(digitalRead(enc1.pinSW) == LOW ? "DOWN" : "UP");
    display.print("SW2 ");
    display.println(digitalRead(enc2.pinSW) == LOW ? "DOWN" : "UP");
    display.print("SW3 ");
    display.println(digitalRead(enc3.pinSW) == LOW ? "DOWN" : "UP");
    display.print("SW4 ");
    display.println(digitalRead(enc4.pinSW) == LOW ? "DOWN" : "UP");
  } else {
    drawStatusUtility("ACTIONS");
    display.setCursor(0, 20);
    display.println("E1 PUSH RESET");
    display.println("E2 PUSH RESET");
    display.println("E3 PUSH RESET");
    display.println("E4 PUSH MENU");
  }
}

void drawClickSummaryScreen() {
  display.clearDisplay();
  drawTitleBar("CLICKS");

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 20);
  display.print("E1 ");
  display.print(digitalRead(enc1.pinSW) == LOW ? "DOWN" : "UP  ");
  display.print(millis() < clickFlashUntil1 ? " CLICK" : "");

  display.setCursor(0, 30);
  display.print("E2 ");
  display.print(digitalRead(enc2.pinSW) == LOW ? "DOWN" : "UP  ");
  display.print(millis() < clickFlashUntil2 ? " CLICK" : "");

  display.setCursor(0, 40);
  display.print("E3 ");
  display.print(digitalRead(enc3.pinSW) == LOW ? "DOWN" : "UP  ");
  display.print(millis() < clickFlashUntil3 ? " CLICK" : "");

  display.setCursor(0, 50);
  display.print("E4 ");
  display.print(digitalRead(enc4.pinSW) == LOW ? "DOWN" : "UP  ");
  display.print(millis() < clickFlashUntil4 ? " MENU" : "");
}

void drawEncoderScreen() {
  Encoder& activeEncoder = getEncoderForDisplay(activeDisplayIndex);
  long value;

  noInterrupts();
  value = activeEncoder.value;
  interrupts();

  display.clearDisplay();

  drawTitleBar(getEncoderTitle(activeDisplayIndex));

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(103, 4);
  display.print("CH");
  display.print(activeDisplayIndex);
  display.setTextColor(SSD1306_WHITE);

  drawTickDial(64, 42, 21, value, millis() < getClickFlashForDisplay(activeDisplayIndex));

  display.setTextSize(1);
  display.setCursor(activeDisplayIndex == 3 ? 38 : 34, 57);
  display.print(activeDisplayIndex == 3 ? "PUSH MENU" : "PUSH RESET");
}

void drawTickDial(int cx, int cy, int r, long value, bool flash) {
  drawTickRing(cx, cy, r, flash);

  float angle = continuousAngle(value);

  int x1 = cx + cos(angle) * (r - 11);
  int y1 = cy + sin(angle) * (r - 11);

  int x2 = cx + cos(angle) * (r - 1);
  int y2 = cy + sin(angle) * (r - 1);

  drawThickNeedle(x1, y1, x2, y2, angle, 3, SSD1306_WHITE);

  drawCenteredSignedValue(cx, cy, value);
}

void drawTickRing(int cx, int cy, int r, bool flash) {
  for (int i = 0; i < 12; i++) {
    float angle = (i * 30.0) * DEG_TO_RAD_FACTOR;

    int inner = r - 3;
    int outer = r + 1;

    if (i % 3 == 0) {
      inner = r - 6;
      outer = r + 1;
    }

    int x1 = cx + cos(angle) * inner;
    int y1 = cy + sin(angle) * inner;

    int x2 = cx + cos(angle) * outer;
    int y2 = cy + sin(angle) * outer;

    display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
  }

  if (flash) {
    for (int i = 0; i < 6; i++) {
      float angle = (i * 60.0) * DEG_TO_RAD_FACTOR;

      int x1 = cx + cos(angle) * (r + 3);
      int y1 = cy + sin(angle) * (r + 3);

      int x2 = cx + cos(angle) * (r + 6);
      int y2 = cy + sin(angle) * (r + 6);

      display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    }
  }
}

float continuousAngle(long value) {
  float degrees = -90.0 + (value * 8.0);

  while (degrees >= 360.0) {
    degrees -= 360.0;
  }

  while (degrees < 0.0) {
    degrees += 360.0;
  }

  return degrees * DEG_TO_RAD_FACTOR;
}

void drawThickNeedle(int x1, int y1, int x2, int y2, float angle, int thickness, int color) {
  float px = -sin(angle);
  float py = cos(angle);

  int startOffset = -(thickness / 2);

  for (int i = 0; i < thickness; i++) {
    int offset = startOffset + i;

    int ox = round(px * offset);
    int oy = round(py * offset);

    display.drawLine(x1 + ox, y1 + oy, x2 + ox, y2 + oy, color);
  }
}

void drawCenteredSignedValue(int cx, int cy, long value) {
  int shown = value % 100;

  char buffer[6];
  sprintf(buffer, "%d", shown);

  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.getTextBounds(buffer, 0, 0, &x1, &y1, &w, &h);

  display.setCursor(cx - (w / 2), cy - (h / 2));
  display.print(buffer);
}

// ======================================================
//  WAVEFORM VIEW
// ======================================================

void drawWaveformScreen() {
  display.clearDisplay();

  drawWaveTopBar(waveformIndex, parameterA, parameterB);
  drawBigWaveform(waveformIndex, parameterA, parameterB);

  if (millis() < clickFlashUntil4) {
    display.drawRect(0, UI_HEADER_HEIGHT, 128, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
  }
}

void drawWaveformUtilityScreen() {
  display.clearDisplay();

  if (activeDisplayIndex == 1) {
    drawStatusUtility("PARAMS");
    display.setCursor(0, 20);
    display.print(getParameterALabel(waveformIndex));
    display.print(": ");
    printSignedTiny(parameterA);
    display.println();
    display.print(getParameterBLabel(waveformIndex));
    display.print(": ");
    printSignedTiny(parameterB);
    display.println();
    display.print("TYPE: ");
    display.println(getWaveformName(waveformIndex));
  } else if (activeDisplayIndex == 2) {
    drawStatusUtility("INPUTS");
    display.setCursor(0, 20);
    display.println("E1 TURN: PARAM A");
    display.println("E2 TURN: PARAM B");
    display.println("E1 PUSH: PREV");
    display.println("E2 PUSH: NEXT");
  } else {
    drawStatusUtility("ENCODERS");
    display.setCursor(0, 20);
    printEncoderLine("E1", enc1);
    printEncoderLine("E2", enc2);
    printEncoderLine("E4", enc4);
  }
}

const char* getWaveformName(int index) {
  switch (index) {
    case WAVE_SINE: return "SINE";
    case WAVE_TRI: return "TRI";
    case WAVE_STEP: return "STEP";
    case WAVE_CONST: return "CONST";
    default: return "WAVE";
  }
}

const char* getParameterALabel(int waveform) {
  switch (waveform) {
    case WAVE_SINE: return "DRV";
    case WAVE_TRI: return "SKW";
    case WAVE_STEP: return "DUT";
    case WAVE_CONST: return "LVL";
    default: return "A";
  }
}

const char* getParameterBLabel(int waveform) {
  switch (waveform) {
    case WAVE_SINE: return "WRP";
    case WAVE_TRI: return "CRV";
    case WAVE_STEP: return "EDG";
    case WAVE_CONST: return "RPL";
    default: return "B";
  }
}

void printSignedTiny(int value) {
  if (value > 0) {
    display.print("+");
  }

  display.print(value);
}

void drawWaveTopBar(int waveform, int paramA, int paramB) {
  display.fillRect(0, 0, 128, UI_HEADER_HEIGHT, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);

  display.setCursor(1, 4);
  display.print(getWaveformName(waveform));

  display.setCursor(39, 4);
  display.print(getParameterALabel(waveform));
  display.print(":");
  printSignedTiny(paramA);

  display.setCursor(83, 4);
  display.print(getParameterBLabel(waveform));
  display.print(":");
  printSignedTiny(paramB);

  display.setTextColor(SSD1306_WHITE);

  if (millis() < clickFlashUntil1) {
    display.fillRect(0, UI_CONTENT_TOP, 5, 5, SSD1306_WHITE);
  }

  if (millis() < clickFlashUntil2) {
    display.fillRect(123, UI_CONTENT_TOP, 5, 5, SSD1306_WHITE);
  }
}

void drawBigWaveform(int waveform, int paramA, int paramB) {
  int x = 2;
  int y = 19;
  int w = 124;
  int h = 43;

  display.drawRect(x, y, w, h, SSD1306_WHITE);

  int midY = y + h / 2;
  display.drawLine(x + 2, midY, x + w - 3, midY, SSD1306_WHITE);

  switch (waveform) {
    case WAVE_SINE:
      drawSineWave(x + 5, y + 5, w - 10, h - 10, paramA, paramB);
      break;

    case WAVE_TRI:
      drawTriWave(x + 5, y + 5, w - 10, h - 10, paramA, paramB);
      break;

    case WAVE_STEP:
      drawStepWave(x + 5, y + 5, w - 10, h - 10, paramA, paramB);
      break;

    case WAVE_CONST:
      drawConstWave(x + 5, y + 5, w - 10, h - 10, paramA, paramB);
      break;
  }
}

void drawSineWave(int x, int y, int w, int h, int driveParam, int warpParam) {
  float drive = mapFloat(driveParam, -50, 50, 0.55, 3.20);
  float warp = mapFloat(warpParam, -50, 50, -1.25, 1.25);

  int prevX = x;
  int prevY = normalizedToY(0.0, y, h);

  for (int i = 1; i <= w; i++) {
    float phase = (float)i / (float)w;
    float angle = phase * TWO_PI * 2.0;

    float warpedAngle = angle + sin(angle * 2.0) * warp;
    float v = sin(warpedAngle);

    if (driveParam >= 0) {
      v = tanh(v * drive);
    } else {
      v = v * drive;
    }

    int currentX = x + i;
    int currentY = normalizedToY(v, y, h);

    drawThickLine(prevX, prevY, currentX, currentY, 2);

    prevX = currentX;
    prevY = currentY;
  }
}

void drawTriWave(int x, int y, int w, int h, int skewParam, int curveParam) {
  float skew = mapFloat(skewParam, -50, 50, 0.12, 0.88);
  float curve = mapFloat(curveParam, -50, 50, 0.35, 2.80);

  int prevX = x;
  int prevY = normalizedToY(-1.0, y, h);

  for (int i = 1; i <= w; i++) {
    float phase = (float)i / (float)w;
    float cycles = phase * 2.0;
    float p = cycles - floor(cycles);

    float t;
    float v;

    if (p < skew) {
      t = p / skew;
      t = applyCurve(t, curve);
      v = -1.0 + t * 2.0;
    } else {
      t = (p - skew) / (1.0 - skew);
      t = applyCurve(t, curve);
      v = 1.0 - t * 2.0;
    }

    int currentX = x + i;
    int currentY = normalizedToY(v, y, h);

    drawThickLine(prevX, prevY, currentX, currentY, 2);

    prevX = currentX;
    prevY = currentY;
  }
}

void drawStepWave(int x, int y, int w, int h, int dutyParam, int edgeParam) {
  float duty = mapFloat(dutyParam, -50, 50, 0.05, 0.95);
  float edgeWidth = mapFloat(edgeParam, -50, 50, 0.001, 0.18);

  int prevX = x;
  int prevY = normalizedToY(1.0, y, h);

  for (int i = 1; i <= w; i++) {
    float phase = (float)i / (float)w;
    float cycles = phase * 2.0;
    float p = cycles - floor(cycles);

    float v;

    if (edgeParam <= -45) {
      v = (p < duty) ? 1.0 : -1.0;
    } else {
      v = smoothedPulse(p, duty, edgeWidth);
    }

    int currentX = x + i;
    int currentY = normalizedToY(v, y, h);

    drawThickLine(prevX, prevY, currentX, currentY, 2);

    prevX = currentX;
    prevY = currentY;
  }
}

void drawConstWave(int x, int y, int w, int h, int levelParam, int rippleParam) {
  float level = mapFloat(levelParam, -50, 50, -0.85, 0.85);
  float ripple = mapFloat(abs(rippleParam), 0, 50, 0.0, 0.45);
  float rippleDirection = rippleParam < 0 ? -1.0 : 1.0;

  int prevX = x;
  int prevY = normalizedToY(level, y, h);

  for (int i = 1; i <= w; i++) {
    float phase = (float)i / (float)w;
    float angle = phase * TWO_PI * 2.0;

    float v = level + sin(angle) * ripple * rippleDirection;
    v = constrainFloat(v, -1.0, 1.0);

    int currentX = x + i;
    int currentY = normalizedToY(v, y, h);

    drawThickLine(prevX, prevY, currentX, currentY, 2);

    prevX = currentX;
    prevY = currentY;
  }

  display.drawLine(x, prevY - 4, x, prevY + 4, SSD1306_WHITE);
  display.drawLine(x + w, prevY - 4, x + w, prevY + 4, SSD1306_WHITE);
}

// ======================================================
//  PONG DRAW
// ======================================================

void drawPongScreen() {
  display.clearDisplay();

  drawPongTopBar();

  if (pongGameOver) {
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    if (!WIDE_PONG || activeDisplayIndex == 1 || activeDisplayIndex == 2) {
      display.setCursor(34, 25);
      display.print("GAME OVER");

      display.setCursor(33, 39);

      if (pongScoreLeft > pongScoreRight) {
        display.print("LEFT WINS");
      } else {
        display.print("RIGHT WINS");
      }
    }

    display.setCursor(WIDE_PONG && activeDisplayIndex == 0 ? 10 : 16, 54);
    display.print(WIDE_PONG && activeDisplayIndex == 0 ? "CLICK E1 RESET" : "CLICK E1/E4 RESET");

    return;
  }

  int worldLeft = WIDE_PONG ? activeDisplayIndex * SCREEN_WIDTH : 0;
  int localBallX = (int)ballX - worldLeft;
  int localLeftPaddleX = PONG_LEFT_X - worldLeft;
  int localRightPaddleX = PONG_RIGHT_X - worldLeft;

  if (WIDE_PONG) {
    display.drawFastHLine(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SSD1306_WHITE);
    display.drawFastHLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, SSD1306_WHITE);

    if (activeDisplayIndex == 0) {
      display.drawFastVLine(0, UI_HEADER_HEIGHT, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
    }

    if (activeDisplayIndex == DISPLAY_COUNT - 1) {
      display.drawFastVLine(SCREEN_WIDTH - 1, UI_HEADER_HEIGHT, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
    }
  } else {
    display.drawRect(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
  }

  if (localLeftPaddleX >= -2 && localLeftPaddleX < SCREEN_WIDTH) {
    drawPongPaddle(localLeftPaddleX, leftPaddleY);
  }

  if (localRightPaddleX >= -2 && localRightPaddleX < SCREEN_WIDTH) {
    drawPongPaddle(localRightPaddleX, rightPaddleY);
  }

  if (localBallX >= -2 && localBallX <= SCREEN_WIDTH + 2) {
    display.fillCircle(localBallX, (int)ballY, 2, SSD1306_WHITE);
  }

  if (millis() < clickFlashUntil1) {
    display.drawRect(1, UI_CONTENT_TOP, 8, PONG_BOTTOM - UI_CONTENT_TOP, SSD1306_WHITE);
  }

  if (millis() < clickFlashUntil4) {
    display.drawRect(119, UI_CONTENT_TOP, 8, PONG_BOTTOM - UI_CONTENT_TOP, SSD1306_WHITE);
  }

  if (millis() < clickFlashUntil4) {
    display.drawRect(0, UI_HEADER_HEIGHT, 128, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
  }
}

void drawPongTopBar() {
  display.fillRect(0, 0, 128, UI_HEADER_HEIGHT, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);

  display.setCursor(2, 4);
  display.print("PONG");
  if (WIDE_PONG) {
    display.print(" ");
    display.print(activeDisplayIndex + 1);
  }

  display.setCursor(WIDE_PONG ? 52 : 48, 4);
  display.print(pongScoreLeft);

  display.setCursor(WIDE_PONG ? 64 : 60, 4);
  display.print("-");

  display.setCursor(WIDE_PONG ? 76 : 72, 4);
  display.print(pongScoreRight);

  display.setCursor(101, 4);
  display.print("5PT");

  display.setTextColor(SSD1306_WHITE);
}

void drawPongPaddle(int x, int centerY) {
  int y1 = centerY - PONG_PADDLE_HALF;
  int y2 = centerY + PONG_PADDLE_HALF;

  y1 = constrain(y1, PONG_TOP + 1, PONG_BOTTOM - 1);
  y2 = constrain(y2, PONG_TOP + 1, PONG_BOTTOM - 1);

  display.drawFastVLine(x, y1, y2 - y1, SSD1306_WHITE);
  display.drawFastVLine(x + 1, y1, y2 - y1, SSD1306_WHITE);
}

// ======================================================
//  MATH + DRAW HELPERS
// ======================================================

float mapFloat(float value, float inMin, float inMax, float outMin, float outMax) {
  return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

float constrainFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }

  if (value > maxValue) {
    return maxValue;
  }

  return value;
}

float applyCurve(float t, float curve) {
  t = constrainFloat(t, 0.0, 1.0);

  if (curve >= 1.0) {
    return pow(t, curve);
  }

  return 1.0 - pow(1.0 - t, 1.0 / curve);
}

float smoothStep(float edge0, float edge1, float x) {
  float t = constrainFloat((x - edge0) / (edge1 - edge0), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}

float smoothedPulse(float phase, float duty, float edgeWidth) {
  float rise = smoothStep(0.0, edgeWidth, phase);
  float fall = 1.0 - smoothStep(duty, duty + edgeWidth, phase);

  float highAmount = min(rise, fall);

  if (phase > duty + edgeWidth) {
    highAmount = 0.0;
  }

  return highAmount * 2.0 - 1.0;
}

int normalizedToY(float value, int y, int h) {
  value = constrainFloat(value, -1.0, 1.0);

  int top = y + 3;
  int bottom = y + h - 3;

  return map((int)(value * 100.0), -100, 100, bottom, top);
}

void drawThickLine(int x1, int y1, int x2, int y2, int thickness) {
  for (int i = 0; i < thickness; i++) {
    display.drawLine(x1, y1 + i, x2, y2 + i, SSD1306_WHITE);
  }
}
