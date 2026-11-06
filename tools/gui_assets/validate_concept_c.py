#!/usr/bin/env python3
"""Validate the deterministic Concept C sprite asset pack before importing to JUCE."""
import json, sys
from pathlib import Path
from PIL import Image

p=Path(sys.argv[1] if len(sys.argv)>1 else "assets/gui-concept-c")
manifest=json.loads((p/"manifest.json").read_text(encoding="utf-8"))
assert manifest["kit"]=="masterlabx-concept-c"
assert len(manifest["assets"])==214, "Expected 202 knob frames + 2 atlases + 6 LEDs + 3 rings + 1 tile"
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
    atlas=next(a for a in manifest["assets"] if a["name"]==f"knob_{size}_atlas.png")
    assert atlas["width"]==size*11 and atlas["height"]==size*10
print("OK: 214 assets, 2x101 knob frames, 6 LED sprites, 1 powdercoat tile; dimensions/RGBA verified.")
