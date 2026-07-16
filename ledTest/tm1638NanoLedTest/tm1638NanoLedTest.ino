#include <TM1638plus.h>

// Test isolato per TM1638 LED&KEY con Arduino Nano a 5V.
// Collega:
// Nano D4  -> TM1638 STB
// Nano D6  -> TM1638 CLK
// Nano D7  -> TM1638 DIO
// Nano 5V  -> TM1638 VCC
// Nano GND -> TM1638 GND

#define TM_STB 4
#define TM_CLK 6
#define TM_DIO 7

TM1638plus tm(TM_STB, TM_CLK, TM_DIO, false);

void setOneLed(uint8_t index) {
  uint8_t mask = 1 << index;
  tm.setLEDs(mask);
}

void setup() {
  tm.displayBegin();
  tm.brightness(3);
  tm.displayText("NANOTEST");
  tm.setLEDs(0xFF);
  delay(1200);
  tm.setLEDs(0x00);
}

void loop() {
  for (uint8_t i = 0; i < 8; i++) {
    char text[9];
    snprintf(text, sizeof(text), "LED %d", i + 1);
    tm.displayText(text);
    setOneLed(i);
    delay(900);
  }

  tm.displayText("ALL ON");
  tm.setLEDs(0xFF);
  delay(1200);
  tm.setLEDs(0x00);
  delay(600);
}
