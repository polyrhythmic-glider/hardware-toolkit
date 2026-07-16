#include <cassert>
#include <cstdint>
#include "neko_algorithms.hpp"

int main() {
    using namespace neko::algorithms;
    assert(battery_percent(3.10F) == 0);
    assert(battery_percent(3.50F) == 10);
    assert(battery_percent(3.84F) == 60);
    assert(battery_percent(4.20F) == 100);
    for (int tenth = 32; tenth <= 42; ++tenth) {
        assert(battery_percent(tenth / 10.0F) <= battery_percent((tenth + 1) / 10.0F));
    }
    const uint8_t shtc3_example[] = {0x66, 0x66};
    assert(shtc3_crc8(shtc3_example, 2) == 0x93);
    assert(from_bcd(to_bcd(0)) == 0);
    assert(from_bcd(to_bcd(59)) == 59);
    assert(from_bcd(to_bcd(23)) == 23);
    return 0;
}
