#!/usr/bin/env python3
"""Deterministic Concept C Final Maximizer assets: static housing only.

Usage: python tools/gui_assets/render_maximizer.py <output-dir>
Requires Pillow at CMake configure time. No GUI assets in the audio thread.
The renderer never creates synthetic meters, waveform data, needles or LEDs.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter
import json, hashlib, math, sys

OUT=Path(sys.argv[1]) if len(sys.argv)>1 else Path("assets/gui-maximizer")
OUT.mkdir(parents=True,exist_ok=True)

def mask(size,radius):
 m=Image.new("L",size,0)
 ImageDraw.Draw(m).rounded_rectangle((0,0,size[0]-1,size[1]-1),radius,fill=255)
 return m

def gradient(size,top,bottom):
 w,h=size
 result=Image.new("RGBA",size)
 p=result.load()
 for y in range(h):
  t=y/max(1,h-1)
  grain=4.0*math.cos(y*math.pi/17.)
  rgb=tuple(max(0,min(255,round(top[k]*(1-t)+bottom[k]*t+grain))) for k in range(3))+(255,)
  for x in range(w):p[x,y]=rgb
 return result

def screws(d,centers,r=9):
 for x,y in centers:
  d.ellipse((x-r,y-r,x+r,y+r),fill=(29,39,48,255),outline=(215,229,226,230),width=2)
  d.ellipse((x-r+3,y-r+3,x+r-3,y+r-3),fill=(129,149,159,255))
  d.line((x-r*.5,y+r*.45,x+r*.5,y-r*.45),fill=(37,48,52,255),width=3)

def meter():
 w,h=512,396
 im=Image.new("RGBA",(w,h))
 shadow=Image.new("RGBA",(w,h))
 ImageDraw.Draw(shadow).rounded_rectangle((20,20,w-18,h-17),27,fill=(0,0,0,165))
 im.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(14)))
 steel=gradient((w-38,h-35),(194,204,207),(74,91,102))
 steel.putalpha(mask(steel.size,29))
 im.alpha_composite(steel,(18,16))
 d=ImageDraw.Draw(im)
 d.rounded_rectangle((20,18,w-23,h-22),27,outline=(239,246,245,210),width=3)
 d.rounded_rectangle((27,25,w-30,h-29),22,outline=(21,34,45,220),width=5)
 d.rounded_rectangle((39,38,w-42,h-45),17,fill=(18,29,38,255),outline=(13,21,28,255),width=7)
 face=gradient((w-105,h-118),(184,176,151),(122,124,112))
 face.putalpha(mask(face.size,12))
 im.alpha_composite(face,(53,53))
 d=ImageDraw.Draw(im)
 for y in range(61,h-75,4):d.line((58,y,w-59,y),fill=(48,63,67,24),width=1)
 d.rounded_rectangle((52,52,w-54,h-65),12,outline=(217,211,179,135),width=2)
 screws(d,[(32,30),(w-34,30),(32,h-34),(w-34,h-34)],8)
 return im

def scope():
 w,h=1600,470
 im=Image.new("RGBA",(w,h))
 shadow=Image.new("RGBA",(w,h))
 ImageDraw.Draw(shadow).rounded_rectangle((10,10,w-10,h-10),31,fill=(0,0,0,155))
 im.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(9)))
 d=ImageDraw.Draw(im)
 d.rounded_rectangle((5,5,w-6,h-6),28,outline=(135,159,172,240),width=15)
 d.rounded_rectangle((15,15,w-16,h-16),23,outline=(36,57,70,255),width=10)
 d.rounded_rectangle((26,26,w-27,h-27),19,outline=(0,145,198,150),width=3)
 d.line((72,12,w-72,12),fill=(210,239,240,110),width=3)
 d.line((72,h-14,w-72,h-14),fill=(6,10,16,175),width=3)
 screws(d,[(30,32),(w-31,32),(30,h-33),(w-31,h-33)],10)
 return im

manifest={"generator":"tools/gui_assets/render_maximizer.py","version":1,
 "purpose":"STATIC housing; no audio telemetry, fake waveforms, meters, LEDs or needle",
 "assets":[]}
for name,img in (("maximizer_meter_bezel.png",meter()),
                  ("maximizer_scope_frame.png",scope())):
 path=OUT/name
 img.save(path,optimize=True)
 manifest["assets"].append({"file":name,"width":img.width,"height":img.height,
                            "mode":"RGBA","sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
(OUT/"maximizer_assets_manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf8")
print("Rendered two static Concept C Maximizer assets in",OUT)
