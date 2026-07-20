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

// Bus I2C principale: PCA9548A. I 4 OLED usano i canali 0-3; la catena
// La catena 4x NeoKey usa il canale 4.
#define NEOKEY_I2C_SDA  21
#define NEOKEY_I2C_SCL  22

#define SURFACE_PCA9548A_ADDRESS  0x70
#define NEOKEY_PCA_CHANNEL        4
#define SURFACE_DISPLAY_I2C_CLOCK_HZ 400000UL
#define NEOKEY_I2C_CLOCK_HZ          100000UL

// Secondo controller I2C indipendente, dedicato al MCP23017.
#define SURFACE_ENCODER_I2C_SDA  19
#define SURFACE_ENCODER_I2C_SCL  15
#define SURFACE_MCP23017_ADDRESS 0x20

// Uscita I2S verso PCM5102.
#define PCM5102_BCK   27
#define PCM5102_LCK   14
#define PCM5102_DIN   13
#define PCM5102_BCK_LABEL  "D27"
#define PCM5102_LCK_LABEL  "D14"
#define PCM5102_DIN_LABEL  "D13"
