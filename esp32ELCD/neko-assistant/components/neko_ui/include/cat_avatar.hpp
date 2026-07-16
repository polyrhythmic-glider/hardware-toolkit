#pragma once

#include "cat_state_machine.hpp"
#include "u8g2.h"

namespace neko {

class CatAvatar final {
public:
    static void draw(u8g2_t *canvas, int center_x, int top_y, int scale,
                     CatExpression expression, uint8_t frame = 0);
};

}  // namespace neko
