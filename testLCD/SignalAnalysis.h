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

float getHarmonicAmplitude(int harmonic) {
  if (harmonic <= 0) return 0.0f;
  float amplitude = 0.0f;

  switch (waveform) {
    case 0:
      amplitude = harmonic == 1 ? 1.0f : 0.0f;
      break;

    case 1:
      amplitude = (harmonic % 2) == 1 ? 1.0f / harmonic : 0.0f;
      break;

    case 2:
      amplitude = (harmonic % 2) == 1 ? 1.0f / ((float)harmonic * harmonic) : 0.0f;
      break;

    case 3:
    case 4:
      amplitude = 1.0f / harmonic;
      break;

    case 5:
    case 11:
      amplitude = fabsf(sinf(3.1415926f * 0.28f * harmonic)) / harmonic;
      break;

    case 6:
      amplitude = harmonic == 1 ? 0.65f : 0.0f;
      break;

    case 7:
      if (harmonic == 1) amplitude = 0.58f;
      else if (harmonic == 2) amplitude = 0.28f;
      else if (harmonic == 3) amplitude = 0.14f;
      break;

    case 8:
      if (harmonic == 1) amplitude = 0.55f;
      else if (harmonic == 2) amplitude = 0.45f;
      break;

    case 9:
      if (harmonic == 1) amplitude = 0.72f;
      else if (harmonic == 3) amplitude = 0.20f;
      else if (harmonic == 5) amplitude = 0.08f;
      break;

    case 10:
      amplitude = 0.85f / harmonic;
      break;

    case 12:
      amplitude = harmonic <= 8 ? 1.0f / (harmonic * 0.85f) : 0.0f;
      break;

    case 13:
      amplitude = (harmonic % 2) == 1 ? 1.0f / sqrtf((float)harmonic) : 0.12f / harmonic;
      break;

    case 14:
      amplitude = 0.16f + 0.18f / sqrtf((float)harmonic);
      break;

    case 15:
      if (harmonic == 1) amplitude = 0.52f;
      else if (harmonic == 3) amplitude = 0.30f;
      else if (harmonic == 7) amplitude = 0.18f;
      break;
  }

  if (waveFold > 0 && harmonic > 1) {
    float foldEnergy = (waveFold / 100.0f) * (1.0f / (harmonic * 1.35f));
    amplitude += foldEnergy;
  }

  return amplitude;
}

int getSpectrumBarCount() {
  return FFT_BIN_VALUES[fftBinsIndex];
}

float getSpectrumMaxHz() {
  return (float)FFT_RANGE_VALUES[fftRangeIndex];
}

const char* getSpectrumMaxLabel() {
  return FFT_RANGE_LABELS[fftRangeIndex];
}

float getWindowLeakage() {
  switch (fftWindow) {
    case 1:
      return 0.05f;

    case 2:
      return 0.20f;

    case 3:
      return 0.30f;
  }

  return 0.12f;
}

float getSpectrumPosition(float hz) {
  float safeHz = hz < SPECTRUM_MIN_HZ ? SPECTRUM_MIN_HZ : hz;
  float maxHz = getSpectrumMaxHz();

  if (safeHz > maxHz) safeHz = maxHz;

  float range = logf(maxHz / SPECTRUM_MIN_HZ);
  float position = logf(safeHz / SPECTRUM_MIN_HZ) / range;
  int bins = getSpectrumBarCount();

  return position * (bins - 1);
}

void buildOscillatorSpectrum(float magnitudes[], bool normalizeOutput) {
  int bins = getSpectrumBarCount();

  for (int i = 0; i < bins; i++) {
    magnitudes[i] = 0.0f;
  }

  float level = normalizeOutput ? 1.0f : volume / 100.0f;
  float leakage = getWindowLeakage();
  float maxHz = getSpectrumMaxHz();

  for (int harmonic = 1; harmonic <= 96; harmonic++) {
    float hz = frequency * harmonic;
    if (hz > maxHz) break;

    float amplitude = getHarmonicAmplitude(harmonic) * level;
    if (amplitude <= 0.0f) continue;

    float position = getSpectrumPosition(hz);
    int left = (int)floorf(position);
    float blend = position - left;

    if (left >= 0 && left < bins) {
      magnitudes[left] += amplitude * (1.0f - blend);
    }

    if (left + 1 >= 0 && left + 1 < bins) {
      magnitudes[left + 1] += amplitude * blend;
    }

    if (left - 1 >= 0 && left - 1 < bins) {
      magnitudes[left - 1] += amplitude * leakage * 0.55f;
    }

    if (left + 2 >= 0 && left + 2 < bins) {
      magnitudes[left + 2] += amplitude * leakage * 0.35f;
    }
  }

  int smoothPasses = fftSmoothing / 35;

  for (int pass = 0; pass < smoothPasses; pass++) {
    float smoothed[SPECTRUM_MAX_BARS];

    for (int i = 0; i < bins; i++) {
      float left = i > 0 ? magnitudes[i - 1] : magnitudes[i];
      float right = i + 1 < bins ? magnitudes[i + 1] : magnitudes[i];
      smoothed[i] = (left + magnitudes[i] * 2.0f + right) * 0.25f;
    }

    for (int i = 0; i < bins; i++) {
      magnitudes[i] = smoothed[i];
    }
  }

  float maxMagnitude = normalizeOutput ? 0.0f : 1.0f;

  if (normalizeOutput) {
    for (int i = 0; i < bins; i++) {
      if (magnitudes[i] > maxMagnitude) maxMagnitude = magnitudes[i];
    }

    if (maxMagnitude < 0.001f) maxMagnitude = 1.0f;
  }

  for (int i = 0; i < bins; i++) {
    magnitudes[i] = clampInt((int)((magnitudes[i] / maxMagnitude) * 100.0f), 0, 100) / 100.0f;
  }
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
