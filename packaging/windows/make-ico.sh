#!/usr/bin/env bash
# Regenerate LASSIE.ico from packaging/linux/LASSIE.png.
#
# ICO is a small container header followed by one or more embedded images
# (PNG or BMP). We build a multi-resolution .ico holding PNG frames at
# 16/32/48/64/128/256 — modern Windows handles PNG-inside-ICO since Vista,
# and PNG keeps the file small while staying high-res for the Start menu.
#
# Runs anywhere with Python 3 and PIL/Pillow (or, lacking Pillow, falls
# back to writing the source PNG into the .ico with no resampling).
# Replace LASSIE.png with real artwork before bumping a release.

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
src_png="${here}/../linux/LASSIE.png"
out_ico="${here}/LASSIE.ico"

python3 - "$src_png" "$out_ico" <<'PY'
import struct, sys

src, dst = sys.argv[1], sys.argv[2]

try:
    from PIL import Image
    img = Image.open(src).convert("RGBA")
    sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    # Pillow's built-in ICO save handles the header + multi-resolution
    # frame packing for us, and chooses PNG/BMP per frame.
    img.save(dst, format="ICO", sizes=sizes)
    print(f"wrote {dst} with sizes {sizes}")
except ImportError:
    # Fallback: single 256x256 frame using the source PNG verbatim.
    with open(src, "rb") as fh:
        png = fh.read()
    # ICONDIR (6 bytes) + 1 x ICONDIRENTRY (16 bytes), then the PNG payload.
    icondir = struct.pack("<HHH", 0, 1, 1)
    width = 0  # ICO encodes 256 as 0
    height = 0
    direntry = struct.pack(
        "<BBBBHHII",
        width, height, 0, 0, 1, 32,
        len(png),
        6 + 16,  # offset to image data
    )
    with open(dst, "wb") as fh:
        fh.write(icondir + direntry + png)
    print(f"wrote {dst} (single-frame fallback; install Pillow for multi-resolution)")
PY
