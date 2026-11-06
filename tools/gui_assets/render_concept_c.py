#!/usr/bin/env python3
"""Deterministic production starter for MasterLabX Concept C rendered GUI assets.
Requires Pillow (dev-only). Output RGBA PNGs; runtime plugin never needs Pillow.
Usage: python tools/gui_assets/render_concept_c.py [output_dir]
"""
from PIL import Image, ImageDraw, ImageFilter
from pathlib import Path
import math, json, sys

OUT = Path(sys.argv[1] if len(sys.argv)>1 else "assets/gui-concept-c")
OUT.mkdir(parents=True, exist_ok=True)
PALETTE = {
    "powdercoat": "#eef0f1", "silver": "#aeb6bc", "ink": "#17212a",
    "green": "#28f14a", "red": "#ff283c", "blue": "#0677ff",
}
SPEC = {"kit":"masterlabx-concept-c","design_reference":"Concept C / 2026-11-06",
        "version":1,"generator":"tools/gui_assets/render_concept_c.py",
        "assets":[]}
def save(im, name, category, **extra):
    path=OUT/name
    im.save(path)
    SPEC["assets"].append({"name":name,"category":category,"width":im.width,
                           "height":im.height,"rgba":True,**extra})

def knob(size, phase):
    """Locked-pivot pre-rendered metallic knob with changing physical index."""
    S=4; d=size*S; c=d/2; im=Image.new("RGBA",(d,d)); dr=ImageDraw.Draw(im,"RGBA")
    def circle(r, fill, ox=0,oy=0,width=0):
        b=(int(c-r+ox),int(c-r+oy),int(c+r+ox),int(c+r+oy))
        if width: dr.ellipse(b,outline=fill,width=width)
        else: dr.ellipse(b,fill=fill)
    circle(.445*d,(12,19,25,110),oy=8*S)
    circle(.432*d,(24,34,44,255))
    circle(.415*d,(155,163,171,255))
    circle(.398*d,(48,57,65,255))
    # fixed brushed-metal shading and highlight: camera does not rotate
    for k in range(int(.365*d),int(.105*d),-1):
        q=k/(.365*d)
        light=math.cos(q*4.1)
        tone=int(133+42*light)
        circle(k,(tone,tone+4,tone+8,255))
    circle(.29*d,(184,191,197,255),width=max(1,2*S))
    circle(.278*d,(89,100,110,65),width=max(1,2*S))
    # radial grind lines remain static; only indicator rotates
    for a in range(0,360,12):
        v=math.radians(a); r1=.21*d;r2=.27*d
        dr.line((c+math.sin(v)*r1,c-math.cos(v)*r1,
                 c+math.sin(v)*r2,c-math.cos(v)*r2),
                fill=(75,84,93,32),width=S)
    angle=math.radians(-135.0+270.0*phase/100.0)
    r1=.12*d;r2=.305*d
    p1=(c+math.sin(angle)*r1,c-math.cos(angle)*r1)
    p2=(c+math.sin(angle)*r2,c-math.cos(angle)*r2)
    dr.line((*p1,*p2),fill=(15,29,40,255),width=6*S)
    dr.line((*p1,*p2),fill=(241,250,252,255),width=3*S)
    return im.resize((size,size),Image.Resampling.LANCZOS)

def led(size, color, active=True):
    S=4; d=size*S; c=d/2
    im=Image.new("RGBA",(d,d));dr=ImageDraw.Draw(im,"RGBA")
    rgb=tuple(int(color[i:i+2],16) for i in (1,3,5))
    if active:
        for r in range(int(.46*d),int(.2*d),-2*S):
            alpha=int(38*(1-r/(.47*d)))
            dr.ellipse((c-r,c-r,c+r,c+r),outline=(*rgb,alpha),width=5*S)
    r=.19*d
    dr.ellipse((c-r,c-r,c+r,c+r),fill=(*rgb,255) if active else (60,72,82,255))
    dr.ellipse((c-r*.62,c-r*.68,c+r*.2,c-r*.05),fill=(255,255,255,180 if active else 35))
    return im.resize((size,size),Image.Resampling.LANCZOS)

for size in (96,160):
    atlas = Image.new("RGBA",(size*11,size*10),(0,0,0,0))
    for i in range(101):
        frame = knob(size,i)
        save(frame,f"knob_{size}_{i:03}.png","knob_frame",
             frame=i,frames=101,pivot=[size/2,size/2],range_degrees=270)
        atlas.alpha_composite(frame,((i%11)*size,(i//11)*size))
    save(atlas,f"knob_{size}_atlas.png","knob_atlas",
         frame_size=size,frames=101,columns=11,rows=10)
for color in ("green","red","blue"):
    for state in ("on","off"):
        save(led(96,PALETTE[color],state=="on"),
             f"led_{color}_{state}.png","led",state=state,role=color)
# Nine-slice starter. Keep the powder coat background stationary and labels separate.
tile=Image.new("RGBA",(128,128),(238,240,241,255));d=ImageDraw.Draw(tile)
for y in range(128):
    t=y/127
    d.line((0,y,127,y),fill=(int(244-10*t),int(246-10*t),int(247-10*t),255))
save(tile,"powdercoat_tile.png","background")
with (OUT/"manifest.json").open("w",encoding="utf-8") as f:
    json.dump(SPEC,f,indent=2)
print(f"Created {len(SPEC['assets'])} RGBA images in {OUT}")
