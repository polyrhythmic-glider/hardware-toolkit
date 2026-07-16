#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"

namespace neko::board {

inline constexpr char kHardwareProfile[] = "WS_RLCD42_R1";
inline constexpr int kPhysicalWidth = 300;
inline constexpr int kPhysicalHeight = 400;
inline constexpr int kUiWidth = 400;
inline constexpr int kUiHeight = 300;

// Values below are from the current official Waveshare schematic and examples.
inline constexpr gpio_num_t kDisplaySclk = GPIO_NUM_11;
inline constexpr gpio_num_t kDisplayMosi = GPIO_NUM_12;
inline constexpr gpio_num_t kDisplayDc = GPIO_NUM_5;
inline constexpr gpio_num_t kDisplayCs = GPIO_NUM_40;
inline constexpr gpio_num_t kDisplayReset = GPIO_NUM_41;
inline constexpr spi_host_device_t kDisplaySpiHost = SPI3_HOST;

inline constexpr gpio_num_t kI2cSda = GPIO_NUM_13;
inline constexpr gpio_num_t kI2cScl = GPIO_NUM_14;
inline constexpr uint8_t kRtcAddress = 0x51;
inline constexpr uint8_t kShtc3Address = 0x70;
inline constexpr uint8_t kEs8311Address = 0x18;
inline constexpr uint8_t kEs7210Address = 0x40;

inline constexpr gpio_num_t kKey = GPIO_NUM_18;
inline constexpr bool kKeyActiveLow = true;
inline constexpr gpio_num_t kBoot = GPIO_NUM_0;

inline constexpr gpio_num_t kBatteryAdcGpio = GPIO_NUM_4;
inline constexpr adc_unit_t kBatteryAdcUnit = ADC_UNIT_1;
inline constexpr adc_channel_t kBatteryAdcChannel = ADC_CHANNEL_3;
inline constexpr float kBatteryDividerRatio = 3.0F;  // 200k high / 100k low

inline constexpr gpio_num_t kSdClk = GPIO_NUM_38;
inline constexpr gpio_num_t kSdCmd = GPIO_NUM_21;
inline constexpr gpio_num_t kSdD0 = GPIO_NUM_39;

inline constexpr gpio_num_t kI2sMclk = GPIO_NUM_16;
inline constexpr gpio_num_t kI2sBclk = GPIO_NUM_9;
inline constexpr gpio_num_t kI2sLrclk = GPIO_NUM_45;
inline constexpr gpio_num_t kI2sDin = GPIO_NUM_10;
inline constexpr gpio_num_t kI2sDout = GPIO_NUM_8;
inline constexpr gpio_num_t kSpeakerEnable = GPIO_NUM_46;

// PWR is part of the discrete power circuit; there is intentionally no PWR GPIO.

}  // namespace neko::board
