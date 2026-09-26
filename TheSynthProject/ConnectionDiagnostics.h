#pragma once

// Riepilogo unico delle periferiche viste dalla ESP32. Questo header viene
// incluso dopo DaisyBridge, cosi la pagina TFT e il report seriale usano gli
// stessi dati e mostrano separatamente le due direzioni del link UART.

constexpr uint8_t CONNECTION_NEOKEY_BOARD_COUNT = 4;
constexpr uint8_t CONNECTION_NEOKEY_BASE_ADDRESS = 0x30;

uint8_t countReadyBits(uint8_t value, uint8_t bitCount = 4) {
  uint8_t count = 0;
  for (uint8_t bit = 0; bit < bitCount; bit++) {
    if ((value & (uint8_t)(1U << bit)) != 0U) count++;
  }
  return count;
}

uint8_t getReadyNeoKeyCount() {
  uint8_t count = 0;
  for (uint8_t board = 0; board < CONNECTION_NEOKEY_BOARD_COUNT; board++) {
    if (neokeyBoardReady[board]) count++;
  }
  return count;
}

bool isDaisyForwardLinkReady() {
  return DaisyBridge::isConnected();
}

bool isDaisyReturnLinkReady() {
  return DaisyBridge::isReturnLinkActive();
}

uint32_t getConnectionStatusHash() {
  uint32_t hash = 0;
  hash |= ControlSurface::isPcaReady() ? (1UL << 0) : 0;
  hash |= ((uint32_t)ControlSurface::getOledDetectedMask() & 0x0FUL) << 1;
  hash |= ((uint32_t)ControlSurface::getOledReadyMask() & 0x0FUL) << 5;
  hash |= ((uint32_t)getReadyNeoKeyCount() & 0x07UL) << 9;
  hash |= ControlSurface::isEncoderExpanderReady() ? (1UL << 12) : 0;
  hash |= ControlSurface::isMcpRawPortsValid() ? (1UL << 13) : 0;
  hash |= isDaisyForwardLinkReady() ? (1UL << 14) : 0;
  hash |= isDaisyReturnLinkReady() ? (1UL << 15) : 0;
  return hash;
}

uint8_t getConnectionProblemCount() {
  uint8_t problems = 0;
  if (!ControlSurface::isPcaReady()) problems++;
  problems += 4 - countReadyBits(ControlSurface::getOledReadyMask());
  problems += CONNECTION_NEOKEY_BOARD_COUNT - getReadyNeoKeyCount();
  if (!ControlSurface::isEncoderExpanderReady()) problems++;
  if (!isDaisyForwardLinkReady()) problems++;
  if (!isDaisyReturnLinkReady()) problems++;
  return problems;
}

constexpr size_t STARTUP_MAP_WIDTH = 68;

void printStartupMapBorder(char fill = '-') {
  Serial.print('+');
  for (size_t column = 0; column < STARTUP_MAP_WIDTH + 2; column++) {
    Serial.print(fill);
  }
  Serial.println('+');
}

void printStartupMapLine(const char* text = "") {
  Serial.print("| ");
  size_t column = 0;
  while (column < STARTUP_MAP_WIDTH && text[column] != '\0') {
    Serial.print(text[column]);
    column++;
  }
  while (column < STARTUP_MAP_WIDTH) {
    Serial.print(' ');
    column++;
  }
  Serial.println(" |");
}

void printStartupMapCentered(const char* text) {
  size_t textLength = strlen(text);
  if (textLength > STARTUP_MAP_WIDTH) textLength = STARTUP_MAP_WIDTH;
  size_t leftPadding = (STARTUP_MAP_WIDTH - textLength) / 2;

  Serial.print("| ");
  for (size_t column = 0; column < leftPadding; column++) Serial.print(' ');
  for (size_t column = 0; column < textLength; column++) Serial.print(text[column]);
  for (size_t column = leftPadding + textLength; column < STARTUP_MAP_WIDTH; column++) {
    Serial.print(' ');
  }
  Serial.println(" |");
}

void printStartupMapStatus(const char* label, const char* status) {
  char line[STARTUP_MAP_WIDTH + 1];
  memset(line, ' ', STARTUP_MAP_WIDTH);
  line[STARTUP_MAP_WIDTH] = '\0';

  size_t statusLength = strlen(status);
  if (statusLength > STARTUP_MAP_WIDTH - 3) statusLength = STARTUP_MAP_WIDTH - 3;
  size_t statusStart = STARTUP_MAP_WIDTH - statusLength - 2;
  size_t labelLength = strlen(label);
  if (labelLength >= statusStart) labelLength = statusStart - 1;

  memcpy(line, label, labelLength);
  line[statusStart] = '[';
  memcpy(line + statusStart + 1, status, statusLength);
  line[STARTUP_MAP_WIDTH - 1] = ']';
  printStartupMapLine(line);
}

void printStartupConnectionMap() {
  const uint8_t oledReadyMask = ControlSurface::getOledReadyMask();
  const bool daisyForwardReady = isDaisyForwardLinkReady();
  const bool daisyReturnReady = isDaisyReturnLinkReady();
  char countStatus[12];
  char systemStatus[12];
  char bootLine[STARTUP_MAP_WIDTH + 1];

  Serial.println();
  printStartupMapBorder('=');
  printStartupMapCentered("POLYRHYTHMIC GLIDER");
  printStartupMapCentered("ESP32 + DAISY STARTUP MAP");
  printStartupMapBorder('=');

  snprintf(bootLine, sizeof(bootLine), "BOOT: %-12s | SERIAL: 115200 | LIVE MONITOR: %s",
           getResetReasonText(esp_reset_reason()),
           isSerialDiagnosticsEnabled() ? "ON" : "OFF");
  printStartupMapLine(bootLine);
  printStartupMapBorder('-');

  printStartupMapLine("ESP32 DevKit");
  printStartupMapStatus("+-- SPI -> TFT ST7735  D5/D4/D2/D18/D23", "OK");
  printStartupMapStatus("+-- I2C D21/D22 -> PCA9548A 0x70",
                        ControlSurface::isPcaReady() ? "OK" : "MISS");
  printStartupMapStatus("|   +-- CH0 -> OLED SH1106  0x3C",
                        (oledReadyMask & 0x01U) ? "OK" : "MISS");
  printStartupMapStatus("|   +-- CH1 -> OLED SH1106  0x3C",
                        (oledReadyMask & 0x02U) ? "OK" : "MISS");
  printStartupMapStatus("|   +-- CH2 -> OLED SH1106  0x3C",
                        (oledReadyMask & 0x04U) ? "OK" : "MISS");
  printStartupMapStatus("|   +-- CH3 -> OLED SSD1306 0x3C",
                        (oledReadyMask & 0x08U) ? "OK" : "MISS");

  snprintf(countStatus, sizeof(countStatus), "%u/4", getReadyNeoKeyCount());
  printStartupMapStatus("|   `-- CH4 -> NeoKey step chain", countStatus);
  for (uint8_t board = 0; board < CONNECTION_NEOKEY_BOARD_COUNT; board++) {
    char neoKeyLabel[48];
    snprintf(neoKeyLabel, sizeof(neoKeyLabel), "|       %s-- NeoKey 0x%02X  steps %02u-%02u",
             board + 1 < CONNECTION_NEOKEY_BOARD_COUNT ? "+" : "`",
             CONNECTION_NEOKEY_BASE_ADDRESS + board, board * 4 + 1, board * 4 + 4);
    printStartupMapStatus(neoKeyLabel, neokeyBoardReady[board] ? "OK" : "MISS");
  }

  printStartupMapStatus("+-- I2C D19/D15 -> MCP23017 0x20 / encoders",
                        ControlSurface::isEncoderExpanderReady() ? "OK" : "MISS");
  printStartupMapLine("`-- UART2 115200 8N1");
  printStartupMapStatus("    +--> ESP TX2 D14 -> Daisy D14 / pin 15 RX",
                        daisyForwardReady ? "ACK" : "WAIT");
  printStartupMapStatus("    `<-- ESP RX2 D13 <- Daisy D13 / pin 14 TX",
                        daisyReturnReady ? "RX" : "WAIT");

  printStartupMapBorder('-');
  printStartupMapStatus("AUDIO -> Daisy OUT L/R pins 18/19, AGND pin 20",
                        daisyForwardReady ? "READY" : "CHECK");
  uint8_t problems = getConnectionProblemCount();
  if (problems == 0) {
    strcpy(systemStatus, "READY");
  } else {
    snprintf(systemStatus, sizeof(systemStatus), "CHECK %u", problems);
  }
  printStartupMapStatus("SYSTEM", systemStatus);
  printStartupMapBorder('-');
  printStartupMapCentered("Live logs: Utility > Serial Diagnostics");
  printStartupMapBorder('=');
  Serial.println();
}

void printConnectionDiagnosticsSnapshot() {
  serialDebugBegin("STARTUP CONNECTION SUMMARY");
  Serial.println("TFT ST7735 SPI: READY");

  Serial.print("PCA9548A 0x");
  Serial.print(ControlSurface::getPcaAddress(), HEX);
  Serial.print(": ");
  Serial.println(ControlSurface::isPcaReady() ? "CONNECTED" : "MISSING");

  Serial.print("OLED detected/initialized: ");
  Serial.print(countReadyBits(ControlSurface::getOledDetectedMask()));
  Serial.print("/4 detected, ");
  Serial.print(countReadyBits(ControlSurface::getOledReadyMask()));
  Serial.println("/4 ready");

  Serial.print("NeoKey ready: ");
  Serial.print(getReadyNeoKeyCount());
  Serial.println("/4");
  for (uint8_t board = 0; board < CONNECTION_NEOKEY_BOARD_COUNT; board++) {
    Serial.print("  NeoKey 0x");
    Serial.print(CONNECTION_NEOKEY_BASE_ADDRESS + board, HEX);
    Serial.print(": ");
    Serial.println(neokeyBoardReady[board] ? "CONNECTED" : "MISSING");
  }

  Serial.print("MCP23017 0x20 / encoders: ");
  Serial.println(ControlSurface::isEncoderExpanderReady() ? "CONNECTED" : "MISSING");

  Serial.print("ESP32 TX2 -> Daisy D14/RX: ");
  Serial.println(isDaisyForwardLinkReady() ? "CONNECTED" : "WAITING FOR ACK");
  Serial.print("Daisy D13/TX -> ESP32 RX2: ");
  Serial.println(isDaisyReturnLinkReady() ? "CONNECTED" : "NO STATUS PACKETS");
  Serial.print("UART tx/drop, rx/bad: ");
  Serial.print(DaisyBridge::getSentControlPacketCount());
  Serial.print("/");
  Serial.print(DaisyBridge::getDroppedControlPacketCount());
  Serial.print(", ");
  Serial.print(DaisyBridge::getReceivedStatusPacketCount());
  Serial.print("/");
  Serial.println(DaisyBridge::getRejectedStatusPacketCount());
  Serial.print("Daisy accepted/rejected controls: ");
  Serial.print(DaisyBridge::getAcceptedPacketCount());
  Serial.print("/");
  Serial.println(DaisyBridge::getRejectedPacketCount());
  Serial.print("Daisy UART RX restarts/errors: ");
  Serial.print(DaisyBridge::getUartRxRestartCount());
  Serial.print("/");
  Serial.println(DaisyBridge::getUartRxErrorCount());
  if (isDaisyReturnLinkReady() && !isDaisyForwardLinkReady()) {
    if (DaisyBridge::getRejectedPacketCount() == 0) {
      Serial.println("HINT: Daisy retries RX automatically; if WAIT persists, check ESP TX2 -> Daisy D14/RX pin 15");
    } else {
      Serial.println("HINT: Daisy receives but rejects data; reflash matching Daisy firmware");
    }
  }
  Serial.println("Audio backend: DAISY SEED");

  uint8_t problems = getConnectionProblemCount();
  Serial.print("Overall: ");
  if (problems == 0) {
    Serial.println("ALL CONNECTIONS READY");
  } else {
    Serial.print("CHECK ");
    Serial.print(problems);
    Serial.println(" CONNECTION(S)");
  }
  serialDebugEnd("STARTUP CONNECTION SUMMARY");
}
