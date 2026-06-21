#pragma once

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

int32_t encoderUiStepsTotal = 0;
uint32_t pushPressCount = 0;
uint32_t k0PressCount = 0;

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

  encoderUiStepsTotal += steps;

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
      pushPressCount++;
    }
  }

  if (k0Now != lastK0State && now - lastK0DebounceMs > debounceMs) {
    lastK0DebounceMs = now;
    lastK0State = k0Now;

    if (k0Now == LOW) {
      k0Event = true;
      k0PressCount++;
    }
  }
}
