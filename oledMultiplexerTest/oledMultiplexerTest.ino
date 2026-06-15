#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

constexpr int OLED_SDA = 21;      // ESP32 D21
constexpr int OLED_SCL = 22;      // ESP32 D22
constexpr int OLED_RESET = -1;

constexpr uint8_t PCA9548A_ADDRESS = 0x70;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t DISPLAY_COUNT = 4;

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void selectPcaChannel(uint8_t channel) {
  if (channel > 7) {
    return;
  }

  Wire.beginTransmission(PCA9548A_ADDRESS);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

bool i2cDeviceFound(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void scanSelectedChannel(uint8_t channel) {
  Serial.print("Canale PCA ");
  Serial.print(channel);
  Serial.println(":");

  bool foundAny = false;

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      foundAny = true;
      Serial.print("  trovato 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }

  if (!foundAny) {
    Serial.println("  nessun dispositivo trovato");
  }
}

void drawBars(uint8_t channel, unsigned long counter) {
  for (uint8_t i = 0; i < 8; i++) {
    int barHeight = ((counter * 7) + (i * 11) + (channel * 13)) % 34;
    int x = 8 + (i * 14);
    display.drawRect(x, 56 - barHeight, 9, barHeight, SSD1306_WHITE);
  }
}

void drawDiagonal(uint8_t channel, unsigned long counter) {
  int offset = (counter * 4 + channel * 9) % SCREEN_WIDTH;

  for (int i = -64; i < SCREEN_WIDTH; i += 16) {
    display.drawLine(i + offset, 18, i + offset + 46, 63, SSD1306_WHITE);
  }
}

void drawBouncingBox(uint8_t channel, unsigned long counter) {
  int x = (counter * 9 + channel * 17) % (SCREEN_WIDTH - 22);
  int y = 24 + ((counter * 5 + channel * 7) % 28);

  display.drawRect(x, y, 20, 10, SSD1306_WHITE);
  display.fillRect(x + 4, y + 3, 12, 4, SSD1306_WHITE);
}

void drawTarget(uint8_t channel, unsigned long counter) {
  int radius = 6 + ((counter + channel) % 4) * 4;

  display.drawCircle(64, 42, radius, SSD1306_WHITE);
  display.drawCircle(64, 42, radius + 7, SSD1306_WHITE);
  display.drawLine(64, 24, 64, 60, SSD1306_WHITE);
  display.drawLine(46, 42, 82, 42, SSD1306_WHITE);
}

void drawTestScreen(uint8_t channel, unsigned long counter) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PCA9548A CH ");
  display.println(channel);

  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print("OLED ");
  display.println(channel + 1);

  display.setTextSize(1);
  display.setCursor(86, 0);
  display.print("Tick: ");
  display.println(counter);

  switch (channel) {
    case 0:
      drawBars(channel, counter);
      break;
    case 1:
      drawDiagonal(channel, counter);
      break;
    case 2:
      drawBouncingBox(channel, counter);
      break;
    case 3:
      drawTarget(channel, counter);
      break;
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Test monolitico 4 OLED 0.96 con visualizzazioni diverse");
  Serial.println("SDA = D21, SCL = D22");

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  Serial.print("PCA9548A a 0x70: ");
  Serial.println(i2cDeviceFound(PCA9548A_ADDRESS) ? "OK" : "NON TROVATO");

  for (uint8_t channel = 0; channel < DISPLAY_COUNT; channel++) {
    selectPcaChannel(channel);
    delay(10);

    scanSelectedChannel(channel);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
      Serial.print("OLED NON inizializzato sul canale ");
      Serial.println(channel);
      continue;
    }

    Serial.print("OLED inizializzato sul canale ");
    Serial.println(channel);

    display.clearDisplay();
    display.display();
    drawTestScreen(channel, 0);
  }

  Serial.println("Setup completato. Inizio loop di test.");
}

void loop() {
  static unsigned long counter = 0;

  for (uint8_t channel = 0; channel < DISPLAY_COUNT; channel++) {
    selectPcaChannel(channel);
    delay(5);

    Serial.print("Aggiorno OLED ");
    Serial.print(channel + 1);
    Serial.print(" su canale PCA ");
    Serial.print(channel);
    Serial.print(" tick ");
    Serial.println(counter);

    drawTestScreen(channel, counter);
  }

  counter++;
  delay(1000);
}
