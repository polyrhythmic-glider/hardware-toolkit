#pragma once

// Display ST7735.
#define TFT_CS    5
#define TFT_DC    4
#define TFT_RST   2
#define TFT_SCK   18
#define TFT_MOSI  23

// Encoder e pulsante.
#define ENC_A     32
#define ENC_B     33
#define ENC_SW    25
#define BTN_K0    26

// Bus I2C principale: PCA9548A. I 4 OLED usano i canali 0-3;
// la catena 4x NeoKey usa il canale 4.
#define NEOKEY_I2C_SDA  21
#define NEOKEY_I2C_SCL  22

#define SURFACE_PCA9548A_ADDRESS  0x70
#define NEOKEY_PCA_CHANNEL        4
#define SURFACE_DISPLAY_I2C_CLOCK_HZ 400000UL
#define NEOKEY_I2C_CLOCK_HZ          100000UL

// Tipo dei quattro OLED dietro al PCA9548A. Un bit a 1 indica SH1106;
// un bit a 0 indica SSD1306. Configurazione attuale:
// CH0, CH1, CH2 = AZDelivery 1.3" SH1106; CH3 = OLED piccolo SSD1306.
#define SURFACE_OLED_SH1106_MASK 0x07

// Secondo controller I2C indipendente, dedicato al MCP23017.
#define SURFACE_ENCODER_I2C_SDA  19
#define SURFACE_ENCODER_I2C_SCL  15
#define SURFACE_MCP23017_ADDRESS 0x20

// Uscita I2S opzionale verso il vecchio PCM5102. Questi pin sono liberi con
// il backend Daisy predefinito e possono restare fisicamente scollegati.
#define PCM5102_BCK   27
#define PCM5102_LCK   14
#define PCM5102_DIN   13
#define PCM5102_BCK_LABEL  "D27"
#define PCM5102_LCK_LABEL  "D14"
#define PCM5102_DIN_LABEL  "D13"

// Collegamento seriale 3,3 V verso la Daisy Seed. I nomi RX2/TX2 sono quelli
// stampati sulla ESP32 DevKit; RX e TX vanno incrociati tra le due schede.
#define DAISY_LINK_RX   16  // RX2 <- Daisy D13 / pin fisico 14 (USART1 TX)
#define DAISY_LINK_TX   17  // TX2 -> Daisy D14 / pin fisico 15 (USART1 RX)
