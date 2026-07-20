  #include <Wire.h>
#include "Adafruit_NeoKey_1x4.h"

#define NEOKEY_SDA 21
#define NEOKEY_SCL 19
#define OLD_ADDR 0x30
#define NEW_ADDR 0x31

Adafruit_NeoKey_1x4 neokey;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(NEOKEY_SDA, NEOKEY_SCL);
  Wire.setClock(400000);

  Serial.println("NeoKey address changer");
  Serial.println("COLLEGA SOLO IL SECONDO NEOKEY.");
  Serial.println("Scrivo address 0x31 al posto di 0x30...");

  if (!neokey.begin(OLD_ADDR)) {
    Serial.println("ERRORE: non trovo NeoKey a 0x30.");
    Serial.println("Controlla SDA=D21, SCL=D19, 3V3 e GND.");
    return;
  }

  neokey.setI2CAddr(NEW_ADDR);
  delay(500);

  if (neokey.begin(NEW_ADDR)) {
    Serial.println("OK: il NeoKey ora risponde a 0x31.");
    Serial.println("SPEGNI e RIACCENDI, poi collega entrambi i NeoKey.");
  } else {
    Serial.println("ATTENZIONE: scrittura fatta, ma non lo rileggo a 0x31.");
    Serial.println("Spegni/riaccendi e prova lo scanner/test duale.");
  }
}

void loop() {
}
