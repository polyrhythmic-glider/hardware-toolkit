#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// =======================
// PIN DISPLAY
// =======================

#define TFT_CS    5
#define TFT_DC    4
#define TFT_RST   2
#define TFT_SCK   18
#define TFT_MOSI  23

// =======================
// PIN ENCODER / BUTTON
// =======================

#define ENC_A     32
#define ENC_B     33
#define ENC_SW    25
#define BTN_K0    26

// =======================
// DISPLAY
// =======================

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const int SCREEN_W = 160;
const int SCREEN_H = 128;

// =======================
// ENCODER ISR
// =======================

portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

volatile int32_t encoderRawTicks = 0;
volatile uint8_t encoderLastState = 0;

const int8_t encoderTable[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void IRAM_ATTR encoderISR() {
  uint8_t a = digitalRead(ENC_A);
  uint8_t b = digitalRead(ENC_B);

  uint8_t newState = (a << 1) | b;
  uint8_t index = (encoderLastState << 2) | newState;

  int8_t movement = encoderTable[index];
  encoderLastState = newState;

  if (movement == 0) return;

  portENTER_CRITICAL_ISR(&encoderMux);
  encoderRawTicks += movement;
  portEXIT_CRITICAL_ISR(&encoderMux);
}

int32_t consumeEncoderTicks() {
  int32_t ticks = 0;

  portENTER_CRITICAL(&encoderMux);
  ticks = encoderRawTicks;
  encoderRawTicks = 0;
  portEXIT_CRITICAL(&encoderMux);

  return ticks;
}

// =======================
// INPUT
// =======================

const int ENCODER_STEPS_PER_DETENT = 2;
int32_t encoderSubSteps = 0;

const uint32_t debounceMs = 35;

bool lastPushState = HIGH;
bool lastK0State = HIGH;

uint32_t lastPushDebounceMs = 0;
uint32_t lastK0DebounceMs = 0;

bool pushEvent = false;
bool k0Event = false;

int32_t getEncoderSteps() {
  int32_t ticks = consumeEncoderTicks();

  if (ticks == 0) return 0;

  encoderSubSteps += ticks;

  int32_t steps = 0;

  while (encoderSubSteps >= ENCODER_STEPS_PER_DETENT) {
    steps++;
    encoderSubSteps -= ENCODER_STEPS_PER_DETENT;
  }

  while (encoderSubSteps <= -ENCODER_STEPS_PER_DETENT) {
    steps--;
    encoderSubSteps += ENCODER_STEPS_PER_DETENT;
  }

  return steps;
}

void updateButtons() {
  uint32_t now = millis();

  bool pushNow = digitalRead(ENC_SW);
  bool k0Now = digitalRead(BTN_K0);

  if (pushNow != lastPushState && now - lastPushDebounceMs > debounceMs) {
    lastPushDebounceMs = now;
    lastPushState = pushNow;

    if (pushNow == LOW) {
      pushEvent = true;
    }
  }

  if (k0Now != lastK0State && now - lastK0DebounceMs > debounceMs) {
    lastK0DebounceMs = now;
    lastK0State = k0Now;

    if (k0Now == LOW) {
      k0Event = true;
    }
  }
}

// =======================
// MENU DATA
// =======================

enum UiMode {
  MODE_MENU,
  MODE_EDIT,
  MODE_ABOUT
};

UiMode uiMode = MODE_MENU;
UiMode lastDrawnMode = MODE_ABOUT;

const char* waveformNames[] = {
  "SINE",
  "SQUARE",
  "TRI",
  "SAW"
};

int waveform = 0;
int frequency = 220;
int volume = 80;
int attack = 10;
int decay = 80;

struct MenuItem {
  const char* label;
};

MenuItem menuItems[] = {
  { "Waveform" },
  { "Frequency" },
  { "Volume" },
  { "Attack" },
  { "Decay" },
  { "About" }
};

const int MENU_COUNT = sizeof(menuItems) / sizeof(menuItems[0]);

int selectedItem = 0;
int lastSelectedItem = -1;

int topItem = 0;
int lastTopItem = -1;

const int VISIBLE_ROWS = 4;

bool fullRedraw = true;
bool valueDirty = true;

int lastWaveform = -1;
int lastFrequency = -1;
int lastVolume = -1;
int lastAttack = -1;
int lastDecay = -1;

// =======================
// UTILITY
// =======================

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

void wrapSelectedMenu() {
  if (selectedItem < 0) selectedItem = MENU_COUNT - 1;
  if (selectedItem >= MENU_COUNT) selectedItem = 0;

  if (selectedItem < topItem) {
    topItem = selectedItem;
  }

  if (selectedItem >= topItem + VISIBLE_ROWS) {
    topItem = selectedItem - VISIBLE_ROWS + 1;
  }
}

const char* getValueText(int itemIndex) {
  static char buffer[24];

  switch (itemIndex) {
    case 0:
      snprintf(buffer, sizeof(buffer), "%s", waveformNames[waveform]);
      break;

    case 1:
      snprintf(buffer, sizeof(buffer), "%dHz", frequency);
      break;

    case 2:
      snprintf(buffer, sizeof(buffer), "%d%%", volume);
      break;

    case 3:
      snprintf(buffer, sizeof(buffer), "%dms", attack);
      break;

    case 4:
      snprintf(buffer, sizeof(buffer), "%dms", decay);
      break;

    case 5:
      snprintf(buffer, sizeof(buffer), "Info");
      break;

    default:
      snprintf(buffer, sizeof(buffer), "-");
      break;
  }

  return buffer;
}

void changeCurrentValue(int delta) {
  switch (selectedItem) {
    case 0:
      waveform += delta;
      if (waveform < 0) waveform = 3;
      if (waveform > 3) waveform = 0;
      break;

    case 1:
      frequency += delta * 5;
      frequency = clampInt(frequency, 20, 2000);
      break;

    case 2:
      volume += delta;
      volume = clampInt(volume, 0, 100);
      break;

    case 3:
      attack += delta;
      attack = clampInt(attack, 0, 500);
      break;

    case 4:
      decay += delta;
      decay = clampInt(decay, 0, 2000);
      break;
  }

  valueDirty = true;
}

// =======================
// UI LOGIC
// =======================

void setMode(UiMode newMode) {
  uiMode = newMode;
  fullRedraw = true;
  valueDirty = true;
}

void updateUiLogic() {
  int32_t steps = getEncoderSteps();

  if (steps != 0) {
    if (uiMode == MODE_MENU) {
      selectedItem += steps;
      wrapSelectedMenu();

      fullRedraw = false;
    }

    else if (uiMode == MODE_EDIT) {
      changeCurrentValue(steps);
    }
  }

  if (pushEvent) {
    pushEvent = false;

    if (uiMode == MODE_MENU) {
      if (selectedItem == 5) {
        setMode(MODE_ABOUT);
      } else {
        setMode(MODE_EDIT);
      }
    }

    else if (uiMode == MODE_EDIT) {
      setMode(MODE_MENU);
    }

    else if (uiMode == MODE_ABOUT) {
      setMode(MODE_MENU);
    }
  }

  if (k0Event) {
    k0Event = false;

    if (uiMode == MODE_EDIT || uiMode == MODE_ABOUT) {
      setMode(MODE_MENU);
    }
  }
}

// =======================
// FAST DRAW HELPERS
// =======================

void drawHeader(const char* title) {
  tft.fillRect(0, 0, SCREEN_W, 20, ST77XX_BLUE);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 6);
  tft.print(title);

  tft.setCursor(124, 6);
  tft.print("PG01");
}

void drawFooter(const char* text) {
  tft.fillRect(0, 116, SCREEN_W, 12, ST77XX_BLACK);
  tft.drawFastHLine(0, 115, SCREEN_W, ST77XX_BLUE);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 119);
  tft.print(text);
}

// =======================
// MENU DRAW
// =======================

void drawMenuBase() {
  tft.fillScreen(ST77XX_BLACK);

  drawHeader("POLYGLIDER");
  drawFooter("ROTATE MOVE  PUSH ENTER");

  lastSelectedItem = -1;
  lastTopItem = -1;
}

void drawMenuRow(int row, int itemIndex) {
  int y = 25 + row * 22;

  bool selected = itemIndex == selectedItem;

  if (selected) {
    tft.fillRoundRect(4, y, 152, 18, 3, ST77XX_YELLOW);
    tft.setTextColor(ST77XX_BLACK);
  } else {
    tft.fillRect(4, y, 152, 18, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
  }

  tft.setTextSize(1);

  tft.setCursor(10, y + 5);
  tft.print(menuItems[itemIndex].label);

  tft.setCursor(96, y + 5);
  tft.print(getValueText(itemIndex));
}

void drawVisibleMenuRows() {
  for (int row = 0; row < VISIBLE_ROWS; row++) {
    int itemIndex = topItem + row;

    if (itemIndex >= MENU_COUNT) {
      tft.fillRect(4, 25 + row * 22, 152, 18, ST77XX_BLACK);
    } else {
      drawMenuRow(row, itemIndex);
    }
  }

  lastSelectedItem = selectedItem;
  lastTopItem = topItem;
}

void updateMenuFast() {
  if (fullRedraw || lastDrawnMode != MODE_MENU) {
    drawMenuBase();
    drawVisibleMenuRows();

    fullRedraw = false;
    lastDrawnMode = MODE_MENU;
    return;
  }

  if (topItem != lastTopItem) {
    drawVisibleMenuRows();
    return;
  }

  if (selectedItem != lastSelectedItem) {
    int oldRow = lastSelectedItem - topItem;
    int newRow = selectedItem - topItem;

    if (oldRow >= 0 && oldRow < VISIBLE_ROWS) {
      drawMenuRow(oldRow, lastSelectedItem);
    }

    if (newRow >= 0 && newRow < VISIBLE_ROWS) {
      drawMenuRow(newRow, selectedItem);
    }

    lastSelectedItem = selectedItem;
  }
}

// =======================
// EDIT DRAW
// =======================

void drawEditBase() {
  tft.fillScreen(ST77XX_BLACK);

  drawHeader("EDIT");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 30);
  tft.print(menuItems[selectedItem].label);

  tft.drawFastHLine(8, 45, 144, ST77XX_BLUE);

  tft.drawRect(8, 94, 144, 10, ST77XX_WHITE);

  drawFooter("ROTATE EDIT  PUSH/K0 BACK");

  lastWaveform = -1;
  lastFrequency = -1;
  lastVolume = -1;
  lastAttack = -1;
  lastDecay = -1;

  valueDirty = true;
}

int getCurrentValueForBar() {
  switch (selectedItem) {
    case 0:
      return map(waveform, 0, 3, 0, 142);

    case 1:
      return map(frequency, 20, 2000, 0, 142);

    case 2:
      return map(volume, 0, 100, 0, 142);

    case 3:
      return map(attack, 0, 500, 0, 142);

    case 4:
      return map(decay, 0, 2000, 0, 142);
  }

  return 0;
}

bool editValueChanged() {
  if (waveform != lastWaveform) return true;
  if (frequency != lastFrequency) return true;
  if (volume != lastVolume) return true;
  if (attack != lastAttack) return true;
  if (decay != lastDecay) return true;

  return false;
}

void saveLastValues() {
  lastWaveform = waveform;
  lastFrequency = frequency;
  lastVolume = volume;
  lastAttack = attack;
  lastDecay = decay;
}

void drawEditValue() {
  if (!valueDirty && !editValueChanged()) {
    return;
  }

  valueDirty = false;
  saveLastValues();

  tft.fillRect(8, 55, 144, 32, ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(8, 61);
  tft.print(getValueText(selectedItem));

  tft.fillRect(9, 95, 142, 8, ST77XX_BLACK);

  int fillW = getCurrentValueForBar();
  tft.fillRect(9, 95, fillW, 8, ST77XX_CYAN);
}

void updateEditFast() {
  if (fullRedraw || lastDrawnMode != MODE_EDIT) {
    drawEditBase();

    fullRedraw = false;
    lastDrawnMode = MODE_EDIT;
  }

  drawEditValue();
}

// =======================
// ABOUT DRAW
// =======================

void drawAboutScreen() {
  tft.fillScreen(ST77XX_BLACK);

  drawHeader("ABOUT");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(10, 32);
  tft.print("POLYGLIDER");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 58);
  tft.print("ESP32 + ST7735S");

  tft.setCursor(10, 74);
  tft.print("EC11 encoder");

  tft.setCursor(10, 90);
  tft.print("Partial redraw UI");

  drawFooter("PUSH / K0 BACK");

  lastDrawnMode = MODE_ABOUT;
  fullRedraw = false;
}

// =======================
// DRAW DISPATCH
// =======================

void drawUi() {
  if (uiMode == MODE_MENU) {
    updateMenuFast();
  }

  else if (uiMode == MODE_EDIT) {
    updateEditFast();
  }

  else if (uiMode == MODE_ABOUT) {
    if (fullRedraw || lastDrawnMode != MODE_ABOUT) {
      drawAboutScreen();
    }
  }
}

// =======================
// SETUP
// =======================

void setup() {
  Serial.begin(115200);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(BTN_K0, INPUT_PULLUP);

  delay(100);

  encoderLastState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  // Se è stabile, prova 40000000.
  // Se vedi glitch o pixel strani, torna a 27000000.
  tft.setSPISpeed(40000000);

  tft.fillScreen(ST77XX_BLACK);

  fullRedraw = true;

  Serial.println("POLYGLIDER FAST UI START");
}

// =======================
// LOOP
// =======================

void loop() {
  updateButtons();
  updateUiLogic();
  drawUi();
}