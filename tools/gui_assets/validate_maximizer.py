#!/usr/bin/env python3
"""Build/CTest validation of real rendered Maximizer static assets."""
from pathlib import Path
from PIL import Image
import json, hashlib, sys
root=Path(sys.argv[1] if len(sys.argv)>1 else "assets/gui-maximizer")
manifest=json.loads((root/"maximizer_assets_manifest.json").read_text(encoding="utf8"))
expected={"maximizer_meter_bezel.png":(512,396),
          "maximizer_scope_frame.png":(1600,470)}
assert len(manifest["assets"])==len(expected), "wrong asset count"
assert "STATIC" in manifest["purpose"], "static renderer purpose not declared"
for entry in manifest["assets"]:
 name=entry["file"]
 assert name in expected, "unexpected asset"
 raw=(root/name).read_bytes()
 assert hashlib.sha256(raw).hexdigest()==entry["sha256"], f"{name} hash mismatch"
 with Image.open(root/name) as source:
  im=source.convert("RGBA")
  assert source.mode=="RGBA" and im.size==expected[name], f"{name} bad image format"
  assert im.getpixel((30,30))[3]>0, f"{name} border missing"
  if name.startswith("maximizer_scope"):
   assert im.getpixel((800,235))[3]==0, "waveform viewport obscured"
  else:
   assert im.getpixel((256,180))[3]>0, "GR meter backplate missing"
print("OK: two deterministic Maximizer static renders, hashes, RGBA and live-waveform aperture")
