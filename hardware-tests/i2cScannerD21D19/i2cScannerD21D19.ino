#include <Wire.h>

#define I2C_SDA 21
#define I2C_SCL 19

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  Serial.println("I2C scanner on ESP32 D21 SDA, D19 SCL");
}

void loop() {
  int found = 0;

  Serial.println("Scanning...");

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      found++;
    }
  }

  if (found == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.print("Found ");
    Serial.print(found);
    Serial.println(" device(s)");
  }

  Serial.println();
  delay(3000);
}
