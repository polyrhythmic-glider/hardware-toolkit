#include <LedControl.h>

// -------------------------
// 74HC595 - LED blu
// -------------------------

const byte DATA_PIN  = 11; // DS / SER  -> 74HC595 pin 14
const byte CLOCK_PIN = 13; // SHCP      -> 74HC595 pin 11
const byte LATCH_PIN = 10; // STCP      -> 74HC595 pin 12

// -------------------------
// MAX7219 - matrice 8x8
// -------------------------

const byte MAX_DIN_PIN = 7;
const byte MAX_CLK_PIN = 6;
const byte MAX_CS_PIN  = 9;

LedControl lc = LedControl(MAX_DIN_PIN, MAX_CLK_PIN, MAX_CS_PIN, 1);

// -------------------------
// Microfono KY-037
// -------------------------

const byte MIC_AO_PIN = A0;
const byte MIC_DO_PIN = 2;

const bool USE_DIGITAL_TRIGGER = true;
const bool DO_ACTIVE_LOW = true;

// -------------------------
// AGC - Automatic Gain Control
// Usato sia dal VU meter sia dalla matrice
// -------------------------

float autoGain = 4.0;
float minGain = 1.0;
float maxGain = 45.0;

// Il sistema prova a portare il segnale amplificato circa qui
float targetBoostedPeak = 80.0;

// Più alto = gain si adatta più velocemente
float agcUpSpeed = 0.018;
float agcDownSpeed = 0.050;

int noiseFloor = 3;
int silenceThreshold = 2;

// Livelli derivati dal gain automatico
byte vuTargetLevel = 0;
byte matrixAudioLevel = 0;

// -------------------------
// VU meter LED blu
// -------------------------

byte displayedLevel = 0;

byte peakLed = 0;
unsigned long lastPeakFall = 0;
const unsigned long peakFallDelay = 90;

// -------------------------
// MAX7219 frattali
// -------------------------

unsigned long lastMatrixFrame = 0;
byte matrixFrame = 0;
byte matrixMode = 0;

int baseMatrixFrameMs = 120;

byte currentMatrixIntensity = 5;

// Se la matrice è ruotata o specchiata, cambia questi
const bool FLIP_X = false;
const bool FLIP_Y = false;
const bool SWAP_XY = false;

// Buffer automa cellulare
byte cellularRows[8] = {
  0b00011000,
  0b00100100,
  0b01000010,
  0b10000001,
  0b10000001,
  0b01000010,
  0b00100100,
  0b00011000
};

void setup() {
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  pinMode(MIC_DO_PIN, INPUT);

  Serial.begin(9600);

  writeLeds(0);

  lc.shutdown(0, false);
  lc.setIntensity(0, currentMatrixIntensity);
  lc.clearDisplay(0);

  randomSeed(analogRead(A1));

  Serial.println("KY-037 + 74HC595 VU meter + MAX7219 frattali con AGC condiviso");
}

void loop() {
  int rawPeak = readMicPeakToPeak(35);

  int cleanedPeak = rawPeak - noiseFloor;

  if (cleanedPeak < 0) {
    cleanedPeak = 0;
  }

  updateAutoGain(cleanedPeak);

  int boostedPeak = cleanedPeak * autoGain;

  bool digitalTrigger = false;

  if (USE_DIGITAL_TRIGGER) {
    digitalTrigger = readDigitalTrigger();
  }

  // Livello VU meter
  vuTargetLevel = mapBoostedPeakToLevel(boostedPeak, 130);

  // Livello matrice, leggermente più sensibile del VU
  matrixAudioLevel = mapBoostedPeakToLevel(boostedPeak, 105);

  if (digitalTrigger) {
    vuTargetLevel = 8;
    matrixAudioLevel = 8;
  }

  // LED blu a tempo
  updateDisplayLevel(vuTargetLevel);
  updatePeakLed(displayedLevel);

  byte bar = levelToByte(displayedLevel);
  byte peak = peakToByte(peakLed);

  writeLeds(bar | peak);

  // Matrice audio-reactive con lo stesso gain automatico
  updateMatrixBrightness(matrixAudioLevel);
  updateMatrix(matrixAudioLevel, digitalTrigger);

  Serial.print("RawPeak: ");
  Serial.print(rawPeak);

  Serial.print(" | Clean: ");
  Serial.print(cleanedPeak);

  Serial.print(" | Gain: ");
  Serial.print(autoGain, 2);

  Serial.print(" | Boosted: ");
  Serial.print(boostedPeak);

  Serial.print(" | VU: ");
  Serial.print(displayedLevel);

  Serial.print(" | MatrixLevel: ");
  Serial.print(matrixAudioLevel);

  Serial.print(" | MatrixMode: ");
  Serial.print(matrixMode);

  Serial.print(" | DO: ");
  Serial.println(digitalTrigger ? "TRIGGER" : "off");

  delay(12);
}

// -------------------------------------------------
// Audio / KY-037
// -------------------------------------------------

int readMicPeakToPeak(unsigned long sampleWindowMs) {
  unsigned long start = millis();

  int signalMax = 0;
  int signalMin = 1023;

  while (millis() - start < sampleWindowMs) {
    int sample = analogRead(MIC_AO_PIN);

    if (sample > signalMax) {
      signalMax = sample;
    }

    if (sample < signalMin) {
      signalMin = sample;
    }
  }

  return signalMax - signalMin;
}

void updateAutoGain(int cleanedPeak) {
  if (cleanedPeak <= silenceThreshold) {
    // In silenzio alza lentamente il gain, ma senza impazzire
    autoGain += 0.004;

    if (autoGain > maxGain) {
      autoGain = maxGain;
    }

    return;
  }

  float boosted = cleanedPeak * autoGain;

  if (boosted < targetBoostedPeak) {
    // Segnale basso: aumenta il gain piano piano
    float error = targetBoostedPeak - boosted;
    autoGain += error * agcUpSpeed / 10.0;
  } else {
    // Segnale forte: abbassa il gain più velocemente
    float error = boosted - targetBoostedPeak;
    autoGain -= error * agcDownSpeed / 10.0;
  }

  if (autoGain < minGain) {
    autoGain = minGain;
  }

  if (autoGain > maxGain) {
    autoGain = maxGain;
  }
}

byte mapBoostedPeakToLevel(int boostedPeak, int maxPeak) {
  if (boostedPeak <= 0) {
    return 0;
  }

  boostedPeak = constrain(boostedPeak, 0, maxPeak);

  byte level = map(boostedPeak, 0, maxPeak, 0, 8);

  return constrain(level, 0, 8);
}

bool readDigitalTrigger() {
  int value = digitalRead(MIC_DO_PIN);

  if (DO_ACTIVE_LOW) {
    return value == LOW;
  } else {
    return value == HIGH;
  }
}

// -------------------------------------------------
// LED blu con 74HC595
// -------------------------------------------------

void updateDisplayLevel(byte newLevel) {
  // Salita immediata
  if (newLevel > displayedLevel) {
    displayedLevel = newLevel;
    return;
  }

  // Discesa morbida
  if (displayedLevel > 0) {
    displayedLevel--;
  }
}

void updatePeakLed(byte currentLevel) {
  if (currentLevel > peakLed) {
    peakLed = currentLevel;
  }

  if (millis() - lastPeakFall > peakFallDelay) {
    lastPeakFall = millis();

    if (peakLed > 0) {
      peakLed--;
    }
  }
}

byte levelToByte(byte level) {
  if (level == 0) {
    return 0b00000000;
  }

  if (level >= 8) {
    return 0b11111111;
  }

  return (1 << level) - 1;
}

byte peakToByte(byte peakLevel) {
  if (peakLevel == 0) {
    return 0b00000000;
  }

  if (peakLevel > 8) {
    peakLevel = 8;
  }

  return 1 << (peakLevel - 1);
}

void writeLeds(byte value) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, value);
  digitalWrite(LATCH_PIN, HIGH);
}

// -------------------------------------------------
// MAX7219 matrix
// -------------------------------------------------

void updateMatrixBrightness(byte audioLevel) {
  // Luminosità automatica della matrice in base al livello audio
  byte targetIntensity = map(audioLevel, 0, 8, 1, 11);

  targetIntensity = constrain(targetIntensity, 1, 11);

  if (targetIntensity != currentMatrixIntensity) {
    currentMatrixIntensity = targetIntensity;
    lc.setIntensity(0, currentMatrixIntensity);
  }
}

void updateMatrix(byte audioLevel, bool beat) {
  // Più volume = frattale più veloce
  int matrixFrameMs = baseMatrixFrameMs - audioLevel * 10;

  if (matrixFrameMs < 25) {
    matrixFrameMs = 25;
  }

  if (millis() - lastMatrixFrame < matrixFrameMs) {
    return;
  }

  lastMatrixFrame = millis();

  drawMatrixMode(audioLevel);

  matrixFrame++;

  if (beat || matrixFrame % 64 == 0) {
    matrixMode++;

    if (matrixMode > 5) {
      matrixMode = 0;
    }

    if (matrixMode == 3) {
      seedCellular();
    }
  }
}

void drawMatrixMode(byte audioLevel) {
  switch (matrixMode) {
    case 0:
      drawXorFractal(audioLevel);
      break;

    case 1:
      drawDiamondFractal(audioLevel);
      break;

    case 2:
      drawWaveInterference(audioLevel);
      break;

    case 3:
      drawCellularAutomata(audioLevel);
      break;

    case 4:
      drawTunnelSquares(audioLevel);
      break;

    case 5:
      drawSierpinskiLike(audioLevel);
      break;
  }
}

void clearMatrix() {
  lc.clearDisplay(0);
}

void setPixel(int x, int y, bool state) {
  if (x < 0 || x > 7 || y < 0 || y > 7) {
    return;
  }

  int px = x;
  int py = y;

  if (FLIP_X) {
    px = 7 - px;
  }

  if (FLIP_Y) {
    py = 7 - py;
  }

  if (SWAP_XY) {
    int temp = px;
    px = py;
    py = temp;
  }

  lc.setLed(0, py, px, state);
}

// -------------------------------------------------
// Frattale 1: XOR
// -------------------------------------------------

void drawXorFractal(byte audioLevel) {
  clearMatrix();

  byte t = matrixFrame + audioLevel;

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      byte v = (x + t) ^ (y + t);

      bool on = (v & 0b00000100) != 0;

      if (audioLevel > 5) {
        on = (v & 0b00000010) != 0;
      }

      if (audioLevel > 7) {
        on = (v & 0b00000001) != 0;
      }

      setPixel(x, y, on);
    }
  }
}

// -------------------------------------------------
// Frattale 2: diamante
// -------------------------------------------------

void drawDiamondFractal(byte audioLevel) {
  clearMatrix();

  int pulse = (matrixFrame + audioLevel) % 8;

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      int dx = abs(x - 3);
      int dy = abs(y - 3);

      int d = dx + dy;

      bool on = ((d + pulse) % 3) == 0;

      if (audioLevel > 6) {
        on = ((d + pulse) % 2) == 0;
      }

      setPixel(x, y, on);
    }
  }
}

// -------------------------------------------------
// Frattale 3: interferenza
// -------------------------------------------------

void drawWaveInterference(byte audioLevel) {
  clearMatrix();

  byte t = matrixFrame;

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      int d1 = abs(x - 1) + abs(y - 1);
      int d2 = abs(x - 6) + abs(y - 6);

      int wave = (d1 + d2 + t + audioLevel) % 5;

      bool on = wave == 0 || wave == 1;

      if (audioLevel > 6) {
        on = wave == 0 || wave == 1 || wave == 2;
      }

      setPixel(x, y, on);
    }
  }
}

// -------------------------------------------------
// Frattale 4: cellular automata
// -------------------------------------------------

void drawCellularAutomata(byte audioLevel) {
  for (int y = 0; y < 8; y++) {
    lc.setRow(0, y, cellularRows[y]);
  }

  stepCellular(audioLevel);
}

void seedCellular() {
  for (int y = 0; y < 8; y++) {
    cellularRows[y] = random(0, 256);
  }
}

void stepCellular(byte audioLevel) {
  byte nextRows[8];

  for (int y = 0; y < 8; y++) {
    byte newRow = 0;

    for (int x = 0; x < 8; x++) {
      int neighbors = countNeighbors(x, y);
      bool alive = getCell(x, y);

      bool nextAlive = false;

      if (alive && (neighbors == 2 || neighbors == 3)) {
        nextAlive = true;
      }

      if (!alive && neighbors == 3) {
        nextAlive = true;
      }

      // Audio alto = mutazioni più frequenti
      if (audioLevel > 6 && random(0, 14) == 0) {
        nextAlive = !nextAlive;
      }

      if (audioLevel > 7 && random(0, 9) == 0) {
        nextAlive = !nextAlive;
      }

      if (nextAlive) {
        newRow |= (1 << x);
      }
    }

    nextRows[y] = newRow;
  }

  for (int y = 0; y < 8; y++) {
    cellularRows[y] = nextRows[y];
  }
}

bool getCell(int x, int y) {
  if (x < 0 || x > 7 || y < 0 || y > 7) {
    return false;
  }

  return (cellularRows[y] & (1 << x)) != 0;
}

int countNeighbors(int x, int y) {
  int count = 0;

  for (int yy = -1; yy <= 1; yy++) {
    for (int xx = -1; xx <= 1; xx++) {
      if (xx == 0 && yy == 0) {
        continue;
      }

      if (getCell(x + xx, y + yy)) {
        count++;
      }
    }
  }

  return count;
}

// -------------------------------------------------
// Frattale 5: tunnel quadrati
// -------------------------------------------------

void drawTunnelSquares(byte audioLevel) {
  clearMatrix();

  int phase = (matrixFrame + audioLevel) % 4;

  drawSquare(phase);
  drawSquare((phase + 2) % 4);

  if (audioLevel > 4) {
    setPixel(3, 3, true);
    setPixel(4, 3, true);
    setPixel(3, 4, true);
    setPixel(4, 4, true);
  }

  if (audioLevel > 7) {
    setPixel(0, 0, true);
    setPixel(7, 0, true);
    setPixel(0, 7, true);
    setPixel(7, 7, true);
  }
}

void drawSquare(int sizeId) {
  int minPos = sizeId;
  int maxPos = 7 - sizeId;

  if (minPos > maxPos) {
    return;
  }

  for (int i = minPos; i <= maxPos; i++) {
    setPixel(i, minPos, true);
    setPixel(i, maxPos, true);
    setPixel(minPos, i, true);
    setPixel(maxPos, i, true);
  }
}

// -------------------------------------------------
// Frattale 6: Sierpinski-like
// -------------------------------------------------

void drawSierpinskiLike(byte audioLevel) {
  clearMatrix();

  int shift = (matrixFrame / 2) + audioLevel;

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      int xx = (x + shift) & 7;
      int yy = y;

      bool on = ((xx & yy) == 0);

      if (audioLevel > 5) {
        on = ((xx & yy) == 0) || (((xx + yy + shift) % 5) == 0);
      }

      if (audioLevel > 7) {
        on = ((xx & yy) == 0) || (((xx ^ yy ^ shift) & 1) == 0);
      }

      setPixel(x, y, on);
    }
  }
}