#include "cat_avatar.hpp"

#include <algorithm>

namespace neko {

static void line(u8g2_t *u, int x1, int y1, int x2, int y2, int s) {
    u8g2_DrawLine(u, x1 * s, y1 * s, x2 * s, y2 * s);
}

void CatAvatar::draw(u8g2_t *u, int cx, int top, int scale, CatExpression expression,
                     uint8_t frame) {
    if (!u) return;
    const int s = std::max(1, scale);
    // Work in a compact 54x70 procedural coordinate system.
    const int ox = cx - 27 * s;
    const int oy = top;
    auto X = [=](int x) { return ox + x * s; };
    auto Y = [=](int y) { return oy + y * s; };
    u8g2_DrawTriangle(u, X(8), Y(20), X(13), Y(2), X(23), Y(15));
    u8g2_DrawTriangle(u, X(31), Y(15), X(42), Y(2), X(47), Y(20));
    u8g2_DrawRFrame(u, X(8), Y(13), 39 * s, 37 * s, 10 * s);

    const int blink = (frame % 24 == 0) ? 1 : 0;
    const int eye_y = 28;
    auto eye = [&](int x, bool left) {
        (void)left;
        switch (expression) {
        case CatExpression::Happy:
        case CatExpression::Bluetooth:
            u8g2_DrawArc(u, X(x), Y(eye_y + 2), 4 * s, 32, 96); break;
        case CatExpression::Sleepy:
        case CatExpression::LowBattery:
            u8g2_DrawLine(u, X(x - 4), Y(eye_y + 2), X(x + 4), Y(eye_y + 1)); break;
        case CatExpression::Worried:
        case CatExpression::Error:
            u8g2_DrawLine(u, X(x - 4), Y(eye_y - (left ? 2 : -1)), X(x + 4),
                          Y(eye_y - (left ? -1 : 2)));
            u8g2_DrawDisc(u, X(x), Y(eye_y + 3), 2 * s); break;
        case CatExpression::Listening:
            u8g2_DrawDisc(u, X(x), Y(eye_y), (left ? 3 : 4) * s); break;
        case CatExpression::Thinking:
        case CatExpression::Focused:
            u8g2_DrawLine(u, X(x - 4), Y(eye_y - 2), X(x + 4), Y(eye_y));
            u8g2_DrawDisc(u, X(x), Y(eye_y + 2), 2 * s); break;
        case CatExpression::Hot:
            u8g2_DrawCircle(u, X(x), Y(eye_y), 3 * s); break;
        case CatExpression::Humid:
            u8g2_DrawDisc(u, X(x), Y(eye_y + 1), 2 * s); break;
        default:
            if (blink) u8g2_DrawHLine(u, X(x - 3), Y(eye_y), 7 * s);
            else u8g2_DrawDisc(u, X(x), Y(eye_y), 2 * s);
        }
    };
    eye(20, true); eye(35, false);
    u8g2_DrawTriangle(u, X(25), Y(37), X(29), Y(37), X(27), Y(40));
    if (expression == CatExpression::Alarm || expression == CatExpression::TimerDone) {
        u8g2_DrawCircle(u, X(27), Y(44), 3 * s);
    } else {
        u8g2_DrawArc(u, X(23), Y(40), 4 * s, 0, 64);
        u8g2_DrawArc(u, X(31), Y(40), 4 * s, 64, 128);
    }
    for (int d : {-4, 0, 4}) {
        u8g2_DrawLine(u, X(16), Y(40 + d / 2), X(3), Y(38 + d));
        u8g2_DrawLine(u, X(38), Y(40 + d / 2), X(51), Y(38 + d));
    }
    // Minimal collar and tie: recognisable, formal, entirely procedural.
    u8g2_DrawHLine(u, X(14), Y(52), 27 * s);
    u8g2_DrawTriangle(u, X(24), Y(53), X(30), Y(53), X(27), Y(59));
    u8g2_DrawTriangle(u, X(27), Y(58), X(23), Y(69), X(31), Y(69));
    if (expression == CatExpression::Bluetooth) {
        u8g2_DrawLine(u, X(47), Y(17), X(52), Y(23));
        u8g2_DrawCircle(u, X(52), Y(24), 2 * s);
    }
    if (expression == CatExpression::Hot) {
        u8g2_DrawLine(u, X(2), Y(18), X(5), Y(10));
        u8g2_DrawLine(u, X(52), Y(18), X(49), Y(10));
    }
}

}  // namespace neko
