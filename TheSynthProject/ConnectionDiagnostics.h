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
#if SYNTH_AUDIO_BACKEND_DAISY
  return DaisyBridge::isConnected();
#else
  return true;
#endif
}

bool isDaisyReturnLinkReady() {
#if SYNTH_AUDIO_BACKEND_DAISY
  return DaisyBridge::isReturnLinkActive();
#else
  return true;
#endif
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
#if SYNTH_AUDIO_BACKEND_DAISY
  if (!isDaisyForwardLinkReady()) problems++;
  if (!isDaisyReturnLinkReady()) problems++;
#else
  if (!dacI2sStarted) problems++;
#endif
  return problems;
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

#if SYNTH_AUDIO_BACKEND_DAISY
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
#else
  Serial.print("PCM5102 I2S: ");
  Serial.println(dacI2sStarted ? "READY" : "NOT READY");
  Serial.println("Daisy UART: DISABLED BY AUDIO BACKEND");
#endif

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
