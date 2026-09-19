#!/usr/bin/env python3
"""Renders the Tray Grid brand assets into docs/brand/ from one palette.

Usage: python3 tools/brand.py [breeze|buzul|pencere]   (default: buzul)
Needs rsvg-convert (librsvg). Fonts: Noto Sans (breeze) or Inter.
"""
import os, subprocess, sys
YON = {
 "breeze": dict(tile="#232629", kenar=None, kare=["#EFF0F1"]*6, kare_op=[1,.78,.56,.78,.56,.34], ok="#3DAEE9",
                zemin="#1B1E20", yazi="#EFF0F1", soluk="#A1A9B1", vurgu="#3DAEE9", font="Noto Sans", panel="#2A2E32", kart="#31363B"),
 "buzul":  dict(tile="#05182F", kenar=None, kare=["#ADD5FF"]*6, kare_op=[1,.8,.6,.8,.6,.4], ok="#E1BCD1",
                zemin="#040F1E", yazi="#D8E6FF", soluk="#95ACD2", vurgu="#E1BCD1", font="Inter", panel="#05182F", kart="#0A2744"),
 "pencere": dict(tile="#F3F3F3", kenar="#D1D1D1", kare=["#0F6CBD","#107C10","#C239B3","#CA5010","#038387","#8764B8"], kare_op=[1]*6, ok="#1F1F1F",
                zemin="#FAFAFA", yazi="#1F1F1F", soluk="#5F5F5F", vurgu="#0F6CBD", font="Inter", panel="#EBEBEB", kart="#FFFFFF"),
}
def logo(y, boyut=128, ic_only=False):
    t = YON[y]; k = f' stroke="{t["kenar"]}" stroke-width="2"' if t["kenar"] else ""
    s = [f'<rect x="1" y="1" width="126" height="126" rx="28" fill="{t["tile"]}"{k}/>']
    # 3x2 simge ızgarası (açılır pencere) + altta ok (çubuktaki düğme)
    kare, ara = 24, 9; gen = 3*kare + 2*ara; x0 = (128-gen)/2; y0 = 24
    for i in range(6):
        c, r = i % 3, i // 3
        s.append(f'<rect x="{x0+c*(kare+ara):.1f}" y="{y0+r*(kare+ara):.1f}" width="{kare}" height="{kare}" rx="7" fill="{t["kare"][i]}" opacity="{t["kare_op"][i]}"/>')
    s.append(f'<path d="M50 104 L64 91 L78 104" fill="none" stroke="{t["ok"]}" stroke-width="9" stroke-linecap="round" stroke-linejoin="round"/>')
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" width="{boyut}" height="{boyut}">' + "".join(s) + "</svg>"
def sosyal(y):
    t = YON[y]; L = logo(y).replace('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" width="128" height="128">','').replace("</svg>","")
    # altta ekran kenarına yapışık çubuk, sağda üstünde yüzen açılır ızgara
    grid = []
    py = 580  # çubuğun üst kenarı; 640'a kadar iner
    kx, ky, kw, kh = 790, py - 14 - 250, 330, 250
    grid.append(f'<rect x="{kx}" y="{ky}" width="{kw}" height="{kh}" rx="18" fill="{t["kart"]}" stroke="{t["soluk"]}" stroke-opacity=".25"/>')
    for i in range(9):
        c, r = i % 3, i // 3
        renk = t["kare"][i % 6]; op = t["kare_op"][i % 6]
        grid.append(f'<rect x="{kx+45+c*90}" y="{ky+30+r*68}" width="60" height="50" rx="14" fill="{renk}" opacity="{op}"/>')
    grid.append(f'<rect x="0" y="{py}" width="1280" height="{640-py}" fill="{t["panel"]}"/>')
    grid.append(f'<rect x="{kx+kw/2-32}" y="{py+6}" width="64" height="48" rx="10" fill="{t["vurgu"]}" opacity=".18"/>')
    grid.append(f'<path d="M{kx+kw/2-12} {py+36} L{kx+kw/2} {py+24} L{kx+kw/2+12} {py+36}" fill="none" stroke="{t["vurgu"]}" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>')
    for j, x in enumerate([1000, 1044, 1088]):
        grid.append(f'<rect x="{x}" y="{py+18}" width="24" height="24" rx="6" fill="{t["soluk"]}" opacity=".6"/>')
    grid.append(f'<text x="1250" y="{py+37}" font-family="{t["font"]}" font-size="20" fill="{t["yazi"]}" text-anchor="end">7:23 PM</text>')
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1280 640" width="1280" height="640">
<rect width="1280" height="640" fill="{t["zemin"]}"/>
<g transform="translate(96,150) scale(1.35)">{L}</g>
<text x="96" y="400" font-family="{t["font"]}" font-weight="700" font-size="84" fill="{t["yazi"]}" letter-spacing="-2">Tray Grid</text>
<text x="96" y="452" font-family="{t["font"]}" font-size="32" fill="{t["soluk"]}">Your hidden tray icons, as a grid.</text>
<text x="96" y="530" font-family="{t["font"]}" font-weight="600" font-size="22" fill="{t["vurgu"]}">KDE Plasma 6 widget</text>
{"".join(grid)}
</svg>'''
def logo16(y):
    t = YON[y]; k = f' stroke="{t["kenar"]}" stroke-width="1"' if t["kenar"] else ""
    s = [f'<rect x="0.5" y="0.5" width="15" height="15" rx="3.5" fill="{t["tile"]}"{k}/>']
    for i in range(4):  # 16 px'te 3x2 ızgara bulanıyor: 2x2 ve kalın ok
        c, r = i % 2, i // 2
        s.append(f'<rect x="{3.5+c*5}" y="{2.5+r*4.5}" width="4" height="3.5" rx="1" fill="{t["kare"][i]}" opacity="{t["kare_op"][i]}"/>')
    s.append(f'<path d="M5 13.2 L8 10.8 L11 13.2" fill="none" stroke="{t["ok"]}" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>')
    return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" width="16" height="16">' + "".join(s) + "</svg>"

if __name__ == "__main__":
    y = sys.argv[1] if len(sys.argv) > 1 else "buzul"
    kok = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs")
    b = os.path.join(kok, "brand")
    os.makedirs(b, exist_ok=True)
    open(os.path.join(b, "logo.svg"), "w").write(logo(y))
    open(os.path.join(b, "logo-16.svg"), "w").write(logo16(y))
    open(os.path.join(b, "social-preview.svg"), "w").write(sosyal(y))
    for n in (256, 512, 1024):
        subprocess.run(["rsvg-convert", "-w", str(n), "-h", str(n), os.path.join(b, "logo.svg"), "-o", os.path.join(b, f"logo-{n}.png")], check=True)
    subprocess.run(["rsvg-convert", "-w", "1280", "-h", "640", os.path.join(b, "social-preview.svg"), "-o", os.path.join(kok, "social-preview.png")], check=True)
    print(f"brand: {y} -> docs/brand/, docs/social-preview.png")
