#include <Arduino.h>
#include <Wire.h>

// Secondo ESP32 dedicato all'esperimento.
// Nomi fisici usati nel cablaggio: D21 = SDA, D22 = SCL.
constexpr uint8_t TEST_SDA = 21;
constexpr uint8_t TEST_SCL = 22;
constexpr uint32_t SERIAL_SPEED = 115200;
constexpr uint32_t I2C_SPEED = 100000;

constexpr uint8_t MCP_ADDRESS = 0x20;
constexpr uint8_t MCP_IODIRA = 0x00;
constexpr uint8_t MCP_IOCON = 0x0A;
constexpr uint8_t MCP_GPPUA = 0x0C;
constexpr uint8_t MCP_GPIOA = 0x12;

// Encoder singolo sulle porte A del MCP23017.
constexpr uint8_t ENCODER_CLK_BIT = 0;  // A0 della riga B0/A0
constexpr uint8_t ENCODER_DT_BIT = 1;   // A1 della riga B1/A1
constexpr uint8_t ENCODER_SW_BIT = 2;   // A2 della riga B2/A2

constexpr uint32_t POLL_INTERVAL_US = 1000;
constexpr uint32_t RETRY_INTERVAL_MS = 2000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr int8_t STEPS_PER_DETENT = 4;

const int8_t QUADRATURE_TABLE[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

bool mcpReady = false;
uint16_t lastPorts = 0xFFFF;
uint8_t lastAb = 0;
int8_t quadratureAccumulator = 0;
long encoderCount = 0;
uint8_t lastRawSwitch = HIGH;
uint8_t stableSwitch = HIGH;
uint32_t switchChangedAt = 0;
uint32_t lastRetryAt = 0;
uint32_t lastPollAtUs = 0;
uint32_t lastStatusAt = 0;
uint32_t successfulReads = 0;
uint32_t readErrors = 0;
uint8_t consecutiveErrors = 0;

uint8_t portBit(uint16_t ports, uint8_t bitIndex) {
  return (ports >> bitIndex) & 0x01;
}

bool devicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission(true) == 0;
}

void printHexByte(uint8_t value) {
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

void printHexWord(uint16_t value) {
  if (value < 0x1000) Serial.print('0');
  if (value < 0x0100) Serial.print('0');
  if (value < 0x0010) Serial.print('0');
  Serial.print(value, HEX);
}

void scanBus() {
  Serial.println();
  Serial.println("=== SCAN D21/D22 ===");
  Serial.print("D21 SDA idle: ");
  Serial.println(digitalRead(TEST_SDA) == HIGH ? "HIGH" : "LOW - CHECK SHORT");
  Serial.print("D22 SCL idle: ");
  Serial.println(digitalRead(TEST_SCL) == HIGH ? "HIGH" : "LOW - CHECK SHORT");
  Serial.print("Devices:");

  uint8_t count = 0;
  for (uint8_t address = 1; address < 0x7F; address++) {
    if (!devicePresent(address)) continue;
    Serial.print(" 0x");
    printHexByte(address);
    count++;
  }

  if (count == 0) Serial.print(" none");
  Serial.println();
  Serial.println("Expected with A0/A1/A2 control at GND: 0x20");
  Serial.println("====================");
}

bool writeRegister(uint8_t registerAddress, uint8_t value) {
  Wire.beginTransmission(MCP_ADDRESS);
  Wire.write(registerAddress);
  Wire.write(value);
  return Wire.endTransmission(true) == 0;
}

bool writeRegisterPair(uint8_t firstRegister, uint8_t valueA, uint8_t valueB) {
  Wire.beginTransmission(MCP_ADDRESS);
  Wire.write(firstRegister);
  Wire.write(valueA);
  Wire.write(valueB);
  return Wire.endTransmission(true) == 0;
}

bool readPorts(uint16_t& ports) {
  Wire.beginTransmission(MCP_ADDRESS);
  Wire.write(MCP_GPIOA);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom(MCP_ADDRESS, (uint8_t)2, (uint8_t)true) != 2) return false;
  uint8_t portA = Wire.read();
  uint8_t portB = Wire.read();
  ports = (uint16_t)portA | ((uint16_t)portB << 8);
  return true;
}

void printEncoderState(uint16_t ports, const char* reason) {
  Serial.print(reason);
  Serial.print(" RAW:0x");
  printHexWord(ports);
  Serial.print("  CLK:");
  Serial.print(portBit(ports, ENCODER_CLK_BIT) ? "HIGH" : "LOW");
  Serial.print("  DT:");
  Serial.print(portBit(ports, ENCODER_DT_BIT) ? "HIGH" : "LOW");
  Serial.print("  SW:");
  Serial.print(portBit(ports, ENCODER_SW_BIT) ? "HIGH" : "LOW");
  Serial.print("  COUNT:");
  Serial.println(encoderCount);
}

bool initializeMcp() {
  if (!devicePresent(MCP_ADDRESS)) return false;

  // BANK=0 e registri sequenziali, poi tutte le porte come ingressi con pull-up.
  if (!writeRegister(MCP_IOCON, 0x00)) return false;
  if (!writeRegister(MCP_IOCON + 1, 0x00)) return false;
  if (!writeRegisterPair(MCP_IODIRA, 0xFF, 0xFF)) return false;
  if (!writeRegisterPair(MCP_GPPUA, 0xFF, 0xFF)) return false;

  uint16_t ports = 0xFFFF;
  if (!readPorts(ports)) return false;

  lastPorts = ports;
  lastAb = (portBit(ports, ENCODER_CLK_BIT) << 1) |
           portBit(ports, ENCODER_DT_BIT);
  lastRawSwitch = portBit(ports, ENCODER_SW_BIT);
  stableSwitch = lastRawSwitch;
  switchChangedAt = millis();
  consecutiveErrors = 0;
  mcpReady = true;

  Serial.println();
  Serial.println("MCP23017 0x20: OK");
  printEncoderState(ports, "INITIAL");
  Serial.println("Turn the encoder and press SW.");
  return true;
}

void tryConnectMcp() {
  scanBus();
  if (initializeMcp()) return;

  mcpReady = false;
  Serial.println("MCP23017 0x20: MISSING");
  Serial.println("Check control-side VCC/GND/RESET/A0-A2 and D21/D22.");
  Serial.println("The test retries automatically every 2 seconds.");
}

void processEncoder(uint16_t ports) {
  uint8_t clk = portBit(ports, ENCODER_CLK_BIT);
  uint8_t dt = portBit(ports, ENCODER_DT_BIT);
  uint8_t newAb = (clk << 1) | dt;
  int8_t movement = QUADRATURE_TABLE[(lastAb << 2) | newAb];
  lastAb = newAb;

  if (movement != 0) {
    quadratureAccumulator += movement;
    if (quadratureAccumulator >= STEPS_PER_DETENT) {
      quadratureAccumulator = 0;
      encoderCount++;
      printEncoderState(ports, "TURN   ");
    } else if (quadratureAccumulator <= -STEPS_PER_DETENT) {
      quadratureAccumulator = 0;
      encoderCount--;
      printEncoderState(ports, "TURN   ");
    }
  }

  uint8_t rawSwitch = portBit(ports, ENCODER_SW_BIT);
  uint32_t now = millis();
  if (rawSwitch != lastRawSwitch) {
    lastRawSwitch = rawSwitch;
    switchChangedAt = now;
  }
  if (rawSwitch != stableSwitch && now - switchChangedAt >= BUTTON_DEBOUNCE_MS) {
    stableSwitch = rawSwitch;
    printEncoderState(ports, stableSwitch == LOW ? "SW DOWN" : "SW UP  ");
  }

  if (ports != lastPorts) {
    // Mostra anche le transizioni elettriche CLK/DT troppo brevi per produrre
    // subito un intero scatto dell'encoder.
    printEncoderState(ports, "CHANGE ");
    lastPorts = ports;
  }
}

void setup() {
  Serial.begin(SERIAL_SPEED);
  delay(500);

  Serial.println();
  Serial.println("MCP23017 + ONE ENCODER ISOLATED TEST");
  Serial.println("ESP32: D21=SDA, D22=SCL, 3V3 and GND");
  Serial.println("Encoder pin order: CLK, DT, SW, +, GND");

  Wire.begin(TEST_SDA, TEST_SCL, I2C_SPEED);
  Wire.setClock(I2C_SPEED);
  tryConnectMcp();
}

void loop() {
  if (!mcpReady) {
    if (millis() - lastRetryAt >= RETRY_INTERVAL_MS) {
      lastRetryAt = millis();
      tryConnectMcp();
    }
    delay(10);
    return;
  }

  uint32_t nowUs = micros();
  if ((uint32_t)(nowUs - lastPollAtUs) < POLL_INTERVAL_US) return;
  lastPollAtUs = nowUs;

  uint16_t ports = 0xFFFF;
  if (readPorts(ports)) {
    successfulReads++;
    consecutiveErrors = 0;
    processEncoder(ports);

    if (millis() - lastStatusAt >= 2000) {
      lastStatusAt = millis();
      Serial.print("STATUS MCP:OK READS:");
      Serial.print(successfulReads);
      Serial.print(" ERRORS:");
      Serial.print(readErrors);
      Serial.print(" RAW:0x");
      printHexWord(ports);
      Serial.print(" COUNT:");
      Serial.println(encoderCount);
    }
  } else {
    readErrors++;
    consecutiveErrors++;
    if (consecutiveErrors >= 10) {
      Serial.print("MCP LOST - reads:");
      Serial.print(successfulReads);
      Serial.print(" errors:");
      Serial.println(readErrors);
      mcpReady = false;
      lastRetryAt = 0;
    }
  }
}
