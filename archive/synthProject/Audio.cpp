#include "Audio.h"

#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

#include "AppConfig.h"

static float audioPhase = 0.0f;
static bool audioReady = false;
static TaskHandle_t audioTaskHandle = nullptr;

static int16_t audioBuffer[AUDIO_BUFFER_SAMPLES * 2];

static float audioMapFloat(float value, float inMin, float inMax, float outMin, float outMax) {
  return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

static float audioConstrainFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }

  if (value > maxValue) {
    return maxValue;
  }

  return value;
}

static float audioApplyCurve(float t, float curve) {
  t = audioConstrainFloat(t, 0.0f, 1.0f);

  if (curve >= 1.0f) {
    return powf(t, curve);
  }

  return 1.0f - powf(1.0f - t, 1.0f / curve);
}

static float audioSmoothStep(float edge0, float edge1, float x) {
  float t = audioConstrainFloat((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

static float audioSmoothedPulse(float phase, float duty, float edgeWidth) {
  float rise = audioSmoothStep(0.0f, edgeWidth, phase);
  float fall = 1.0f - audioSmoothStep(duty, duty + edgeWidth, phase);
  float highAmount = min(rise, fall);

  if (phase > duty + edgeWidth) {
    highAmount = 0.0f;
  }

  return highAmount * 2.0f - 1.0f;
}

static float renderAudioSample(float phase) {
  switch (waveformIndex) {
    case WAVE_SINE: {
      float drive = audioMapFloat(parameterA, -50, 50, 0.55f, 3.20f);
      float warp = audioMapFloat(parameterB, -50, 50, -1.25f, 1.25f);
      float angle = phase * TWO_PI;
      float warpedAngle = angle + sinf(angle * 2.0f) * warp;
      float value = sinf(warpedAngle);

      if (parameterA >= 0) {
        value = tanhf(value * drive);
      } else {
        value *= drive;
      }

      return value;
    }

    case WAVE_TRI: {
      float skew = audioMapFloat(parameterA, -50, 50, 0.12f, 0.88f);
      float curve = audioMapFloat(parameterB, -50, 50, 0.35f, 2.80f);
      float value;

      if (phase < skew) {
        float t = audioApplyCurve(phase / skew, curve);
        value = -1.0f + t * 2.0f;
      } else {
        float t = audioApplyCurve((phase - skew) / (1.0f - skew), curve);
        value = 1.0f - t * 2.0f;
      }

      return value;
    }

    case WAVE_STEP: {
      float duty = audioMapFloat(parameterA, -50, 50, 0.05f, 0.95f);
      float edgeWidth = audioMapFloat(parameterB, -50, 50, 0.001f, 0.18f);

      if (parameterB <= -45) {
        return phase < duty ? 1.0f : -1.0f;
      }

      return audioSmoothedPulse(phase, duty, edgeWidth);
    }

    case WAVE_CONST: {
      float level = audioMapFloat(parameterA, -50, 50, -0.85f, 0.85f);
      float ripple = audioMapFloat(abs(parameterB), 0, 50, 0.0f, 0.45f);
      float rippleDirection = parameterB < 0 ? -1.0f : 1.0f;
      return audioConstrainFloat(level + sinf(phase * TWO_PI) * ripple * rippleDirection, -1.0f, 1.0f);
    }
  }

  return 0.0f;
}

static void fillAudioBuffer(bool active) {
  float phaseStep = AUDIO_BASE_FREQUENCY / (float)AUDIO_SAMPLE_RATE;

  for (int i = 0; i < AUDIO_BUFFER_SAMPLES; i++) {
    int16_t pcm = 0;

    if (active) {
      float sample = renderAudioSample(audioPhase) * 0.28f;
      pcm = (int16_t)(audioConstrainFloat(sample, -1.0f, 1.0f) * 32767.0f);

      audioPhase += phaseStep;

      if (audioPhase >= 1.0f) {
        audioPhase -= 1.0f;
      }
    }

    audioBuffer[i * 2] = pcm;
    audioBuffer[i * 2 + 1] = pcm;
  }
}

static void audioTask(void* parameter) {
  (void)parameter;

  while (true) {
    bool active = currentMode == MODE_WAVEFORM && !menuOpen;

    fillAudioBuffer(active);

    size_t bytesWritten = 0;
    i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytesWritten, portMAX_DELAY);
  }
}

void setupAudio() {
  i2s_config_t config = {};
  config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = AUDIO_SAMPLE_RATE;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
#ifdef I2S_COMM_FORMAT_STAND_I2S
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
#else
  config.communication_format = I2S_COMM_FORMAT_I2S;
#endif
  config.intr_alloc_flags = 0;
  config.dma_buf_count = 8;
  config.dma_buf_len = AUDIO_BUFFER_SAMPLES;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;
  config.fixed_mclk = 0;

  i2s_pin_config_t pins = {};
  pins.bck_io_num = AUDIO_I2S_BCK;
  pins.ws_io_num = AUDIO_I2S_LRCK;
  pins.data_out_num = AUDIO_I2S_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) {
    audioReady = false;
    return;
  }

  if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) {
    i2s_driver_uninstall(I2S_NUM_0);
    audioReady = false;
    return;
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
  audioReady = true;

  if (xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 1, &audioTaskHandle, 0) != pdPASS) {
    i2s_driver_uninstall(I2S_NUM_0);
    audioReady = false;
    audioTaskHandle = nullptr;
  }
}

void updateAudio() {
  // Audio is fed by a small FreeRTOS task so OLED redraws do not starve I2S.
}

void resetAudioPhase() {
  audioPhase = 0.0f;
}
