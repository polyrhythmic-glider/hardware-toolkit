#include <Wire.h>
#include "Adafruit_NeoKey_1x4.h"
#include "seesaw_neopixel.h"

#define NEOKEY_SDA 21
#define NEOKEY_SCL 19
#define NEOKEY_ADDR_A 0x30
#define NEOKEY_ADDR_B 0x31

Adafruit_NeoKey_1x4 keys[2] = {
  Adafruit_NeoKey_1x4(NEOKEY_ADDR_A),
  Adafruit_NeoKey_1x4(NEOKEY_ADDR_B),
};

bool ready[2] = { false, false };
uint8_t lastButtons[2] = { 0, 0 };

uint32_t color(uint8_t r, uint8_t g, uint8_t b) {
  return seesaw_NeoPixel::Color(r, g, b);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(NEOKEY_SDA, NEOKEY_SCL);
  Wire.setClock(400000);

  Serial.println("Dual NeoKey 1x4 test");
  Serial.println("SDA=D21/GPIO21, SCL=D19/GPIO19");

  for (int board = 0; board < 2; board++) {
    uint8_t address = board == 0 ? NEOKEY_ADDR_A : NEOKEY_ADDR_B;
    ready[board] = keys[board].begin(address);

    Serial.print("NeoKey ");
    Serial.print(board + 1);
    Serial.print(" address 0x");
    Serial.print(address, HEX);
    Serial.println(ready[board] ? " OK" : " MISSING");

    if (ready[board]) {
      keys[board].pixels.setBrightness(40);
      keys[board].pixels.clear();
      keys[board].pixels.show();
    }
  }
}

void loop() {
  for (int board = 0; board < 2; board++) {
    if (!ready[board]) continue;

    uint8_t buttons = keys[board].read() & 0x0F;

    if (buttons != lastButtons[board]) {
      Serial.print("NeoKey ");
      Serial.print(board + 1);
      Serial.print(" buttons: ");
      Serial.println(buttons, BIN);
      lastButtons[board] = buttons;
    }

    for (int i = 0; i < 4; i++) {
      bool pressed = (buttons & (1 << i)) != 0;
      uint32_t c = pressed
        ? (board == 0 ? color(0, 180, 80) : color(0, 80, 220))
        : color(0, 0, 0);
      keys[board].pixels.setPixelColor(i, c);
    }

    keys[board].pixels.show();
  }

  delay(10);
}
