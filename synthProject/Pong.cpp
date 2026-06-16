#include "Pong.h"

#include <math.h>

#include "AppConfig.h"
#include "Hardware.h"

float ballX = PONG_WORLD_WIDTH / 2.0f;
float ballY = 32.0f;
float ballVX = PONG_BASE_X_SPEED;
float ballVY = PONG_BASE_Y_SPEED;

int leftPaddleY = 40;
int rightPaddleY = 40;

int pongScoreLeft = 0;
int pongScoreRight = 0;

long pongStartValue1 = 0;
long pongStartValue2 = 0;

unsigned long lastPongUpdate = 0;
unsigned long pongServeDelayUntil = 0;

bool pongGameOver = false;

static float constrainPongFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }

  if (value > maxValue) {
    return maxValue;
  }

  return value;
}

void resetPongGame() {
  pongScoreLeft = 0;
  pongScoreRight = 0;
  leftPaddleY = 40;
  rightPaddleY = 40;
  pongGameOver = false;
  resetPongBall(1);
  lastPongUpdate = millis();
}

void resetPongBall(int direction) {
  ballX = PONG_WORLD_WIDTH / 2.0f;
  ballY = 40.0f;

  if (direction >= 0) {
    ballVX = PONG_BASE_X_SPEED;
  } else {
    ballVX = -PONG_BASE_X_SPEED;
  }

  ballVY = ((pongScoreLeft + pongScoreRight) % 2 == 0) ? PONG_BASE_Y_SPEED : -PONG_BASE_Y_SPEED;
  pongServeDelayUntil = millis() + 450;
}

void updatePong() {
  unsigned long now = millis();

  if (now - lastPongUpdate < 20) {
    return;
  }

  lastPongUpdate = now;

  if (pongGameOver) {
    valuesChanged = true;
    return;
  }

  if (now < pongServeDelayUntil) {
    valuesChanged = true;
    return;
  }

  ballX += ballVX;
  ballY += ballVY;

  if (ballY <= PONG_TOP + 2) {
    ballY = PONG_TOP + 2;
    ballVY = fabs(ballVY);
  }

  if (ballY >= PONG_BOTTOM - 2) {
    ballY = PONG_BOTTOM - 2;
    ballVY = -fabs(ballVY);
  }

  checkPongPaddles();

  if (ballX < 0) {
    pongScoreRight++;

    if (pongScoreRight >= 5) {
      pongGameOver = true;
    } else {
      resetPongBall(1);
    }
  }

  if (ballX > PONG_WORLD_WIDTH - 1) {
    pongScoreLeft++;

    if (pongScoreLeft >= 5) {
      pongGameOver = true;
    } else {
      resetPongBall(-1);
    }
  }

  ballVX = constrainPongFloat(ballVX, -PONG_MAX_X_SPEED, PONG_MAX_X_SPEED);
  ballVY = constrainPongFloat(ballVY, -PONG_MAX_Y_SPEED, PONG_MAX_Y_SPEED);

  valuesChanged = true;
}

void checkPongPaddles() {
  float collisionReach = fabs(ballVX);

  if (collisionReach < 4.0f) {
    collisionReach = 4.0f;
  }

  if (ballVX < 0 && ballX <= PONG_LEFT_X + collisionReach && ballX >= PONG_LEFT_X - collisionReach) {
    if (ballY >= leftPaddleY - PONG_PADDLE_HALF && ballY <= leftPaddleY + PONG_PADDLE_HALF) {
      float hit = (ballY - leftPaddleY) / (float)PONG_PADDLE_HALF;
      ballX = PONG_LEFT_X + 4;
      ballVX = fabs(ballVX) + (0.10f * PONG_SPEED_MULTIPLIER);
      ballVY = hit * 1.85f * PONG_SPEED_MULTIPLIER;
    }
  }

  if (ballVX > 0 && ballX >= PONG_RIGHT_X - collisionReach && ballX <= PONG_RIGHT_X + collisionReach) {
    if (ballY >= rightPaddleY - PONG_PADDLE_HALF && ballY <= rightPaddleY + PONG_PADDLE_HALF) {
      float hit = (ballY - rightPaddleY) / (float)PONG_PADDLE_HALF;
      ballX = PONG_RIGHT_X - 4;
      ballVX = -fabs(ballVX) - (0.10f * PONG_SPEED_MULTIPLIER);
      ballVY = hit * 1.85f * PONG_SPEED_MULTIPLIER;
    }
  }
}

void handlePongInputLogic() {
  long v1;
  long v4;

  noInterrupts();
  v1 = enc1.value;
  v4 = enc4.value;
  interrupts();

  int delta1 = v1 - pongStartValue1;
  int delta4 = v4 - pongStartValue2;

  if (delta1 != 0) {
    leftPaddleY += delta1 * 2;
    leftPaddleY = constrain(leftPaddleY, PONG_TOP + PONG_PADDLE_HALF, PONG_BOTTOM - PONG_PADDLE_HALF);
    pongStartValue1 = v1;
    valuesChanged = true;
  }

  if (delta4 != 0) {
    rightPaddleY += delta4 * 2;
    rightPaddleY = constrain(rightPaddleY, PONG_TOP + PONG_PADDLE_HALF, PONG_BOTTOM - PONG_PADDLE_HALF);
    pongStartValue2 = v4;
    valuesChanged = true;
  }

  if (pongGameOver && (enc1.clickEvent || enc4.clickEvent)) {
    resetPongGame();
    valuesChanged = true;
  }

  if (enc1.clickEvent) {
    clickFlashUntil1 = millis() + 120;
    valuesChanged = true;
  }

  if (enc4.clickEvent) {
    clickFlashUntil4 = millis() + 120;
    valuesChanged = true;
  }
}
