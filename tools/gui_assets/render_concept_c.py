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

def led_ring(size,color):
    S=4;d=size*S;c=d//2
    rgb=tuple(int(color[i:i+2],16) for i in (1,3,5))
    glow=Image.new("RGBA",(d,d),(0,0,0,0));dg=ImageDraw.Draw(glow,"RGBA")
    box=(int(d*.075),int(d*.075),int(d*.925),int(d*.925))
    for k in range(12):
        start=-120+k*25
        dg.arc(box,start,start+17,fill=(*rgb,240),width=9*S)
    blur=glow.filter(ImageFilter.GaussianBlur(8*S))
    blur.alpha_composite(glow)
    return blur.resize((size,size),Image.Resampling.LANCZOS)

def analog_meter_bezel():
    """Locked-frame, high-res PNG faceplate; live needle is drawn by JUCE."""
    width,height=504,392
    im=Image.new("RGBA",(width,height),(0,0,0,0))
    dr=ImageDraw.Draw(im,"RGBA")
    dr.rounded_rectangle((2,2,width-3,height-3),radius=28,fill=(68,76,83,255),
                         outline=(18,25,32,255),width=6)
    dr.rounded_rectangle((12,12,width-13,height-13),radius=22,fill=(164,174,182,255),
                         outline=(220,228,232,255),width=5)
    dr.rounded_rectangle((26,24,width-27,height-28),radius=18,fill=(24,32,40,255),
                         outline=(96,111,123,255),width=5)
    dr.rounded_rectangle((36,34,width-37,height-39),radius=12,fill=(235,220,188,255),
                         outline=(110,93,77,255),width=4)
    for y in range(47,height-44,3):
        alpha=int(26*(y-47)/max(1,height-90))
        dr.line((48,y,width-48,y),fill=(124,101,72,alpha),width=2)
    # Four hardware mounting screws, in separate recessed corners.
    for cx,cy in ((20,20),(width-20,20),(20,height-20),(width-20,height-20)):
        dr.ellipse((cx-11,cy-11,cx+11,cy+11),fill=(43,53,64,255))
        dr.ellipse((cx-7,cy-7,cx+7,cy+7),fill=(197,208,215,255))
        dr.line((cx-4,cy+4,cx+4,cy-4),fill=(62,73,84,255),width=3)
    return im

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
    save(led_ring(192,PALETTE[color]),f"ring_{color}_on.png","ring",
         role=color,diameter=192,segments=12)
# Nine-slice starter. Keep the powder coat background stationary and labels separate.
tile=Image.new("RGBA",(128,128),(238,240,241,255));d=ImageDraw.Draw(tile)
for y in range(128):
    t=y/127
    d.line((0,y,127,y),fill=(int(244-10*t),int(246-10*t),int(247-10*t),255))
save(tile,"powdercoat_tile.png","background")
save(analog_meter_bezel(),"analog_gr_bezel.png","meter_bezel",live_needle=True)

# Glass/LED kit (separate RGBA strata, imported at CMake configure time).
def segmented_wreath(size, phase):
    d=size*2; im=Image.new("RGBA",(d,d),(0,0,0,0))
    layer=Image.new("RGBA",(d,d),(0,0,0,0))
    draw=ImageDraw.Draw(layer,"RGBA")
    n=48; count=round(n*phase/100)
    center=d*.5; r1=d*.411; r2=d*.463
    for i in range(n):
        angle=math.radians(-135+270*i/(n-1))
        p1=(center+math.sin(angle)*r1,center-math.cos(angle)*r1)
        p2=(center+math.sin(angle)*r2,center-math.cos(angle)*r2)
        draw.line((*p1,*p2),fill=(57,221,255,255) if i<count else (30,65,87,145),
                  width=max(2,size//22))
    glow=layer.filter(ImageFilter.GaussianBlur(max(2,size//15)))
    glow.putalpha(glow.getchannel("A").point(lambda x: int(x*.66)))
    im.alpha_composite(glow); im.alpha_composite(layer)
    return im.resize((size,size),Image.Resampling.LANCZOS)

def recessed_glass(size, overlay=False):
    w,h=size; im=Image.new("RGBA",(w,h),(0,0,0,0));d=ImageDraw.Draw(im,"RGBA")
    if not overlay:
        d.rounded_rectangle((0,0,w-1,h-1),radius=12,fill=(8,21,37,255),
                            outline=(45,156,200,255),width=3)
        d.rounded_rectangle((5,5,w-6,h-6),radius=9,outline=(149,201,230,88),width=2)
    d.polygon(((9,7),(int(w*.55),7),(int(w*.28),int(h*.45)),(6,int(h*.52))),
              fill=(235,249,255,42 if overlay else 20))
    d.polygon(((int(w*.81),6),(w-7,6),(w-7,int(h*.42)),(int(w*.68),int(h*.59))),
              fill=(222,244,255,26 if overlay else 16))
    return im

def card_glow(on):
    w,h=320,180; core=Image.new("RGBA",(w,h),(0,0,0,0))
    d=ImageDraw.Draw(core,"RGBA")
    d.rounded_rectangle((11,11,w-12,h-12),radius=12,
                        outline=(8,205,255,230) if on else (65,89,108,95),width=4)
    glow=core.filter(ImageFilter.GaussianBlur(12))
    glow.alpha_composite(core)
    return glow

def switch_sprite(on):
    w,h=160,70; im=Image.new("RGBA",(w,h),(0,0,0,0));d=ImageDraw.Draw(im,"RGBA")
    d.rounded_rectangle((3,3,w-4,h-4),radius=11,fill=(25,42,58,255),
                        outline=(119,149,170,255),width=3)
    d.rounded_rectangle((9,9,w-10,h-10),radius=8,
                        fill=(18,91,126,255) if on else (12,22,32,255),
                        outline=(45,204,249,235) if on else (61,83,98,255),width=2)
    d.line((16,13,w-17,13),fill=(240,251,255,125),width=2)
    return im

for size in (96,160):
    wreath=Image.new("RGBA",(size*11,size*10),(0,0,0,0))
    for phase in range(101):
        wreath.alpha_composite(segmented_wreath(size,phase),
                               ((phase%11)*size,(phase//11)*size))
    save(wreath,f"led_wreath_{size}_atlas.png","led_ring_atlas",
         frame_size=size,frames=101,columns=11,rows=10,segments=48)
for name,im,category in (
    ("glass_analyzer.png",recessed_glass((720,240)),"glass"),
    ("glass_overlay.png",recessed_glass((720,240),True),"glass_overlay"),
    ("glass_readout.png",recessed_glass((240,80)),"glass"),
    ("glass_readout_overlay.png",recessed_glass((240,80),True),"glass_overlay"),
    ("module_glow_on.png",card_glow(True),"panel_glow"),
    ("module_glow_off.png",card_glow(False),"panel_glow"),
    ("switch_on.png",switch_sprite(True),"switch"),
    ("switch_off.png",switch_sprite(False),"switch"),
):
    save(im,name,category)

with (OUT/"manifest.json").open("w",encoding="utf-8") as f:
    json.dump(SPEC,f,indent=2)
print(f"Created {len(SPEC['assets'])} RGBA images in {OUT}")
