#!/usr/bin/env python3
"""Places the rendered grid (from gridtest --preview) above a Plasma-like panel for the README.

The panel is drawn the way Plasma draws it with the Buzul colour scheme: Breeze icons tinted with the
scheme's text colour, the widget's own arrow (pointing down while open) and the "expanded" line above it.
No wallpaper: a plain gradient, so no third-party image ends up in the repository.

Usage: readme-compose.py GRID.png OUTPUT.png [SCALE]
Needs rsvg-convert and Pillow.
"""
import os, re, subprocess, sys, tempfile
from PIL import Image, ImageDraw, ImageFilter, ImageFont

grid_png, out_png = sys.argv[1], sys.argv[2]
S = int(sys.argv[3]) if len(sys.argv) > 3 else 2
W, H, P = 760, 380, 44                       # logical size, panel height
PANEL, TEXT, CARD, EDGE = (5, 24, 47), "#95acd2", (5, 24, 47), (40, 70, 105)
BREEZE = "/usr/share/icons/breeze-dark"

def icon(path, size):
    svg = open(path).read()
    svg = re.sub(r"#fcfcfc|#FCFCFC", TEXT, svg)          # ColorScheme-Text → scheme text colour
    with tempfile.NamedTemporaryFile("w", suffix=".svg", delete=False) as f:
        f.write(svg)
    png = f.name + ".png"
    subprocess.run(["rsvg-convert", "-w", str(size * S), "-h", str(size * S), f.name, "-o", png], check=True)
    img = Image.open(png).convert("RGBA"); os.remove(f.name); os.remove(png)
    return img

# background: soft navy gradient
im = Image.new("RGBA", (W * S, H * S))
d = ImageDraw.Draw(im)
for y in range(H * S):
    t = y / (H * S)
    d.line([(0, y), (W * S, y)], fill=(int(14 + 10 * t), int(38 + 18 * t), int(68 + 26 * t), 255))

# panel, attached to the bottom edge
top = (H - P) * S
d.rectangle((0, top, W * S, H * S), fill=PANEL)
font = ImageFont.truetype("/usr/share/fonts/noto/NotoSans-Regular.ttf", 13 * S)
x = W * S - 16 * S
d.text((x, top + P * S // 2), "7:23 PM", font=font, fill=TEXT, anchor="rm")
x -= 70 * S
for name in ["status/22/audio-volume-high", "status/22/network-wired-activated",
             "actions/22/edit-paste", "actions/22/notifications"]:
    ic = icon(f"{BREEZE}/{name}.svg", 22)
    im.paste(ic, (x - ic.width, top + (P * S - ic.height) // 2), ic)
    x -= ic.width + 12 * S
arrow = icon(f"{BREEZE}/actions/22/arrow-down-symbolic.svg", 22)
ax = x - arrow.width
im.paste(arrow, (ax, top + (P * S - arrow.height) // 2), arrow)
d.rectangle((ax - 4 * S, top, ax + arrow.width + 4 * S, top + 2 * S), fill=TEXT)   # "expanded" line
arrow_cx = ax + arrow.width // 2

# floating popup above the arrow
g = Image.open(grid_png).convert("RGB")
pad = 10 * S
cw, ch = g.width + 2 * pad, g.height + 2 * pad
px = min(arrow_cx - cw // 2, W * S - cw - 10 * S)
py = top - ch - 10 * S
shadow = Image.new("RGBA", im.size, (0, 0, 0, 0))
ImageDraw.Draw(shadow).rounded_rectangle((px + 4, py + 8, px + cw + 4, py + ch + 8), 10 * S, fill=(0, 0, 0, 120))
im = Image.alpha_composite(im, shadow.filter(ImageFilter.GaussianBlur(10 * S)))
d = ImageDraw.Draw(im)
d.rounded_rectangle((px, py, px + cw, py + ch), 9 * S, fill=CARD, outline=EDGE, width=S)
im.paste(g, (px + pad, py + pad))
im.convert("RGB").save(out_png, optimize=True)
print(out_png)
