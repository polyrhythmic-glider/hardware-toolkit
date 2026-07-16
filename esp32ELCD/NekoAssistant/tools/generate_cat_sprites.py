"""Generate the deterministic 1-bit Neko sprite sheet used by the firmware."""

from pathlib import Path
import struct
import zlib

W, H = 96, 88
NAMES = ("idle", "happy", "sleepy", "alert")


def canvas():
    return [[0 for _ in range(W)] for _ in range(H)]


def pixel(img, x, y, value=1):
    if 0 <= x < W and 0 <= y < H:
        img[y][x] = value


def line(img, x0, y0, x1, y1, value=1, width=1):
    dx, sx = abs(x1 - x0), 1 if x0 < x1 else -1
    dy, sy = -abs(y1 - y0), 1 if y0 < y1 else -1
    error = dx + dy
    while True:
        radius = width // 2
        for yy in range(y0 - radius, y0 + radius + 1):
            for xx in range(x0 - radius, x0 + radius + 1):
                pixel(img, xx, yy, value)
        if x0 == x1 and y0 == y1:
            break
        twice = 2 * error
        if twice >= dy:
            error += dy
            x0 += sx
        if twice <= dx:
            error += dx
            y0 += sy


def ellipse(img, cx, cy, rx, ry, value=1, width=1):
    # Dense parametric outline is reliable at this small sprite size.
    import math
    points = []
    for degree in range(361):
        angle = math.radians(degree)
        points.append((round(cx + rx * math.cos(angle)), round(cy + ry * math.sin(angle))))
    for a, b in zip(points, points[1:]):
        line(img, *a, *b, value=value, width=width)


def disc(img, cx, cy, radius, value=1):
    for y in range(cy - radius, cy + radius + 1):
        for x in range(cx - radius, cx + radius + 1):
            if (x - cx) ** 2 + (y - cy) ** 2 <= radius ** 2:
                pixel(img, x, y, value)


def polygon(img, points, value=1):
    min_y = max(0, min(y for _, y in points))
    max_y = min(H - 1, max(y for _, y in points))
    for y in range(min_y, max_y + 1):
        intersections = []
        for i, (x1, y1) in enumerate(points):
            x2, y2 = points[(i + 1) % len(points)]
            if y1 == y2 or y < min(y1, y2) or y >= max(y1, y2):
                continue
            intersections.append(round(x1 + (y - y1) * (x2 - x1) / (y2 - y1)))
        intersections.sort()
        for left, right in zip(intersections[0::2], intersections[1::2]):
            for x in range(left, right + 1):
                pixel(img, x, y, value)


def base_sprite(mood):
    img = canvas()

    # Ears: heavy outer silhouette and clean inner cut-out.
    polygon(img, [(12, 31), (17, 5), (37, 23)], 1)
    polygon(img, [(84, 31), (79, 5), (59, 23)], 1)
    polygon(img, [(18, 20), (20, 10), (30, 22)], 0)
    polygon(img, [(78, 20), (76, 10), (66, 22)], 0)
    line(img, 19, 14, 29, 23, width=2)
    line(img, 77, 14, 67, 23, width=2)

    # Rounded face outline with short crown tufts.
    ellipse(img, 48, 43, 35, 29, width=3)
    polygon(img, [(38, 17), (42, 11), (46, 18), (50, 10), (54, 18), (59, 13), (61, 20)], 1)
    polygon(img, [(40, 19), (43, 15), (46, 21), (50, 14), (54, 21), (58, 17), (59, 22)], 0)

    # Forehead stripes and cheek dots.
    line(img, 38, 23, 40, 29, width=2)
    line(img, 48, 21, 48, 28, width=2)
    line(img, 58, 23, 56, 29, width=2)
    for x, y in ((27, 51), (31, 54), (69, 54), (73, 51)):
        disc(img, x, y, 1)

    # Eyes are the primary expression layer.
    if mood == "sleepy":
        line(img, 27, 39, 36, 41, width=2)
        line(img, 60, 41, 69, 39, width=2)
    elif mood == "happy":
        line(img, 27, 42, 31, 38, width=2)
        line(img, 31, 38, 36, 42, width=2)
        line(img, 60, 42, 65, 38, width=2)
        line(img, 65, 38, 69, 42, width=2)
    elif mood == "alert":
        ellipse(img, 32, 40, 6, 7, width=2)
        ellipse(img, 64, 40, 6, 7, width=2)
        disc(img, 32, 40, 3)
        disc(img, 64, 40, 3)
        pixel(img, 31, 38, 0); pixel(img, 63, 38, 0)
    else:
        ellipse(img, 32, 40, 6, 5, width=2)
        ellipse(img, 64, 40, 6, 5, width=2)
        disc(img, 32, 40, 2)
        disc(img, 64, 40, 2)

    # Nose, muzzle and mood-specific mouth.
    polygon(img, [(48, 48), (44, 46), (52, 46)], 1)
    line(img, 48, 48, 48, 52, width=1)
    if mood == "happy":
        ellipse(img, 48, 52, 10, 7, width=2)
        for y in range(45, 52):
            for x in range(36, 60):
                pixel(img, x, y, 0)
        disc(img, 48, 56, 3)
    elif mood == "alert":
        ellipse(img, 48, 57, 4, 6, width=2)
    elif mood == "sleepy":
        line(img, 42, 55, 54, 55, width=2)
    else:
        line(img, 48, 52, 42, 57, width=2)
        line(img, 48, 52, 54, 57, width=2)

    # Long whiskers, then a small tuxedo bib and papillon.
    for y, offset in ((48, -3), (54, 0), (60, 3)):
        line(img, 39, y, 5, y + offset, width=1)
        line(img, 57, y, 91, y + offset, width=1)
    line(img, 33, 68, 39, 78, width=2)
    line(img, 63, 68, 57, 78, width=2)
    polygon(img, [(47, 73), (34, 68), (34, 82), (47, 77)], 1)
    polygon(img, [(49, 73), (62, 68), (62, 82), (49, 77)], 1)
    disc(img, 48, 75, 4, 1)
    polygon(img, [(43, 84), (48, 78), (53, 84)], 1)
    return img


def xbm_bytes(img):
    output = []
    for row in img:
        for x in range(0, W, 8):
            byte = 0
            for bit in range(8):
                if row[x + bit]:
                    byte |= 1 << bit
            output.append(byte)
    return output


def write_header(path, sprites):
    lines = ["#pragma once", "", "#include <Arduino.h>", "",
             f"constexpr uint8_t NEKO_SPRITE_WIDTH = {W};",
             f"constexpr uint8_t NEKO_SPRITE_HEIGHT = {H};", ""]
    for name, image in sprites.items():
        values = xbm_bytes(image)
        lines.append(f"static const uint8_t NEKO_CAT_{name.upper()}[] PROGMEM = {{")
        for start in range(0, len(values), 16):
            chunk = ", ".join(f"0x{value:02X}" for value in values[start:start + 16])
            lines.append(f"  {chunk},")
        lines.extend(["};", ""])
    path.write_text("\n".join(lines), encoding="utf-8")


def png_chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def write_preview(path, sprites):
    scale, gap = 3, 12
    width = len(sprites) * W * scale + (len(sprites) + 1) * gap
    height = H * scale + gap * 2
    rows = []
    for y in range(height):
        row = bytearray([255] * width)
        for index, image in enumerate(sprites.values()):
            left = gap + index * (W * scale + gap)
            for x in range(W * scale):
                sy = (y - gap) // scale
                if 0 <= sy < H and image[sy][x // scale]:
                    row[left + x] = 0
        rows.append(b"\x00" + bytes(row))
    data = b"\x89PNG\r\n\x1a\n"
    data += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0))
    data += png_chunk(b"IDAT", zlib.compress(b"".join(rows), 9))
    data += png_chunk(b"IEND", b"")
    path.write_bytes(data)


if __name__ == "__main__":
    root = Path(__file__).resolve().parent.parent
    sprites = {name: base_sprite(name) for name in NAMES}
    write_header(root / "NekoCatSprites.h", sprites)
    write_preview(root / "cat-sprites-preview.png", sprites)
