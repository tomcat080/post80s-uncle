#!/usr/bin/env python3
"""Build an Apple-styled `logo` partition image (32 MiB) without needing a stock base image.

Format (per tools/mksplash.py, verified against the Poco F1 stock logo.img):
  0x0000-0x3fff  zeros
  0x4000         b"LOGO!!!!" + u32 words: start0, len0, start1, len1, ... (4 KiB blocks)
  each slot      plain 24-bpp BI_RGB bottom-up BMP, 4 KiB aligned

Slots kept: 0 = power-on (Apple logo, full screen), 1 = fastboot (Apple logo + caption),
2..8 = black full-screen so ABL always finds a valid BMP for charging / warning slots.
"""
import os
import struct
import sys

sys.path.insert(0, "/Volumes/TARGET/Bergamota-main/tools")
import mksplash
from bergamota_img import Canvas, crop_alpha, read_png, resize_rgba, write_png

SIZE = 32 * 1024 * 1024
HDR = 0x4000
BLK = 4096
W, H = 1080, 2246
APPLE = "/Users/tom-mbp162/Doubao/chats/2026-11-17/new-chat-2/开机首屏_苹果标志预览.png"


def fit(img, max_w, max_h):
    w, h, _ = img
    s = min(max_w / w, max_h / h)
    return resize_rgba(img, max(1, round(w * s)), max(1, round(h * s)))


def apple_canvas(apple_h=0.17, apple_y=0.45):
    """Power-on screen: black field + white apple, sized like a real iPhone X boot logo
    (about 15-17% of panel height), centred slightly above the vertical middle."""
    c = Canvas(W, H, (0, 0, 0))
    logo = fit(crop_alpha(read_png(APPLE), 4), W * 0.34, H * apple_h)
    x = (W - logo[0]) // 2
    y = int(H * apple_y - logo[1] / 2)
    c.blit(logo, x, y)
    return c


def black_canvas(w=W, h=320):
    return Canvas(w, h, (0, 0, 0))


def main(out_path, preview_dir=None):
    c0 = apple_canvas()                       # slot 0: power-on = black + white apple
    c1 = mksplash.compose_fastboot(W, H)      # slot 1: fastboot = Bergamota tool screen
    blank = black_canvas()
    bmps = [mksplash.rgb_to_bmp(c0.w, c0.h, c0.px),
            mksplash.rgb_to_bmp(c1.w, c1.h, c1.px)]
    bblank = mksplash.rgb_to_bmp(blank.w, blank.h, blank.px)
    bmps += [bblank] * 7                       # slots 2..8
    words, cur = [], HDR // BLK + 1
    for b in bmps:
        need = (len(b) + BLK - 1) // BLK
        words += [cur, need]
        cur += need
    assert cur * BLK <= SIZE, "does not fit in 32 MiB (%d KiB)" % (cur * BLK // 1024)
    out = bytearray(SIZE)
    for i, b in enumerate(bmps):
        off = words[2 * i] * BLK
        out[off:off + len(b)] = b
    out[HDR:HDR + 8] = b"LOGO!!!!"
    struct.pack_into("<%dI" % len(words), out, HDR + 8, *words)
    with open(out_path, "wb") as f:
        f.write(out)
    print("wrote %s (%d bytes, %d slots, used %.2f MiB)" % (
        out_path, len(out), len(bmps), cur * BLK / 1048576))
    mksplash.verify(out_path)
    if preview_dir:
        mksplash.decode(out_path, preview_dir)


if __name__ == "__main__":
    out = sys.argv[1]
    prev = sys.argv[2] if len(sys.argv) > 2 else None
    main(out, prev)
