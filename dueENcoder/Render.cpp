#include "Render.h"

#include <math.h>
#include <stdio.h>

#include "AppConfig.h"
#include "Hardware.h"
#include "Pong.h"

void drawMenuScreen();
void drawMenuItem(int x, int y, const char* text, bool selected);
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

float mapFloat(float value, float inMin, float inMax, float outMin, float outMax);
float constrainFloat(float value, float minValue, float maxValue);
float applyCurve(float t, float curve);
float smoothStep(float edge0, float edge1, float x);
float smoothedPulse(float phase, float duty, float edgeWidth);
int normalizedToY(float value, int y, int h);
void drawThickLine(int x1, int y1, int x2, int y2, int thickness);

void drawScreen() {
  if (menuOpen) {
    drawMenuScreen();
    display.display();
    return;
  }

  if (currentMode == MODE_ENCODERS) {
    drawEncoderScreen();
  } else if (currentMode == MODE_WAVEFORM) {
    drawWaveformScreen();
  } else if (currentMode == MODE_PONG) {
    drawPongScreen();
  }

  display.display();
}

// ======================================================
//  MENU + COMMON TOP BAR
// ======================================================

void drawMenuScreen() {
  display.clearDisplay();

  drawTitleBar("MENU");

  drawMenuItem(8, 22, "ENCODER VIEW", menuSelection == MODE_ENCODERS);
  drawMenuItem(8, 38, "WAVEFORM VIEW", menuSelection == MODE_WAVEFORM);
  drawMenuItem(8, 54, "PONG", menuSelection == MODE_PONG);

  if (millis() < clickFlashUntil3) {
    display.drawRect(0, UI_HEADER_HEIGHT, 128, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
  }
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

void drawEncoderScreen() {
  long v1;
  long v2;

  noInterrupts();
  v1 = enc1.value;
  v2 = enc2.value;
  interrupts();

  display.clearDisplay();

  drawTopInfoBar("ENC", v1 % 100, v2 % 100);

  if (millis() < clickFlashUntil3) {
    display.drawRect(0, UI_HEADER_HEIGHT, 128, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
  }

  drawTickDial(32, 42, 20, v1, millis() < clickFlashUntil1);
  drawTickDial(96, 42, 20, v2, millis() < clickFlashUntil2);
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

  if (millis() < clickFlashUntil3) {
    display.drawRect(0, UI_HEADER_HEIGHT, 128, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
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

    display.setCursor(34, 25);
    display.print("GAME OVER");

    display.setCursor(33, 39);

    if (pongScoreLeft > pongScoreRight) {
      display.print("LEFT WINS");
    } else {
      display.print("RIGHT WINS");
    }

    display.setCursor(16, 54);
    display.print("CLICK E1/E2 RESET");

    return;
  }

  display.drawRect(0, UI_HEADER_HEIGHT, 128, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);

  for (int y = UI_CONTENT_TOP + 2; y < PONG_BOTTOM - 1; y += 6) {
    display.drawFastVLine(64, y, 3, SSD1306_WHITE);
  }

  drawPongPaddle(PONG_LEFT_X, leftPaddleY);
  drawPongPaddle(PONG_RIGHT_X, rightPaddleY);

  display.fillCircle((int)ballX, (int)ballY, 2, SSD1306_WHITE);

  if (millis() < clickFlashUntil1) {
    display.drawRect(1, UI_CONTENT_TOP, 8, PONG_BOTTOM - UI_CONTENT_TOP, SSD1306_WHITE);
  }

  if (millis() < clickFlashUntil2) {
    display.drawRect(119, UI_CONTENT_TOP, 8, PONG_BOTTOM - UI_CONTENT_TOP, SSD1306_WHITE);
  }

  if (millis() < clickFlashUntil3) {
    display.drawRect(0, UI_HEADER_HEIGHT, 128, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
  }
}

void drawPongTopBar() {
  display.fillRect(0, 0, 128, UI_HEADER_HEIGHT, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);

  display.setCursor(2, 4);
  display.print("PONG");

  display.setCursor(48, 4);
  display.print(pongScoreLeft);

  display.setCursor(60, 4);
  display.print("-");

  display.setCursor(72, 4);
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
