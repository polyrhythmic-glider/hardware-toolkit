#ifndef PONG_H
#define PONG_H

#include <Arduino.h>

extern float ballX;
extern float ballY;
extern float ballVX;
extern float ballVY;




extern int leftPaddleY;
extern int rightPaddleY;

extern int pongScoreLeft;
extern int pongScoreRight;

extern long pongStartValue1;
extern long pongStartValue2;

extern unsigned long lastPongUpdate;
extern unsigned long pongServeDelayUntil;

extern bool pongGameOver;

void resetPongGame();
void resetPongBall(int direction);
void updatePong();
void checkPongPaddles();
void handlePongInputLogic();

#endif
