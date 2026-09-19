# Tray Grid brand kit

The identity comes from a glacier lake at dusk — the wallpaper of the desktop Tray Grid was built on, and the palette
Material You drew from it: deep navy surfaces, ice-blue icons, one blush-pink arrow. Inter as the typeface.

All assets are rendered from one palette by `tools/brand.py`; edit the palette there, never the exported files.

```bash
python3 tools/brand.py          # needs rsvg-convert (librsvg)
```

## Logo

| File | Use |
| --- | --- |
| `logo.svg` | 22 px and up; the widget icon and README header |
| `logo-16.svg` | 16 px only: the 3×2 grid blurs at this size, so it becomes 2×2 with a heavier arrow |
| `logo-256.png`, `logo-512.png`, `logo-1024.png` | Store listings, slides, social |

- **The idea:** the popup's icon grid on top, the panel's arrow button underneath. It is the widget, drawn.
- Rounded navy tile; six ice-blue squares fading from bright to dim, left to right and top to bottom; the arrow in blush pink.
- The arrow is the only coloured element. Do not recolour the squares, rotate, stretch, add shadows or gradients.
- Leave clear space of one eighth of the logo's width around it.

## Colours

| Name | Value | Role |
| --- | --- | --- |
| Navy | `#05182F` | Logo tile, panel strip |
| Deep | `#040F1E` | Page background of images |
| Card | `#0A2744` | Popup surface in illustrations |
| Ice | `#ADD5FF` | Grid squares, highlights |
| Frost | `#D8E6FF` | Text on dark |
| Mist | `#95ACD2` | Secondary text |
| Blush | `#E1BCD1` | The single warm accent: the arrow, links |

Inside the widget no colour is hard-coded: it follows the user's Plasma colour scheme.

## Type

- **Inter** (SIL Open Font License 1.1). Headlines 700 with slight negative tracking, body 400.
- The widget itself ships no fonts; it uses the system font.

## Voice

Short, concrete, from the user's side of the screen. Main line: **"Your hidden tray icons, as a grid."**
Say what it does before how it works; no "revolutionary", no "seamless".

## Images

`docs/social-preview.png` is the GitHub social preview (1280×640). Screenshots of the real widget live in `docs/screenshots/`.
