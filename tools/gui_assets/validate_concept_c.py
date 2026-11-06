#!/usr/bin/env python3
"""Validate the deterministic Concept C sprite asset pack before importing to JUCE."""
import json, sys
from pathlib import Path
from PIL import Image

p=Path(sys.argv[1] if len(sys.argv)>1 else "assets/gui-concept-c")
manifest=json.loads((p/"manifest.json").read_text(encoding="utf-8"))
assert manifest["kit"]=="masterlabx-concept-c"
assert len(manifest["assets"])==209, "Expected 202 knob frames + 6 LED states + 1 tile"
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
print("OK: 209 assets, 2x101 knob frames, 6 LED sprites, 1 powdercoat tile; dimensions/RGBA verified.")
