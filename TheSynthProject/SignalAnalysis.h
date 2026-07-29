#pragma once

float getOscillatorRms() {
  float level = volume / 100.0f;
  float sum = 0.0f;

  for (int i = 0; i < 48; i++) {
    float sample = getOscillatorSample((float)i / 48.0f) * level;
    sum += sample * sample;
  }

  return sqrtf(sum / 48.0f);
}

int getRmsDbFs() {
  float rms = getOscillatorRms();
  if (rms < 0.0001f) return -80;

  int db = (int)(20.0f * log10f(rms));
  return clampInt(db, -80, 0);
}

void getPitchLabel(char* buffer, size_t size) {
  const char* noteNames[] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
  };

  float midi = 69.0f + 12.0f * (logf(frequency / 440.0f) / logf(2.0f));
  int nearest = (int)roundf(midi);
  int note = nearest % 12;
  if (note < 0) note += 12;

  int octave = nearest / 12 - 1;
  int cents = (int)roundf((midi - nearest) * 100.0f);

  snprintf(buffer, size, "%s%d %+dc", noteNames[note], octave, cents);
}

int getWavePreviewY(int step, int width, int y, int height) {
  if (width <= 1) return y + height / 2;

  float phase = (float)step / (float)(width - 1);
  int travel = height - 4;
  float value = getOscillatorSample(phase);

  return y + height / 2 - (int)(value * (travel / 2));
}

void drawWavePreview(int x, int y, int width, int height, uint16_t color) {
  int prevY = getWavePreviewY(0, width, y, height);

  for (int i = 1; i < width; i++) {
    int nextY = getWavePreviewY(i, width, y, height);
    tft.drawLine(x + i - 1, prevY, x + i, nextY, color);
    prevY = nextY;
  }
}

int getWaveShapePreviewYForParams(int shape, int step, int width, int y, int height, float foldAmount, float phaseDegrees);
void drawWaveShapePreviewForParams(int x, int y, int width, int height, int shape, float foldAmount, float phaseDegrees, uint16_t color);

int getWaveShapePreviewY(int shape, int step, int width, int y, int height) {
  return getWaveShapePreviewYForParams(shape, step, width, y, height, (float)waveFold, (float)wavePhase);
}

int getWaveShapePreviewYForParams(int shape, int step, int width, int y, int height, float foldAmount, float phaseDegrees) {
  if (width <= 1) return y + height / 2;

  float phase = (float)step / (float)(width - 1);
  int travel = height - 4;
  float value = getOscillatorSampleForParams(phase, shape, foldAmount, phaseDegrees);

  return y + height / 2 - (int)(value * (travel / 2));
}

void drawWaveShapePreview(int x, int y, int width, int height, int shape, uint16_t color) {
  drawWaveShapePreviewForParams(x, y, width, height, shape, (float)waveFold, (float)wavePhase, color);
}

void drawWaveShapePreviewForParams(int x, int y, int width, int height, int shape, float foldAmount, float phaseDegrees, uint16_t color) {
  int prevY = getWaveShapePreviewYForParams(shape, 0, width, y, height, foldAmount, phaseDegrees);

  for (int i = 1; i < width; i++) {
    int nextY = getWaveShapePreviewYForParams(shape, i, width, y, height, foldAmount, phaseDegrees);
    tft.drawLine(x + i - 1, prevY, x + i, nextY, color);
    prevY = nextY;
  }
}
