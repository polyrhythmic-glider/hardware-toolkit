#include "ControlSurface.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <stdio.h>

#include "ControlSurfaceBridge.h"
#include "HardwarePins.h"
#include "SerialDebug.h"

namespace ControlSurface {
namespace {

constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t DISPLAY_COUNT = 4;
constexpr uint8_t OLED_ADDRESS_PRIMARY = 0x3C;
constexpr uint8_t OLED_ADDRESS_SECONDARY = 0x3D;
constexpr uint32_t MAIN_I2C_CLOCK_HZ = 400000;
constexpr uint32_t ENCODER_I2C_CLOCK_HZ = 400000;
constexpr uint8_t MCP_IODIRA = 0x00;
constexpr uint8_t MCP_GPPUA = 0x0C;
constexpr uint8_t MCP_PORTA_DATA_REGISTER = 0x12;
constexpr uint8_t ENCODER_STEPS_PER_DETENT = 4;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
// Resta visibile durante il boot del TFT, ma usa solo una checklist testuale.
constexpr uint32_t STARTUP_SPLASH_MS = 3600;
constexpr uint32_t DRAW_INTERVAL_MS = 55;
constexpr uint32_t PONG_STEP_MS = 20;
constexpr int UI_HEADER_HEIGHT = 16;
constexpr int UI_CONTENT_TOP = 17;
constexpr int UI_CONTENT_BOTTOM = 63;
constexpr int PONG_WORLD_WIDTH = SCREEN_WIDTH * DISPLAY_COUNT;
constexpr int PONG_LEFT_X = 4;
constexpr int PONG_RIGHT_X = PONG_WORLD_WIDTH - 5;
constexpr int PONG_PADDLE_HALF = 7;
constexpr float PONG_SPEED_MULTIPLIER = 4.0f;
constexpr float PONG_BASE_X_SPEED = 1.45f * PONG_SPEED_MULTIPLIER;
constexpr float PONG_BASE_Y_SPEED = 0.75f * PONG_SPEED_MULTIPLIER;
constexpr float PONG_MAX_X_SPEED = 3.0f * PONG_SPEED_MULTIPLIER;
constexpr float PONG_MAX_Y_SPEED = 2.4f * PONG_SPEED_MULTIPLIER;

enum class OledController : uint8_t {
  SSD1306,
  SH1106
};

constexpr OledController controllerForChannel(uint8_t channel) {
  return (SURFACE_OLED_SH1106_MASK & (uint8_t)(1U << channel))
           ? OledController::SH1106
           : OledController::SSD1306;
}

const char* controllerName(OledController controller) {
  return controller == OledController::SH1106 ? "SH1106" : "SSD1306";
}

// Adafruit_GFX usa lo stesso framebuffer 128x64 per entrambi i controller.
// L'SH1106, pero', richiede page addressing e un offset orizzontale di due
// colonne; il trasferimento SSD1306 standard usa invece column addressing.
// Questa classe conserva tutte le primitive grafiche gia' usate dalla UI e
// cambia soltanto inizializzazione e invio del framebuffer.
class MixedOledDisplay : public Adafruit_SSD1306 {
 public:
  MixedOledDisplay(uint8_t width, uint8_t height, TwoWire* bus, int8_t resetPin)
    : Adafruit_SSD1306(width,
                       height,
                       bus,
                       resetPin,
                       MAIN_I2C_CLOCK_HZ,
                       MAIN_I2C_CLOCK_HZ) {}

  bool beginController(OledController controller, uint8_t address) {
    selectController(controller, address);

    // Alloca il framebuffer e configura il bus. L'inizializzazione SSD1306
    // temporanea viene subito sostituita dalla sequenza SH1106 quando serve.
    if (!Adafruit_SSD1306::begin(SSD1306_SWITCHCAPVCC,
                                 address,
                                 false,
                                 false)) {
      return false;
    }

    selectController(controller, address);
    if (controller == OledController::SH1106) return beginSh1106();
    return true;
  }

  void selectController(OledController controller, uint8_t address) {
    controller_ = controller;
    if (address != 0) i2caddr = address;
  }

  void display() {
    if (controller_ == OledController::SH1106) {
      displaySh1106();
    } else {
      Adafruit_SSD1306::display();
    }
  }

 private:
  static constexpr uint8_t SH1106_COLUMN_OFFSET = 2;
  static constexpr uint8_t I2C_DATA_CHUNK = 31;
  OledController controller_ = OledController::SSD1306;

  bool writeCommandList(const uint8_t* commands, size_t count) {
    wire->setClock(MAIN_I2C_CLOCK_HZ);
    wire->beginTransmission((uint8_t)i2caddr);
    wire->write((uint8_t)0x00);
    wire->write(commands, count);
    bool ok = wire->endTransmission(true) == 0;
    wire->setClock(MAIN_I2C_CLOCK_HZ);
    return ok;
  }

  bool beginSh1106() {
    // Sequenza per SH1106G 128x64, inclusi DC/DC interno e page addressing.
    static const uint8_t initCommands[] = {
      0xAE,             // display off
      0xD5, 0x80,       // clock divide
      0xA8, 0x3F,       // multiplex 64 righe
      0xD3, 0x00,       // display offset
      0x40,             // start line 0
      0xAD, 0x8B,       // DC/DC on
      0xA1,             // segment remap
      0xC8,             // COM scan reverse
      0xDA, 0x12,       // COM pins
      0x81, 0xFF,       // contrast
      0xD9, 0x1F,       // precharge
      0xDB, 0x40,       // VCOM deselect
      0x33,             // VPP 9 V
      0xA6,             // normal display
      0x20, 0x10,       // page addressing mode
      0xA4              // display follows RAM
    };
    if (!writeCommandList(initCommands, sizeof(initCommands))) return false;

    delay(100);
    const uint8_t displayOn = 0xAF;
    return writeCommandList(&displayOn, 1);
  }

  void displaySh1106() {
    wire->setClock(MAIN_I2C_CLOCK_HZ);

    for (uint8_t page = 0; page < SCREEN_HEIGHT / 8; page++) {
      const uint8_t column = SH1106_COLUMN_OFFSET;
      const uint8_t pageCommands[] = {
        (uint8_t)(0xB0 + page),
        (uint8_t)(0x10 + (column >> 4)),
        (uint8_t)(column & 0x0F)
      };
      if (!writeCommandList(pageCommands, sizeof(pageCommands))) break;

      uint16_t offset = (uint16_t)page * SCREEN_WIDTH;
      uint8_t remaining = SCREEN_WIDTH;
      while (remaining != 0) {
        uint8_t chunk = min(remaining, I2C_DATA_CHUNK);
        wire->beginTransmission((uint8_t)i2caddr);
        wire->write((uint8_t)0x40);
        wire->write(buffer + offset, chunk);
        if (wire->endTransmission(true) != 0) {
          wire->setClock(MAIN_I2C_CLOCK_HZ);
          return;
        }
        offset += chunk;
        remaining -= chunk;
      }
      yield();
    }

    wire->setClock(MAIN_I2C_CLOCK_HZ);
  }
};

const int8_t QUADRATURE_TABLE[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

const uint8_t ENCODER_PIN_A[4] = { 0, 3, 6, 9 };
const uint8_t ENCODER_PIN_B[4] = { 1, 4, 7, 10 };
const uint8_t ENCODER_PIN_SW[4] = { 2, 5, 8, 11 };

struct EncoderState {
  volatile long value;
  volatile int32_t pendingDelta;
  int8_t accumulator;
  uint8_t lastAb;
  uint8_t lastRawButton;
  volatile uint8_t stableButton;
  uint32_t buttonChangedAt;
  volatile bool clickPending;
};

struct InputFrame {
  int32_t delta[4];
  long value[4];
  bool click[4];
  bool pressed[4];
};

MixedOledDisplay oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
TwoWire encoderWire(1);
EncoderState encoders[4] = {};
portMUX_TYPE encoderStateMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t encoderTaskHandle = nullptr;

volatile bool encoderExpanderReady = false;
volatile bool encoderInputDirty = false;
volatile uint32_t encoderReadErrors = 0;
volatile uint32_t encoderReadCount = 0;
volatile uint16_t encoderRawPorts = 0xFFFF;
volatile bool encoderRawPortsValid = false;
bool encoderSdaHigh = false;
bool encoderSclHigh = false;
uint8_t encoderBusAddresses[16] = {};
uint8_t encoderBusDeviceCount = 0;

bool pcaReady = false;
bool oledBusReady = false;
uint8_t pcaAddress = SURFACE_PCA9548A_ADDRESS;
uint8_t oledAddress = OLED_ADDRESS_PRIMARY;
uint8_t oledReadyMask = 0;
uint8_t oledDetectedMask = 0;
uint8_t oledChannelAddress[DISPLAY_COUNT] = { 0, 0, 0, 0 };
uint8_t mainBusAddresses[16] = {};
uint8_t mainBusDeviceCount = 0;
uint8_t neokeyDetectedMask = 0;
bool mainSdaHigh = false;
bool mainSclHigh = false;
uint8_t activeDisplay = 0;

volatile Mode currentMode = SYNTH_CONTROL;
bool menuOpen = false;
int menuSelection = SYNTH_CONTROL;
uint8_t synthPage = ControlSurfaceBridge::PAGE_VOICE_1;
bool redrawRequested = true;
uint32_t lastDrawAt = 0;
uint32_t startupUntil = 0;
uint32_t lastMainStateHash = 0;
uint32_t clickFlashUntil[4] = { 0, 0, 0, 0 };
uint8_t lastTouchedPage = ControlSurfaceBridge::PAGE_VOICE_1;
uint8_t lastTouchedEncoder = 0;
uint32_t lastParameterChangeMs = 0;

const uint8_t VOICE_1_CONTEXT_PAGES[] = {
  ControlSurfaceBridge::PAGE_VOICE_1,
  ControlSurfaceBridge::PAGE_TUNING,
  ControlSurfaceBridge::PAGE_SEQUENCER,
  ControlSurfaceBridge::PAGE_TRANSPORT
};
const uint8_t VOICE_2_CONTEXT_PAGES[] = {
  ControlSurfaceBridge::PAGE_VOICE_2,
  ControlSurfaceBridge::PAGE_TUNING,
  ControlSurfaceBridge::PAGE_SEQUENCER,
  ControlSurfaceBridge::PAGE_TRANSPORT
};
const uint8_t KICK_CONTEXT_PAGES[] = {
  ControlSurfaceBridge::PAGE_KICK,
  ControlSurfaceBridge::PAGE_KICK_SHAPE,
  ControlSurfaceBridge::PAGE_SEQUENCER,
  ControlSurfaceBridge::PAGE_TRANSPORT
};

const uint8_t* getContextPages(uint8_t& count) {
  count = 4;
  uint8_t trackPage = ControlSurfaceBridge::activeTrackPage();
  if (trackPage == ControlSurfaceBridge::PAGE_VOICE_2) return VOICE_2_CONTEXT_PAGES;
  if (trackPage == ControlSurfaceBridge::PAGE_KICK) return KICK_CONTEXT_PAGES;
  return VOICE_1_CONTEXT_PAGES;
}

void syncSynthPageToActiveTrack() {
  uint8_t count = 0;
  const uint8_t* pages = getContextPages(count);
  for (uint8_t i = 0; i < count; i++) {
    if (synthPage == pages[i]) return;
  }

  synthPage = pages[0];
  lastTouchedPage = synthPage;
  lastTouchedEncoder = 0;
  lastParameterChangeMs = millis();
  redrawRequested = true;
}

void stepContextPage(int delta) {
  if (delta == 0) return;

  uint8_t count = 0;
  const uint8_t* pages = getContextPages(count);
  int current = 0;
  for (uint8_t i = 0; i < count; i++) {
    if (synthPage == pages[i]) {
      current = i;
      break;
    }
  }

  current += delta > 0 ? 1 : -1;
  while (current < 0) current += count;
  synthPage = pages[current % count];
  lastTouchedPage = synthPage;
  lastTouchedEncoder = 0;
  lastParameterChangeMs = millis();
  redrawRequested = true;
}

volatile int legacyWaveform = 0;
volatile int legacyParameterA = 0;
volatile int legacyParameterB = 0;

float ballX = PONG_WORLD_WIDTH / 2.0f;
float ballY = 40.0f;
float ballVx = PONG_BASE_X_SPEED;
float ballVy = PONG_BASE_Y_SPEED;
int leftPaddleY = 40;
int rightPaddleY = 40;
int pongScoreLeft = 0;
int pongScoreRight = 0;
bool pongGameOver = false;
long pongLastEncoder1 = 0;
long pongLastEncoder4 = 0;
uint32_t pongLastUpdateAt = 0;
uint32_t pongServeUntil = 0;

bool writeMcpPair(uint8_t startRegister, uint8_t valueA, uint8_t valueB) {
  encoderWire.beginTransmission(SURFACE_MCP23017_ADDRESS);
  encoderWire.write(startRegister);
  encoderWire.write(valueA);
  encoderWire.write(valueB);
  return encoderWire.endTransmission(true) == 0;
}

bool readMcpPorts(uint16_t& value) {
  encoderWire.beginTransmission(SURFACE_MCP23017_ADDRESS);
  encoderWire.write(MCP_PORTA_DATA_REGISTER);
  if (encoderWire.endTransmission(false) != 0) return false;

  if (encoderWire.requestFrom((uint8_t)SURFACE_MCP23017_ADDRESS, (uint8_t)2, (uint8_t)true) != 2) {
    return false;
  }

  uint8_t portA = encoderWire.read();
  uint8_t portB = encoderWire.read();
  value = (uint16_t)portA | ((uint16_t)portB << 8);
  encoderRawPorts = value;
  encoderRawPortsValid = true;
  return true;
}

uint8_t readMappedBit(uint16_t ports, uint8_t bitIndex) {
  return (ports >> bitIndex) & 0x01;
}

void processEncoderPorts(uint16_t ports) {
  uint32_t now = millis();

  for (uint8_t i = 0; i < 4; i++) {
    EncoderState& encoder = encoders[i];
    uint8_t a = readMappedBit(ports, ENCODER_PIN_A[i]);
    uint8_t b = readMappedBit(ports, ENCODER_PIN_B[i]);
    uint8_t newAb = (a << 1) | b;
    uint8_t tableIndex = (encoder.lastAb << 2) | newAb;
    int8_t movement = QUADRATURE_TABLE[tableIndex];

    if (movement != 0) {
      encoder.accumulator += movement;
      int8_t step = 0;

      if (encoder.accumulator >= ENCODER_STEPS_PER_DETENT) {
        encoder.accumulator = 0;
        step = 1;
      } else if (encoder.accumulator <= -ENCODER_STEPS_PER_DETENT) {
        encoder.accumulator = 0;
        step = -1;
      }

      if (step != 0) {
        portENTER_CRITICAL(&encoderStateMux);
        encoder.value += step;
        encoder.pendingDelta += step;
        encoderInputDirty = true;
        portEXIT_CRITICAL(&encoderStateMux);
      }
    }

    encoder.lastAb = newAb;

    uint8_t rawButton = readMappedBit(ports, ENCODER_PIN_SW[i]);
    if (rawButton != encoder.lastRawButton) {
      encoder.lastRawButton = rawButton;
      encoder.buttonChangedAt = now;
    }

    if (rawButton != encoder.stableButton && now - encoder.buttonChangedAt >= BUTTON_DEBOUNCE_MS) {
      portENTER_CRITICAL(&encoderStateMux);
      encoder.stableButton = rawButton;
      if (rawButton == LOW) {
        encoder.clickPending = true;
        encoderInputDirty = true;
      }
      portEXIT_CRITICAL(&encoderStateMux);
    }
  }
}

void encoderPollTask(void* parameter) {
  (void)parameter;
  TickType_t nextPoll = xTaskGetTickCount();

  while (true) {
    uint16_t ports = 0xFFFF;
    if (readMcpPorts(ports)) {
      encoderReadCount = encoderReadCount + 1;
      processEncoderPorts(ports);
    } else {
      encoderReadErrors = encoderReadErrors + 1;
    }

    // 500 Hz is ample for mechanical quadrature encoders. At 400 kHz this
    // keeps the independent MCP bus responsive without monopolising core 1.
    vTaskDelayUntil(&nextPoll, pdMS_TO_TICKS(2));
  }
}

bool beginEncoderExpander() {
  if (!encoderWire.begin(SURFACE_ENCODER_I2C_SDA,
                         SURFACE_ENCODER_I2C_SCL,
                         ENCODER_I2C_CLOCK_HZ)) {
    return false;
  }

  if (!writeMcpPair(MCP_IODIRA, 0xFF, 0xFF)) return false;
  if (!writeMcpPair(MCP_GPPUA, 0xFF, 0xFF)) return false;

  uint16_t ports = 0xFFFF;
  if (!readMcpPorts(ports)) return false;

  uint32_t now = millis();
  for (uint8_t i = 0; i < 4; i++) {
    encoders[i].value = 0;
    encoders[i].pendingDelta = 0;
    encoders[i].accumulator = 0;
    encoders[i].lastAb = (readMappedBit(ports, ENCODER_PIN_A[i]) << 1) |
                         readMappedBit(ports, ENCODER_PIN_B[i]);
    encoders[i].lastRawButton = readMappedBit(ports, ENCODER_PIN_SW[i]);
    encoders[i].stableButton = encoders[i].lastRawButton;
    encoders[i].buttonChangedAt = now;
    encoders[i].clickPending = false;
  }

  encoderExpanderReady = true;

  bool taskStarted = xTaskCreatePinnedToCore(
    encoderPollTask,
    "surfaceEncoders",
    3072,
    nullptr,
    2,
    &encoderTaskHandle,
    1
  ) == pdPASS;
  if (!taskStarted) encoderExpanderReady = false;
  return taskStarted;
}

bool storedEncoderBusAddressPresent(uint8_t address) {
  uint8_t storedCount = min(encoderBusDeviceCount, (uint8_t)sizeof(encoderBusAddresses));
  for (uint8_t i = 0; i < storedCount; i++) {
    if (encoderBusAddresses[i] == address) return true;
  }
  return false;
}

void snapshotEncoderBus() {
  // Il task MCP puo' trovarsi nel mezzo di una transazione quando viene aperto
  // il pannello diagnostico. Attende una finestra di bus realmente inattivo,
  // evitando di segnalare un falso LOW durante START/clock/dati.
  encoderSdaHigh = false;
  encoderSclHigh = false;
  uint32_t idleWaitStartedAt = micros();
  do {
    bool sdaNowHigh = digitalRead(SURFACE_ENCODER_I2C_SDA) == HIGH;
    bool sclNowHigh = digitalRead(SURFACE_ENCODER_I2C_SCL) == HIGH;
    if (sdaNowHigh && sclNowHigh) {
      encoderSdaHigh = true;
      encoderSclHigh = true;
      break;
    }
    delayMicroseconds(5);
  } while ((uint32_t)(micros() - idleWaitStartedAt) < 3000);

  encoderBusDeviceCount = 0;
  memset(encoderBusAddresses, 0, sizeof(encoderBusAddresses));

  if (encoderExpanderReady && encoderTaskHandle != nullptr) {
    encoderBusAddresses[0] = SURFACE_MCP23017_ADDRESS;
    encoderBusDeviceCount = 1;
    return;
  }

  encoderWire.setClock(ENCODER_I2C_CLOCK_HZ);
  for (uint8_t address = 1; address < 0x7F; address++) {
    encoderWire.beginTransmission(address);
    if (encoderWire.endTransmission(true) != 0) continue;
    if (encoderBusDeviceCount < sizeof(encoderBusAddresses)) {
      encoderBusAddresses[encoderBusDeviceCount] = address;
    }
    if (encoderBusDeviceCount < 0xFF) encoderBusDeviceCount++;
  }
}

void printMcpDiagnosticReport() {
  serialDebugBegin("MCP23017 / ENCODERS");
  Serial.print("D19 SDA idle: ");
  Serial.println(encoderSdaHigh ? "HIGH" : "LOW - CHECK SHORT/WIRING");
  Serial.print("D15 SCL idle: ");
  Serial.println(encoderSclHigh ? "HIGH" : "LOW - CHECK SHORT/WIRING");

  Serial.print("Encoder bus devices:");
  uint8_t storedCount = min(encoderBusDeviceCount, (uint8_t)sizeof(encoderBusAddresses));
  if (storedCount == 0) Serial.print(" none");
  for (uint8_t i = 0; i < storedCount; i++) {
    Serial.print(" 0x");
    if (encoderBusAddresses[i] < 0x10) Serial.print('0');
    Serial.print(encoderBusAddresses[i], HEX);
  }
  Serial.println();

  Serial.print("MCP23017 0x20: ");
  Serial.println(encoderExpanderReady ? "OK" : "MISSING - check D19/D15/VCC/GND/RESET/A0-A2");
  Serial.print("Raw ports A/B: ");
  if (encoderRawPortsValid) {
    Serial.print("0x");
    if (encoderRawPorts < 0x1000) Serial.print('0');
    if (encoderRawPorts < 0x0100) Serial.print('0');
    if (encoderRawPorts < 0x0010) Serial.print('0');
    Serial.println((uint16_t)encoderRawPorts, HEX);
  } else {
    Serial.println("----");
  }

  if (!encoderRawPortsValid) {
    Serial.println("Encoder CLK/DT/SW: UNKNOWN - MCP23017 has not returned data");
  } else {
    uint16_t ports = encoderRawPorts;
    for (uint8_t i = 0; i < 4; i++) {
      Serial.print("Encoder ");
      Serial.print(i + 1);
      Serial.print("  CLK:");
      Serial.print(readMappedBit(ports, ENCODER_PIN_A[i]) ? "HIGH" : "LOW");
      Serial.print("  DT:");
      Serial.print(readMappedBit(ports, ENCODER_PIN_B[i]) ? "HIGH" : "LOW");
      Serial.print("  SW:");
      Serial.println(readMappedBit(ports, ENCODER_PIN_SW[i]) ? "HIGH" : "LOW");
    }
  }
  Serial.print("Reads: ");
  Serial.print((uint32_t)encoderReadCount);
  Serial.print("  errors: ");
  Serial.println((uint32_t)encoderReadErrors);
  Serial.println("At rest CLK/DT may be HIGH/HIGH or LOW/LOW; pressed SW becomes LOW.");
  serialDebugEnd("MCP23017 / ENCODERS");
}

void consumeInput(InputFrame& input) {
  portENTER_CRITICAL(&encoderStateMux);
  for (uint8_t i = 0; i < 4; i++) {
    input.delta[i] = encoders[i].pendingDelta;
    input.value[i] = encoders[i].value;
    input.click[i] = encoders[i].clickPending;
    input.pressed[i] = encoders[i].stableButton == LOW;
    encoders[i].pendingDelta = 0;
    encoders[i].clickPending = false;
  }
  encoderInputDirty = false;
  portEXIT_CRITICAL(&encoderStateMux);
}

void resetEncoderValue(uint8_t index) {
  if (index >= 4) return;
  portENTER_CRITICAL(&encoderStateMux);
  encoders[index].value = 0;
  encoders[index].pendingDelta = 0;
  encoders[index].accumulator = 0;
  portEXIT_CRITICAL(&encoderStateMux);
}

long getEncoderValue(uint8_t index) {
  if (index >= 4) return 0;
  portENTER_CRITICAL(&encoderStateMux);
  long value = encoders[index].value;
  portEXIT_CRITICAL(&encoderStateMux);
  return value;
}

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission(true) == 0;
}

void selectPcaChannel(uint8_t channel) {
  if (!pcaReady || channel >= 8) return;
  Wire.beginTransmission(pcaAddress);
  Wire.write(1 << channel);
  Wire.endTransmission(true);
}

void disablePcaChannels() {
  if (!pcaReady) return;
  Wire.beginTransmission(pcaAddress);
  Wire.write(0);
  Wire.endTransmission(true);
}

void scanMainBus() {
  mainBusDeviceCount = 0;
  for (uint8_t address = 1; address < 0x7F; address++) {
    if (!i2cDevicePresent(address)) continue;
    if (mainBusDeviceCount < sizeof(mainBusAddresses)) {
      mainBusAddresses[mainBusDeviceCount] = address;
    }
    if (mainBusDeviceCount < 0xFF) mainBusDeviceCount++;
  }
}

bool storedMainBusAddressPresent(uint8_t address) {
  uint8_t storedCount = min(mainBusDeviceCount, (uint8_t)sizeof(mainBusAddresses));
  for (uint8_t i = 0; i < storedCount; i++) {
    if (mainBusAddresses[i] == address) return true;
  }
  return false;
}

void scanOledHardware() {
  Wire.setClock(MAIN_I2C_CLOCK_HZ);
  mainSdaHigh = digitalRead(NEOKEY_I2C_SDA) == HIGH;
  mainSclHigh = digitalRead(NEOKEY_I2C_SCL) == HIGH;
  pcaReady = false;
  pcaAddress = SURFACE_PCA9548A_ADDRESS;
  oledDetectedMask = 0;
  neokeyDetectedMask = 0;
  memset(oledChannelAddress, 0, sizeof(oledChannelAddress));

  for (uint8_t address = 0x70; address <= 0x77; address++) {
    if (i2cDevicePresent(address)) {
      pcaAddress = address;
      pcaReady = true;
      break;
    }
  }

  // Esclude tutti i canali dalla scansione del bus principale: qui deve
  // apparire il PCA, mentre OLED e NeoKey sono dietro al multiplexer.
  disablePcaChannels();
  scanMainBus();

  if (!pcaReady) return;

  for (uint8_t channel = 0; channel < DISPLAY_COUNT; channel++) {
    selectPcaChannel(channel);

    uint8_t candidateAddress = 0;
    if (i2cDevicePresent(OLED_ADDRESS_PRIMARY)) candidateAddress = OLED_ADDRESS_PRIMARY;
    else if (i2cDevicePresent(OLED_ADDRESS_SECONDARY)) candidateAddress = OLED_ADDRESS_SECONDARY;
    if (candidateAddress != 0) {
      oledDetectedMask |= (1 << channel);
      oledChannelAddress[channel] = candidateAddress;
    }
  }

  Wire.setClock(NEOKEY_I2C_CLOCK_HZ);
  selectPcaChannel(NEOKEY_PCA_CHANNEL);
  for (uint8_t address = 0x30; address <= 0x33; address++) {
    if (i2cDevicePresent(address)) {
      neokeyDetectedMask |= (uint8_t)(1U << (address - 0x30));
    }
  }

  Wire.setClock(MAIN_I2C_CLOCK_HZ);
  selectPcaChannel(0);
}

void printOledDiagnosticReport() {
  serialDebugBegin("OLED / PCA / NEOKEY SCAN");
  Serial.print("D21 SDA idle: ");
  Serial.println(mainSdaHigh ? "HIGH" : "LOW - CHECK SHORT/WIRING");
  Serial.print("D22 SCL idle: ");
  Serial.println(mainSclHigh ? "HIGH" : "LOW - CHECK SHORT/WIRING");

  Serial.print("Root bus devices:");
  uint8_t storedCount = min(mainBusDeviceCount, (uint8_t)sizeof(mainBusAddresses));
  if (storedCount == 0) Serial.print(" none");
  for (uint8_t i = 0; i < storedCount; i++) {
    Serial.print(" 0x");
    if (mainBusAddresses[i] < 0x10) Serial.print('0');
    Serial.print(mainBusAddresses[i], HEX);
  }
  Serial.println();

  Serial.print("PCA: ");
  if (pcaReady) {
    Serial.print("OK at 0x");
    Serial.println(pcaAddress, HEX);
  } else {
    Serial.println("MISSING - check D21/D22/VIN/GND/RST/A0-A2");
  }

  for (uint8_t channel = 0; channel < DISPLAY_COUNT; channel++) {
    Serial.print("PCA channel ");
    Serial.print(channel);
    Serial.print(" (SC");
    Serial.print(channel);
    Serial.print("/SD");
    Serial.print(channel);
    Serial.print("): ");
    if (oledChannelAddress[channel] != 0) {
      Serial.print("OLED ACK 0x");
      Serial.print(oledChannelAddress[channel], HEX);
      Serial.print(" ");
      Serial.print(controllerName(controllerForChannel(channel)));
      Serial.println((oledReadyMask & (1 << channel)) ? " INIT OK" : " INIT FAIL");
    } else {
      Serial.println("NO OLED - check GND/VCC/SCL/SDA");
    }
  }

  Serial.print("Detected mask: 0x");
  Serial.print(oledDetectedMask, HEX);
  Serial.print("  initialized mask: 0x");
  Serial.println(oledReadyMask, HEX);
  for (uint8_t address = 0x30; address <= 0x33; address++) {
    Serial.print("PCA channel 4 NeoKey 0x");
    Serial.print(address, HEX);
    Serial.print(": ");
    Serial.println((neokeyDetectedMask & (1U << (address - 0x30))) ? "OK" : "MISS");
  }
  Serial.println("Expected for four OLED: detected 0xF, initialized 0xF");
  serialDebugEnd("OLED / PCA / NEOKEY SCAN");
}

bool beginOleds() {
  scanOledHardware();
  oledReadyMask = 0;

  if (!pcaReady) {
    oledBusReady = false;
    if (isSerialDiagnosticsEnabled()) printOledDiagnosticReport();
    return false;
  }

  for (uint8_t channel = 0; channel < DISPLAY_COUNT; channel++) {
    uint8_t candidateAddress = oledChannelAddress[channel];
    if (candidateAddress == 0) continue;

    selectPcaChannel(channel);

    oledAddress = candidateAddress;
    if (!oled.beginController(controllerForChannel(channel), oledAddress)) continue;

    oledReadyMask |= (1 << channel);
    oled.clearDisplay();
    oled.display();
  }

  selectPcaChannel(0);
  oledBusReady = oledReadyMask != 0;
  if (isSerialDiagnosticsEnabled()) printOledDiagnosticReport();
  return oledBusReady;
}

void setDisplay(uint8_t index) {
  activeDisplay = index % DISPLAY_COUNT;
  selectPcaChannel(activeDisplay);
  oled.selectController(controllerForChannel(activeDisplay),
                        oledChannelAddress[activeDisplay]);
}

void centerText(const char* text, int y, uint8_t size) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  oled.setTextSize(size);
  oled.getTextBounds(text, 0, y, &x1, &y1, &width, &height);
  oled.setCursor((SCREEN_WIDTH - width) / 2, y);
  oled.print(text);
}

void drawCenteredText(const char* text, int y, uint8_t size, uint16_t color) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  oled.setTextSize(size);
  oled.getTextBounds(text, 0, y, &x1, &y1, &width, &height);
  int x = max(0, (SCREEN_WIDTH - (int)width) / 2);
  oled.setTextColor(color);
  oled.setCursor(x, y);
  oled.print(text);
}

void drawTitleBar(const char* title) {
  oled.fillRect(0, 0, SCREEN_WIDTH, UI_HEADER_HEIGHT, SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_BLACK);
  oled.setCursor(2, 4);
  oled.print(title);
  oled.setCursor(108, 4);
  oled.print("E");
  oled.print(activeDisplay + 1);
  oled.setTextColor(SSD1306_WHITE);
}

void drawStatusFrame(const char* title) {
  oled.clearDisplay();
  drawTitleBar(title);
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
}

void drawSynthParameterCard(const char* page, const char* label, const char* value, uint8_t slot) {
  drawStatusFrame(page);

  oled.drawRoundRect(3, 19, 122, 42, 4, SSD1306_WHITE);
  oled.drawPixel(6, 22, SSD1306_WHITE);
  oled.drawPixel(121, 22, SSD1306_WHITE);
  oled.drawPixel(6, 57, SSD1306_WHITE);
  oled.drawPixel(121, 57, SSD1306_WHITE);

  int16_t x1;
  int16_t y1;
  uint16_t labelWidth;
  uint16_t labelHeight;
  oled.setTextSize(1);
  oled.getTextBounds(label, 0, 0, &x1, &y1, &labelWidth, &labelHeight);
  int pillW = constrain((int)labelWidth + 14, 34, 112);
  int pillX = (SCREEN_WIDTH - pillW) / 2;
  oled.fillRoundRect(pillX, 23, pillW, 12, 3, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setCursor(pillX + (pillW - (int)labelWidth) / 2, 26);
  oled.print(label);

  uint8_t valueSize = strlen(value) <= 8 ? 2 : 1;
  int valueY = valueSize == 2 ? 41 : 44;
  drawCenteredText(value, valueY, valueSize, SSD1306_WHITE);
}

void drawBootCheck(const char* label, uint8_t row, bool checked) {
  constexpr int statusX = 115;
  int y = 15 + row * 12;

  oled.setCursor(0, y);
  oled.print("- ");
  oled.print(label);
  oled.drawRoundRect(statusX, y - 1, 11, 9, 2, SSD1306_WHITE);

  if (checked) {
    oled.drawLine(statusX + 2, y + 3, statusX + 4, y + 5, SSD1306_WHITE);
    oled.drawLine(statusX + 4, y + 5, statusX + 8, y + 1, SSD1306_WHITE);
    oled.drawLine(statusX + 2, y + 4, statusX + 4, y + 6, SSD1306_WHITE);
    oled.drawLine(statusX + 4, y + 6, statusX + 8, y + 2, SSD1306_WHITE);
  }
}

void drawSplashPanel() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);

  if (activeDisplay == 0) {
    oled.println("> BOOT / CORE");
    drawBootCheck("CPU ONLINE", 0, true);
    drawBootCheck("RTOS ACTIVE", 1, true);
    drawBootCheck("MEMORY READY", 2, true);
    drawBootCheck("FW LOADED", 3, true);
  } else if (activeDisplay == 1) {
    oled.println("> BOOT / DISPLAYS");
    drawBootCheck("TFT ONLINE", 0, true);
    drawBootCheck("I2C LINES", 1, mainSdaHigh && mainSclHigh);
    drawBootCheck("PCA MUX", 2, pcaReady);
    drawBootCheck("OLED BANK", 3, oledReadyMask != 0);
  } else if (activeDisplay == 2) {
    oled.println("> BOOT / CONTROLS");
    drawBootCheck("INPUT BUS", 0, encoderExpanderReady);
    drawBootCheck("MCP23017", 1, encoderExpanderReady);
    drawBootCheck("ENCODER TASK", 2, encoderTaskHandle != nullptr);
    drawBootCheck("GPIO MAP", 3, true);
  } else {
    oled.println("> PRELAUNCH");
    drawBootCheck("UI MODEL", 0, true);
    drawBootCheck("PARAM MAP", 1, true);
    drawBootCheck("OLED SURFACE", 2, oledBusReady);
    drawBootCheck("SURFACE READY", 3, oledBusReady && encoderExpanderReady);
  }
}

int firstVisibleMenuItem() {
  constexpr int visibleItems = 3;
  if (menuSelection < visibleItems) return 0;
  if (menuSelection >= MODE_COUNT - 1) return MODE_COUNT - visibleItems;
  return menuSelection - 1;
}

void drawMenuPanel() {
  if (activeDisplay == 0) {
    drawStatusFrame("CONTROL SURFACE");
    oled.setCursor(0, 22);
    oled.println("4 OLED + 4 ENCODER");
    oled.println("integrated with LCD");
    oled.println();
    oled.println("shared synth state");
    return;
  }

  if (activeDisplay == 1) {
    drawStatusFrame("HARDWARE");
    oled.setCursor(0, 20);
    oled.print("PCA: "); oled.println(pcaReady ? "OK" : "MISSING");
    oled.print("OLED: "); oled.print(__builtin_popcount((unsigned int)oledReadyMask)); oled.println("/4");
    oled.print("MCP: "); oled.println(encoderExpanderReady ? "OK" : "MISSING");
    oled.println("I2C 400 kHz");
    return;
  }

  if (activeDisplay == 2) {
    drawStatusFrame("CONTROLS");
    oled.setCursor(0, 20);
    oled.println("ENCODERS: 4");
    oled.println("BUTTONS: 4");
    oled.println("NEOKEY: 16");
    oled.println("LCD LINK: READY");
    return;
  }

  drawStatusFrame("MAIN MENU");
  int first = firstVisibleMenuItem();
  for (int row = 0; row < 3; row++) {
    int item = first + row;
    int y = 19 + row * 14;
    bool selected = item == menuSelection;
    if (selected) {
      oled.fillRect(0, y - 2, 128, 12, SSD1306_WHITE);
      oled.setTextColor(SSD1306_BLACK);
    } else {
      oled.setTextColor(SSD1306_WHITE);
    }
    oled.setCursor(3, y);
    oled.print(getModeName((Mode)item));
  }
  oled.setTextColor(SSD1306_WHITE);
}

float continuousAngle(long value) {
  float degrees = -90.0f + value * 8.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;
  while (degrees < 0.0f) degrees += 360.0f;
  return degrees * 0.01745329251f;
}

void drawEncoderDial(long value, bool flash) {
  int cx = 64;
  int cy = 41;
  int radius = 20;

  for (int i = 0; i < 12; i++) {
    float angle = i * 30.0f * 0.01745329251f;
    int inner = i % 3 == 0 ? radius - 6 : radius - 3;
    oled.drawLine(
      cx + cosf(angle) * inner,
      cy + sinf(angle) * inner,
      cx + cosf(angle) * (radius + 1),
      cy + sinf(angle) * (radius + 1),
      SSD1306_WHITE
    );
  }

  if (flash) oled.drawCircle(cx, cy, radius + 4, SSD1306_WHITE);

  float angle = continuousAngle(value);
  int x1 = cx + cosf(angle) * (radius - 11);
  int y1 = cy + sinf(angle) * (radius - 11);
  int x2 = cx + cosf(angle) * (radius - 1);
  int y2 = cy + sinf(angle) * (radius - 1);
  oled.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
  oled.drawLine(x1 + 1, y1, x2 + 1, y2, SSD1306_WHITE);

  char valueText[12];
  snprintf(valueText, sizeof(valueText), "%ld", value % 100);
  centerText(valueText, 37, 1);
}

void drawEncoderPanel() {
  char title[16];
  snprintf(title, sizeof(title), "ENCODER %d", activeDisplay + 1);
  drawStatusFrame(title);
  long value = getEncoderValue(activeDisplay);
  drawEncoderDial(value, millis() < clickFlashUntil[activeDisplay]);
  oled.setCursor(activeDisplay == 3 ? 38 : 32, 56);
  oled.print(activeDisplay == 3 ? "PUSH MENU" : "PUSH RESET");
}

float mapFloat(float value, float inMin, float inMax, float outMin, float outMax) {
  return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

float constrainFloat(float value, float minValue, float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

float applyCurve(float t, float curve) {
  t = constrainFloat(t, 0.0f, 1.0f);
  return curve >= 1.0f ? powf(t, curve) : 1.0f - powf(1.0f - t, 1.0f / curve);
}

float smoothStep(float edge0, float edge1, float value) {
  float t = constrainFloat((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

float smoothedPulse(float phase, float duty, float edgeWidth) {
  float rise = smoothStep(0.0f, edgeWidth, phase);
  float fall = 1.0f - smoothStep(duty, duty + edgeWidth, phase);
  float high = min(rise, fall);
  if (phase > duty + edgeWidth) high = 0.0f;
  return high * 2.0f - 1.0f;
}

int normalizedToY(float value, int y, int height) {
  value = constrainFloat(value, -1.0f, 1.0f);
  return map((int)(value * 100.0f), -100, 100, y + height - 3, y + 3);
}

const char* legacyWaveformName(int index) {
  switch (index) {
    case 0: return "SINE";
    case 1: return "TRI";
    case 2: return "STEP";
    case 3: return "CONST";
    default: return "WAVE";
  }
}

const char* legacyParameterLabelA(int index) {
  switch (index) {
    case 0: return "DRV";
    case 1: return "SKW";
    case 2: return "DUT";
    case 3: return "LVL";
    default: return "A";
  }
}

const char* legacyParameterLabelB(int index) {
  switch (index) {
    case 0: return "WRP";
    case 1: return "CRV";
    case 2: return "EDG";
    case 3: return "RPL";
    default: return "B";
  }
}

void drawWaveformGraph() {
  int wave = legacyWaveform;
  int parameterA = legacyParameterA;
  int parameterB = legacyParameterB;
  int x = 3;
  int y = 20;
  int width = 122;
  int height = 41;

  oled.clearDisplay();
  oled.fillRect(0, 0, 128, 16, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(1, 4);
  oled.print(legacyWaveformName(wave));
  oled.setCursor(39, 4);
  oled.print(legacyParameterLabelA(wave)); oled.print(":"); oled.print(parameterA);
  oled.setCursor(84, 4);
  oled.print(legacyParameterLabelB(wave)); oled.print(":"); oled.print(parameterB);
  oled.setTextColor(SSD1306_WHITE);
  oled.drawRect(x, y, width, height, SSD1306_WHITE);
  oled.drawFastHLine(x + 2, y + height / 2, width - 4, SSD1306_WHITE);

  int previousX = x + 4;
  int previousY = normalizedToY(0.0f, y + 2, height - 4);
  for (int i = 1; i <= width - 8; i++) {
    float phase = (float)i / (float)(width - 8);
    float p = phase * 2.0f;
    p -= floorf(p);
    float sample = 0.0f;

    if (wave == 0) {
      float drive = mapFloat(parameterA, -50, 50, 0.55f, 3.20f);
      float warp = mapFloat(parameterB, -50, 50, -1.25f, 1.25f);
      float angle = p * TWO_PI;
      sample = sinf(angle + sinf(angle * 2.0f) * warp);
      sample = parameterA >= 0 ? tanhf(sample * drive) : sample * drive;
    } else if (wave == 1) {
      float skew = mapFloat(parameterA, -50, 50, 0.12f, 0.88f);
      float curve = mapFloat(parameterB, -50, 50, 0.35f, 2.80f);
      if (p < skew) sample = -1.0f + applyCurve(p / skew, curve) * 2.0f;
      else sample = 1.0f - applyCurve((p - skew) / (1.0f - skew), curve) * 2.0f;
    } else if (wave == 2) {
      float duty = mapFloat(parameterA, -50, 50, 0.05f, 0.95f);
      float edge = mapFloat(parameterB, -50, 50, 0.001f, 0.18f);
      sample = parameterB <= -45 ? (p < duty ? 1.0f : -1.0f) : smoothedPulse(p, duty, edge);
    } else {
      float level = mapFloat(parameterA, -50, 50, -0.85f, 0.85f);
      float ripple = mapFloat(abs(parameterB), 0, 50, 0.0f, 0.45f);
      sample = constrainFloat(level + sinf(p * TWO_PI) * ripple * (parameterB < 0 ? -1.0f : 1.0f), -1.0f, 1.0f);
    }

    int currentX = x + 4 + i;
    int currentY = normalizedToY(sample, y + 2, height - 4);
    oled.drawLine(previousX, previousY, currentX, currentY, SSD1306_WHITE);
    oled.drawLine(previousX, previousY + 1, currentX, currentY + 1, SSD1306_WHITE);
    previousX = currentX;
    previousY = currentY;
  }
}

void drawWaveformPanel() {
  if (activeDisplay == 0) {
    drawWaveformGraph();
  } else if (activeDisplay == 1) {
    drawStatusFrame("PARAMETERS");
    oled.setCursor(0, 21);
    oled.print(legacyParameterLabelA(legacyWaveform)); oled.print(": "); oled.println(legacyParameterA);
    oled.print(legacyParameterLabelB(legacyWaveform)); oled.print(": "); oled.println(legacyParameterB);
    oled.print("TYPE: "); oled.println(legacyWaveformName(legacyWaveform));
    oled.println("OUTPUT: VISUAL ONLY");
  } else if (activeDisplay == 2) {
    drawStatusFrame("INPUTS");
    oled.setCursor(0, 21);
    oled.println("E1 TURN: PARAM A");
    oled.println("E2 TURN: PARAM B");
    oled.println("E1 PUSH: PREV");
    oled.println("E2 PUSH: NEXT");
  } else {
    drawStatusFrame("WAVEFORM");
    oled.setCursor(0, 21);
    oled.print("E1 "); oled.println(getEncoderValue(0));
    oled.print("E2 "); oled.println(getEncoderValue(1));
    oled.println("E4 PUSH: MENU");
    oled.println("NO AUDIO OUTPUT");
  }
}

void resetPongBall(int direction) {
  ballX = PONG_WORLD_WIDTH / 2.0f;
  ballY = 40.0f;
  ballVx = direction >= 0 ? PONG_BASE_X_SPEED : -PONG_BASE_X_SPEED;
  ballVy = ((pongScoreLeft + pongScoreRight) % 2 == 0) ? PONG_BASE_Y_SPEED : -PONG_BASE_Y_SPEED;
  pongServeUntil = millis() + 450;
}

void resetPongGame() {
  pongScoreLeft = 0;
  pongScoreRight = 0;
  leftPaddleY = 40;
  rightPaddleY = 40;
  pongGameOver = false;
  resetPongBall(1);
  pongLastUpdateAt = millis();
}

void updatePongPhysics() {
  uint32_t now = millis();
  int catchupSteps = 0;

  while (now - pongLastUpdateAt >= PONG_STEP_MS && catchupSteps < 6) {
    pongLastUpdateAt += PONG_STEP_MS;
    catchupSteps++;

    if (pongGameOver || now < pongServeUntil) continue;

    ballX += ballVx;
    ballY += ballVy;

    if (ballY <= UI_CONTENT_TOP + 2) {
      ballY = UI_CONTENT_TOP + 2;
      ballVy = fabsf(ballVy);
    }
    if (ballY >= UI_CONTENT_BOTTOM - 2) {
      ballY = UI_CONTENT_BOTTOM - 2;
      ballVy = -fabsf(ballVy);
    }

    float collisionReach = max(4.0f, fabsf(ballVx));
    if (ballVx < 0 && ballX <= PONG_LEFT_X + collisionReach && ballX >= PONG_LEFT_X - collisionReach &&
        ballY >= leftPaddleY - PONG_PADDLE_HALF && ballY <= leftPaddleY + PONG_PADDLE_HALF) {
      float hit = (ballY - leftPaddleY) / (float)PONG_PADDLE_HALF;
      ballX = PONG_LEFT_X + 4;
      ballVx = fabsf(ballVx) + 0.10f * PONG_SPEED_MULTIPLIER;
      ballVy = hit * 1.85f * PONG_SPEED_MULTIPLIER;
    }
    if (ballVx > 0 && ballX >= PONG_RIGHT_X - collisionReach && ballX <= PONG_RIGHT_X + collisionReach &&
        ballY >= rightPaddleY - PONG_PADDLE_HALF && ballY <= rightPaddleY + PONG_PADDLE_HALF) {
      float hit = (ballY - rightPaddleY) / (float)PONG_PADDLE_HALF;
      ballX = PONG_RIGHT_X - 4;
      ballVx = -fabsf(ballVx) - 0.10f * PONG_SPEED_MULTIPLIER;
      ballVy = hit * 1.85f * PONG_SPEED_MULTIPLIER;
    }

    if (ballX < 0) {
      pongScoreRight++;
      if (pongScoreRight >= 5) pongGameOver = true;
      else resetPongBall(1);
    } else if (ballX > PONG_WORLD_WIDTH - 1) {
      pongScoreLeft++;
      if (pongScoreLeft >= 5) pongGameOver = true;
      else resetPongBall(-1);
    }

    ballVx = constrainFloat(ballVx, -PONG_MAX_X_SPEED, PONG_MAX_X_SPEED);
    ballVy = constrainFloat(ballVy, -PONG_MAX_Y_SPEED, PONG_MAX_Y_SPEED);
  }

  if (catchupSteps > 0) redrawRequested = true;
}

void drawPongPanel() {
  oled.clearDisplay();
  oled.fillRect(0, 0, 128, 16, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(2, 4); oled.print("PONG "); oled.print(activeDisplay + 1);
  oled.setCursor(52, 4); oled.print(pongScoreLeft);
  oled.setCursor(64, 4); oled.print("-");
  oled.setCursor(76, 4); oled.print(pongScoreRight);
  oled.setCursor(101, 4); oled.print("5PT");
  oled.setTextColor(SSD1306_WHITE);

  if (pongGameOver) {
    if (activeDisplay == 1 || activeDisplay == 2) {
      centerText("GAME OVER", 25, 1);
      centerText(pongScoreLeft > pongScoreRight ? "LEFT WINS" : "RIGHT WINS", 39, 1);
    }
    if (activeDisplay == 0) centerText("CLICK E1 RESET", 54, 1);
    if (activeDisplay == 3) centerText("CLICK E4 RESET", 54, 1);
    return;
  }

  int worldLeft = activeDisplay * SCREEN_WIDTH;
  int localBallX = (int)ballX - worldLeft;
  int localLeftX = PONG_LEFT_X - worldLeft;
  int localRightX = PONG_RIGHT_X - worldLeft;

  oled.drawFastHLine(0, UI_HEADER_HEIGHT, SCREEN_WIDTH, SSD1306_WHITE);
  oled.drawFastHLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, SSD1306_WHITE);
  if (activeDisplay == 0) oled.drawFastVLine(0, UI_HEADER_HEIGHT, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);
  if (activeDisplay == 3) oled.drawFastVLine(127, UI_HEADER_HEIGHT, SCREEN_HEIGHT - UI_HEADER_HEIGHT, SSD1306_WHITE);

  if (localLeftX >= -2 && localLeftX < SCREEN_WIDTH) {
    int y1 = constrain(leftPaddleY - PONG_PADDLE_HALF, UI_CONTENT_TOP + 1, UI_CONTENT_BOTTOM - 1);
    int y2 = constrain(leftPaddleY + PONG_PADDLE_HALF, UI_CONTENT_TOP + 1, UI_CONTENT_BOTTOM - 1);
    oled.drawFastVLine(localLeftX, y1, y2 - y1, SSD1306_WHITE);
    oled.drawFastVLine(localLeftX + 1, y1, y2 - y1, SSD1306_WHITE);
  }
  if (localRightX >= -2 && localRightX < SCREEN_WIDTH) {
    int y1 = constrain(rightPaddleY - PONG_PADDLE_HALF, UI_CONTENT_TOP + 1, UI_CONTENT_BOTTOM - 1);
    int y2 = constrain(rightPaddleY + PONG_PADDLE_HALF, UI_CONTENT_TOP + 1, UI_CONTENT_BOTTOM - 1);
    oled.drawFastVLine(localRightX, y1, y2 - y1, SSD1306_WHITE);
    oled.drawFastVLine(localRightX + 1, y1, y2 - y1, SSD1306_WHITE);
  }
  if (localBallX >= -2 && localBallX <= SCREEN_WIDTH + 2) {
    oled.fillCircle(localBallX, (int)ballY, 2, SSD1306_WHITE);
  }
}

void drawSynthControlPanel() {
  if (activeDisplay == 3) {
    drawStatusFrame("PAGE MENU");

    uint8_t count = 0;
    const uint8_t* pages = getContextPages(count);
    for (uint8_t i = 0; i < count; i++) {
      int y = 18 + i * 12;
      bool selected = synthPage == pages[i];
      if (selected) {
        oled.fillRect(0, y - 1, SCREEN_WIDTH, 11, SSD1306_WHITE);
        oled.setTextColor(SSD1306_BLACK);
      } else {
        oled.setTextColor(SSD1306_WHITE);
      }
      oled.setCursor(4, y + 1);
      oled.print(ControlSurfaceBridge::pageName(pages[i]));
    }
    oled.setTextColor(SSD1306_WHITE);
    return;
  }

  if (ControlSurfaceBridge::isOscillatorSummaryPage(synthPage)) {
    char title[24];
    ControlSurfaceBridge::formatOscillatorSummaryTitle(title, sizeof(title));
    drawStatusFrame(title);

    for (uint8_t line = 0; line < 6; line++) {
      int y = 17 + line * 8;
      bool editableRow = line == activeDisplay + 1;
      if (editableRow) {
        oled.fillRect(0, y - 1, SCREEN_WIDTH, 8, SSD1306_WHITE);
        oled.setTextColor(SSD1306_BLACK);
      } else {
        oled.setTextColor(SSD1306_WHITE);
      }

      char summaryLine[24];
      ControlSurfaceBridge::formatOscillatorSummaryLine(line, summaryLine, sizeof(summaryLine));
      oled.setCursor(2, y);
      oled.print(summaryLine);
    }
    oled.setTextColor(SSD1306_WHITE);
    return;
  }

  char value[24];
  ControlSurfaceBridge::formatParameterValue(synthPage, activeDisplay, value, sizeof(value));
  drawSynthParameterCard(
    ControlSurfaceBridge::pageName(synthPage),
    ControlSurfaceBridge::parameterLabel(synthPage, activeDisplay),
    value,
    activeDisplay
  );
}

void drawDiagnosticsPanel() {
  if (activeDisplay == 0) {
    drawStatusFrame("I2C MAIN");
    oled.setCursor(0, 20);
    oled.print("PCA 0x"); oled.print(pcaAddress, HEX); oled.println(pcaReady ? " OK" : " FAIL");
    oled.print("ACK 0x"); oled.print(oledDetectedMask, HEX);
    oled.print(" INIT 0x"); oled.println(oledReadyMask, HEX);
    oled.println("SDA D21 / SCL D22");
    oled.print("KEY CH4 ");
    oled.print((neokeyDetectedMask & 0x01) ? "30+" : "30-");
    oled.print(' ');
    oled.println((neokeyDetectedMask & 0x02) ? "31+" : "31-");
    oled.print("       ");
    oled.print((neokeyDetectedMask & 0x04) ? "32+" : "32-");
    oled.print(' ');
    oled.println((neokeyDetectedMask & 0x08) ? "33+" : "33-");
  } else if (activeDisplay == 1) {
    drawStatusFrame("ENCODER BUS");
    oled.setCursor(0, 20);
    oled.print("MCP 0x"); oled.print(SURFACE_MCP23017_ADDRESS, HEX); oled.println(encoderExpanderReady ? " OK" : " FAIL");
    oled.print("READ "); oled.println((uint32_t)encoderReadCount);
    oled.print("ERR  "); oled.println((uint32_t)encoderReadErrors);
    oled.println("SDA D19 / SCL D15");
  } else if (activeDisplay == 2) {
    drawStatusFrame("ENCODER VALUES");
    oled.setCursor(0, 20);
    oled.print("E1 "); oled.print(getEncoderValue(0)); oled.print("  E2 "); oled.println(getEncoderValue(1));
    oled.print("E3 "); oled.print(getEncoderValue(2)); oled.print("  E4 "); oled.println(getEncoderValue(3));
    oled.print("Serial: "); oled.println(isSerialDiagnosticsEnabled() ? "ON" : "OFF");
    oled.println("Utility > Serial");
  } else {
    drawStatusFrame("QUICK TEST");
    oled.setCursor(0, 20);
    oled.println("TURN ALL ENCODERS");
    oled.println("PUSH E1/E2/E3");
    oled.println("PUSH E4: MENU");
    oled.println("Utility diagnostics");
  }
}

void drawCurrentPanel() {
  if (startupUntil != 0) drawSplashPanel();
  else if (menuOpen) drawMenuPanel();
  else if (currentMode == SYNTH_CONTROL) drawSynthControlPanel();
  else if (currentMode == ENCODER_VIEW) drawEncoderPanel();
  else if (currentMode == WAVEFORM) drawWaveformPanel();
  else if (currentMode == PONG) drawPongPanel();
  else if (currentMode == DIAGNOSTICS) drawDiagnosticsPanel();
  else drawSplashPanel();
}

void drawAllDisplays() {
  if (!oledBusReady) return;

  for (uint8_t displayIndex = 0; displayIndex < DISPLAY_COUNT; displayIndex++) {
    if ((oledReadyMask & (1 << displayIndex)) == 0) continue;
    setDisplay(displayIndex);
    drawCurrentPanel();
    oled.display();
  }

  setDisplay(0);
  lastDrawAt = millis();
  redrawRequested = false;
}

void preparePongTracking(const InputFrame* input = nullptr) {
  pongLastEncoder1 = input ? input->value[0] : getEncoderValue(0);
  pongLastEncoder4 = input ? input->value[3] : getEncoderValue(3);
  pongLastUpdateAt = millis();
}

void handleMenuInput(const InputFrame& input) {
  if (input.delta[3] != 0) {
    menuSelection += input.delta[3];
    while (menuSelection < 0) menuSelection += MODE_COUNT;
    menuSelection %= MODE_COUNT;
    redrawRequested = true;
  }

  if (input.click[3]) {
    setMode((Mode)menuSelection);
    clickFlashUntil[3] = millis() + 180;
  }
}

void handleSynthControlInput(const InputFrame& input) {
  for (uint8_t i = 0; i < 3; i++) {
    if (input.delta[i] != 0) {
      ControlSurfaceBridge::adjustParameter(synthPage, i, input.delta[i]);
      lastTouchedPage = synthPage;
      lastTouchedEncoder = i;
      lastParameterChangeMs = millis();
      redrawRequested = true;
    }
  }

  if (input.delta[3] != 0) {
    stepContextPage(input.delta[3]);
  }

  if (input.click[2]) {
    ControlSurfaceBridge::activatePageAction(synthPage);
    clickFlashUntil[2] = millis() + 180;
    redrawRequested = true;
  }

  if (input.click[3]) {
    menuSelection = currentMode;
    menuOpen = true;
    clickFlashUntil[3] = millis() + 180;
    redrawRequested = true;
  }
}

void handleEncoderViewInput(const InputFrame& input) {
  for (uint8_t i = 0; i < 3; i++) {
    if (input.click[i]) {
      resetEncoderValue(i);
      clickFlashUntil[i] = millis() + 180;
      redrawRequested = true;
    }
  }
  for (uint8_t i = 0; i < 4; i++) {
    if (input.delta[i] != 0) redrawRequested = true;
  }
}

void handleWaveformInput(const InputFrame& input) {
  if (input.delta[0] != 0) {
    legacyParameterA = constrain((int)legacyParameterA + input.delta[0], -50, 50);
    redrawRequested = true;
  }
  if (input.delta[1] != 0) {
    legacyParameterB = constrain((int)legacyParameterB + input.delta[1], -50, 50);
    redrawRequested = true;
  }
  if (input.click[0]) {
    legacyWaveform = (legacyWaveform + 3) % 4;
    clickFlashUntil[0] = millis() + 180;
    redrawRequested = true;
  }
  if (input.click[1]) {
    legacyWaveform = (legacyWaveform + 1) % 4;
    clickFlashUntil[1] = millis() + 180;
    redrawRequested = true;
  }
}

void handlePongInput(const InputFrame& input) {
  int deltaLeft = input.value[0] - pongLastEncoder1;
  int deltaRight = input.value[3] - pongLastEncoder4;
  pongLastEncoder1 = input.value[0];
  pongLastEncoder4 = input.value[3];

  if (deltaLeft != 0) {
    leftPaddleY = constrain(leftPaddleY + deltaLeft * 2, UI_CONTENT_TOP + PONG_PADDLE_HALF, UI_CONTENT_BOTTOM - PONG_PADDLE_HALF);
    redrawRequested = true;
  }
  if (deltaRight != 0) {
    rightPaddleY = constrain(rightPaddleY + deltaRight * 2, UI_CONTENT_TOP + PONG_PADDLE_HALF, UI_CONTENT_BOTTOM - PONG_PADDLE_HALF);
    redrawRequested = true;
  }

  if (pongGameOver && (input.click[0] || input.click[3])) {
    resetPongGame();
    redrawRequested = true;
  }

  updatePongPhysics();
}

}  // namespace

void begin() {
  bool mcpStarted = beginEncoderExpander();
  encoderExpanderReady = mcpStarted;
  snapshotEncoderBus();
  if (isSerialDiagnosticsEnabled()) printMcpDiagnosticReport();
  beginOleds();
  resetPongGame();
  synthPage = ControlSurfaceBridge::PAGE_VOICE_1;
  syncSynthPageToActiveTrack();
  currentMode = SYNTH_CONTROL;
  menuSelection = SYNTH_CONTROL;
  menuOpen = false;
  startupUntil = millis() + STARTUP_SPLASH_MS;
  redrawRequested = true;

  if (oledBusReady) {
    bool savedMenuOpen = menuOpen;
    menuOpen = false;
    Mode savedMode = currentMode;
    currentMode = SPLASH;
    drawAllDisplays();
    currentMode = savedMode;
    menuOpen = savedMenuOpen;
  }

  if (isSerialDiagnosticsEnabled()) {
    serialDebugBegin("CONTROL SURFACE STATUS");
    Serial.print("Control surface PCA: ");
    Serial.print(pcaReady ? "OK" : "MISSING");
    Serial.print(", OLED mask: 0x");
    Serial.print(oledReadyMask, HEX);
    Serial.print(", MCP23017: ");
    Serial.println(encoderExpanderReady ? "OK" : "MISSING");
    serialDebugEnd("CONTROL SURFACE STATUS");
  }
}

void tick() {
  InputFrame input = {};
  consumeInput(input);
  uint32_t now = millis();

  bool skipStartup = false;
  for (uint8_t i = 0; i < 4; i++) skipStartup = skipStartup || input.click[i];
  if (startupUntil != 0 && (skipStartup || now >= startupUntil)) {
    startupUntil = 0;
    redrawRequested = true;
  }

  if (startupUntil != 0) {
    // The four complete 1 KiB framebuffers are already shown by begin().
    // Keep the splash static instead of retransmitting them throughout boot.
    return;
  }

  syncSynthPageToActiveTrack();

  if (menuOpen) {
    handleMenuInput(input);
  } else if (currentMode == SYNTH_CONTROL) {
    handleSynthControlInput(input);
  } else {
    bool pongResetClick = currentMode == PONG && pongGameOver && input.click[3];
    if (input.click[3] && !pongResetClick) {
      menuSelection = currentMode;
      menuOpen = true;
      clickFlashUntil[3] = now + 180;
      redrawRequested = true;
    } else if (currentMode == ENCODER_VIEW) {
      handleEncoderViewInput(input);
    } else if (currentMode == WAVEFORM) {
      handleWaveformInput(input);
    } else if (currentMode == PONG) {
      handlePongInput(input);
    } else if (currentMode == DIAGNOSTICS) {
      for (uint8_t i = 0; i < 3; i++) {
        if (input.click[i]) clickFlashUntil[i] = now + 180;
      }
      if (now - lastDrawAt >= 500) redrawRequested = true;
    }
  }

  if (currentMode == PONG && !menuOpen) updatePongPhysics();

  if (currentMode == SYNTH_CONTROL && !menuOpen) {
    uint32_t hash = ControlSurfaceBridge::stateHash();
    if (hash != lastMainStateHash) {
      lastMainStateHash = hash;
      redrawRequested = true;
    }
  }

  if (currentMode == SPLASH && !menuOpen && now - lastDrawAt >= 250) redrawRequested = true;

  if (redrawRequested && now - lastDrawAt >= DRAW_INTERVAL_MS) drawAllDisplays();
}

bool isReady() {
  return oledBusReady && encoderExpanderReady;
}

bool isPcaReady() {
  return pcaReady;
}

bool isEncoderExpanderReady() {
  return encoderExpanderReady;
}

void runOledDiagnostics() {
  beginOleds();
  if (!isSerialDiagnosticsEnabled()) printOledDiagnosticReport();
  redrawRequested = true;
}

bool isMainSdaHigh() {
  return mainSdaHigh;
}

bool isMainSclHigh() {
  return mainSclHigh;
}

uint8_t getMainBusDeviceCount() {
  return mainBusDeviceCount;
}

bool isMainBusAddressPresent(uint8_t address) {
  return storedMainBusAddressPresent(address);
}

bool isNeoKeyAddressPresent(uint8_t address) {
  if (address < 0x30 || address > 0x33) return false;
  return (neokeyDetectedMask & (1U << (address - 0x30))) != 0;
}

uint8_t getPcaAddress() {
  return pcaAddress;
}

uint8_t getOledDetectedMask() {
  return oledDetectedMask;
}

uint8_t getOledReadyMask() {
  return oledReadyMask;
}

uint8_t getOledChannelAddress(uint8_t channel) {
  if (channel >= DISPLAY_COUNT) return 0;
  return oledChannelAddress[channel];
}

void runMcpDiagnostics() {
  snapshotEncoderBus();

  // Consente di cablare/correggere il MCP dopo il boot e rilanciare il test
  // dal TFT. Se il task esiste gia', snapshotEncoderBus evita accessi I2C
  // concorrenti e usa l'ultima lettura valida dei registri GPIO.
  if (!encoderExpanderReady && storedEncoderBusAddressPresent(SURFACE_MCP23017_ADDRESS)) {
    encoderRawPortsValid = false;
    encoderExpanderReady = beginEncoderExpander();
    snapshotEncoderBus();
  }

  printMcpDiagnosticReport();
  redrawRequested = true;
}

bool isEncoderSdaHigh() {
  return encoderSdaHigh;
}

bool isEncoderSclHigh() {
  return encoderSclHigh;
}

uint8_t getEncoderBusDeviceCount() {
  return encoderBusDeviceCount;
}

bool isEncoderBusAddressPresent(uint8_t address) {
  return storedEncoderBusAddressPresent(address);
}

bool isMcpRawPortsValid() {
  return encoderRawPortsValid;
}

uint16_t getMcpRawPorts() {
  return encoderRawPorts;
}

bool getEncoderPinHigh(uint8_t encoderIndex, uint8_t signalIndex) {
  if (encoderIndex >= 4 || signalIndex >= 3 || !encoderRawPortsValid) return false;
  uint8_t bitIndex = signalIndex == 0 ? ENCODER_PIN_A[encoderIndex]
                   : signalIndex == 1 ? ENCODER_PIN_B[encoderIndex]
                                      : ENCODER_PIN_SW[encoderIndex];
  return readMappedBit(encoderRawPorts, bitIndex) != 0;
}

long getEncoderCount(uint8_t encoderIndex) {
  return getEncoderValue(encoderIndex);
}

Mode getMode() {
  return currentMode;
}

void setMode(Mode mode) {
  if (mode >= MODE_COUNT) mode = SYNTH_CONTROL;
  currentMode = mode;
  menuSelection = mode;
  menuOpen = false;

  if (mode == PONG) {
    if (pongGameOver) resetPongGame();
    preparePongTracking();
  }

  redrawRequested = true;
}

void stepMode(int delta) {
  if (delta == 0) return;
  int next = (int)currentMode + delta;
  while (next < 0) next += MODE_COUNT;
  next %= MODE_COUNT;
  setMode((Mode)next);
}

const char* getModeName() {
  return getModeName(currentMode);
}

const char* getModeName(Mode mode) {
  switch (mode) {
    case SYNTH_CONTROL: return "SYNTH CONTROL";
    case ENCODER_VIEW: return "ENCODER VIEW";
    case WAVEFORM: return "WAVEFORM";
    case PONG: return "PONG";
    case DIAGNOSTICS: return "DIAGNOSTICS";
    case SPLASH: return "PRELAUNCH";
    default: return "CONTROL";
  }
}

uint8_t getSynthPage() {
  syncSynthPageToActiveTrack();
  return synthPage;
}

const char* getSynthPageName() {
  syncSynthPageToActiveTrack();
  return ControlSurfaceBridge::pageName(synthPage);
}

const char* getLastParameterLabel() {
  syncSynthPageToActiveTrack();
  if (lastTouchedPage != synthPage) {
    lastTouchedPage = synthPage;
    lastTouchedEncoder = 0;
  }
  return ControlSurfaceBridge::parameterLabel(lastTouchedPage, lastTouchedEncoder);
}

void formatLastParameterValue(char* output, size_t outputSize) {
  syncSynthPageToActiveTrack();
  if (lastTouchedPage != synthPage) {
    lastTouchedPage = synthPage;
    lastTouchedEncoder = 0;
  }
  ControlSurfaceBridge::formatParameterValue(lastTouchedPage, lastTouchedEncoder, output, outputSize);
}

uint32_t getLastParameterChangeMs() {
  return lastParameterChangeMs;
}

void markDirty() {
  redrawRequested = true;
}

}  // namespace ControlSurface
