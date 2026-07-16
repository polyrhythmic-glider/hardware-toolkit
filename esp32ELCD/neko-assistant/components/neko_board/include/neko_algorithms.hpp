#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace neko::algorithms {

inline uint8_t shtc3_crc8(const uint8_t *data, size_t size) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80U) ? static_cast<uint8_t>((crc << 1) ^ 0x31U)
                                : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

inline uint8_t battery_percent(float v) {
    struct Point { float volts; float percent; };
    static constexpr std::array<Point, 12> curve{{
        {3.20F, 0}, {3.35F, 5}, {3.50F, 10}, {3.60F, 20}, {3.68F, 30},
        {3.74F, 40}, {3.79F, 50}, {3.84F, 60}, {3.90F, 70}, {3.98F, 80},
        {4.08F, 90}, {4.18F, 100},
    }};
    if (v <= curve.front().volts) return 0;
    if (v >= curve.back().volts) return 100;
    float percent = 0;
    for (size_t i = 1; i < curve.size(); ++i) {
        if (v <= curve[i].volts) {
            const float f = (v - curve[i - 1].volts) /
                            (curve[i].volts - curve[i - 1].volts);
            percent = curve[i - 1].percent + f * (curve[i].percent - curve[i - 1].percent);
            break;
        }
    }
    return static_cast<uint8_t>(std::clamp(std::lround(percent / 5.0F) * 5L, 0L, 100L));
}

inline uint8_t to_bcd(int value) {
    return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

inline int from_bcd(uint8_t value) {
    return ((value >> 4) * 10) + (value & 0x0F);
}

}  // namespace neko::algorithms
