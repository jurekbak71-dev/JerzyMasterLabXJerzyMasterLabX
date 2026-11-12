#!/usr/bin/env python3
"""Validate the deterministic Concept C sprite asset pack before importing to JUCE."""
import json, sys
from pathlib import Path
from PIL import Image

p=Path(sys.argv[1] if len(sys.argv)>1 else "assets/gui-concept-c")
manifest=json.loads((p/"manifest.json").read_text(encoding="utf-8"))
assert manifest["kit"]=="masterlabx-concept-c"
assert len(manifest["assets"])==235, "Expected Concept C pack plus 10 separate Final Maximizer assets"
seen=set()
for a in manifest["assets"]:
    name=a["name"]
    assert name not in seen, "Duplicate asset "+name
    seen.add(name)
    path=p/name
    assert path.is_file(), name
    with Image.open(path) as im:
        assert im.mode=="RGBA", (name, im.mode)
        assert im.size==(a["width"],a["height"]), name
        if a["category"]=="knob_frame":
            assert im.getpixel((0,0))[3]==0, name+" needs transparent corners"
for size in (96,160):
    frames=sorted(a["frame"] for a in manifest["assets"]
                  if a["category"]=="knob_frame" and a["width"]==size)
    assert frames==list(range(101)), f"Missing size {size} or frame"
for size in (96,160):
    light=next(a for a in manifest["assets"] if a["name"]==f"led_wreath_{size}_atlas.png")
    assert light["width"]==size*11 and light["height"]==size*10 and light["frames"]==101
for size in (96,160):
    atlas=next(a for a in manifest["assets"] if a["name"]==f"knob_{size}_atlas.png")
    assert atlas["width"]==size*11 and atlas["height"]==size*10
# Regression: the prior faux-glass overlays contained opaque-looking white
# triangles. Reflections must be alpha-only; dark substrate must stay dark.
for name in ("glass_overlay.png","glass_readout_overlay.png"):
    with Image.open(p/name) as im:
        assert im.getchannel("A").getextrema()[1]<=65, (
            name,"glass glare can never obscure actual LCD telemetry")
for name in ("glass_analyzer.png","glass_readout.png"):
    with Image.open(p/name) as im:
        red,green,blue,alpha=im.getpixel((im.width//2,im.height//2))
        assert alpha==255 and red<30 and green<40 and blue<65, (
            name,"LCD substrate must be dark, not white painted glass")
for size in (96,160):
    with Image.open(p/f"led_wreath_{size}_atlas.png") as img:
        def count_lit(frame):
            ox=(frame%11)*size
            oy=(frame//11)*size
            crop=img.crop((ox,oy,ox+size,oy+size))
            return sum(1 for red,green,blue,alpha in crop.getdata()
                       if red<150 and green>155 and blue>185 and alpha>75)
        n0,n50,n100=(count_lit(f) for f in (0,50,100))
        assert n0==0 and n50>20 and n100>n50*1.35, (
            "Wreath arc fails to increase with normalized APVTS value",size,n0,n50,n100)
for name,size in {
    "maximizer_fader_cap_vertical.png":(44,20),
    "maximizer_fader_cap_vertical_2x.png":(88,40),
    "maximizer_fader_cap_horizontal.png":(20,44),
    "maximizer_fader_cap_horizontal_2x.png":(40,88),
    "maximizer_power_on.png":(160,42),"maximizer_power_off.png":(160,42),
    "maximizer_selector_on.png":(104,34),"maximizer_selector_off.png":(104,34),
    "maximizer_display_bezel.png":(720,260),"maximizer_readout_bezel.png":(220,58),
}.items():
    with Image.open(p/name) as im: assert im.mode=="RGBA" and im.size==size,name
print("OK: 235 assets including 10 separate Final Maximizer surfaces; dimensions/RGBA verified.")
