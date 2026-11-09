#!/usr/bin/env python3
"""Fetch pinned StemgenRT weights for authorized noncommercial evaluation.

Separate CC BY-NC-SA model terms apply; see licenses/StemgenRT. This script
does not establish clearance of third-party training recordings or package them.
"""
import argparse
import hashlib
from pathlib import Path
import urllib.request

REVISION="7d2d0c19d956f67259b8358c4127f991077e02b0"
MODELS={
 "model.onnx":("model/model.onnx",31946241,"0a49388dd2a47986ab9a161c8764c22853b4d601c188ac052eb530f076d513c7"),
 "drums.onnx":("model/parallel/drums.onnx",12267886,"f2a3fc5dd90f45690536370f02d9b9377e6625ab08cb32b3e921371ad134d874"),
 "bass.onnx":("model/parallel/bass.onnx",8660860,"1187b414f22344028035caba0a763d2211d13fe8bfce366c5f6b7ee4af77df40"),
 "vocals.onnx":("model/parallel/vocals.onnx",11019332,"f29a3d8fa596ce4672dd4dc77ccf224f2538389940264aad46ab8168311c2148")}

def verify(path,size,digest):
    if path.stat().st_size!=size or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise ValueError(f"Model identity mismatch: {path.name}")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out",type=Path,required=True)
    parser.add_argument("--accept-noncommercial-model-terms",action="store_true",required=True)
    args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    for name,(relative,size,digest) in MODELS.items():
        target=args.out/name
        if not target.exists():
            temporary=target.with_suffix(".download")
            try:
                with urllib.request.urlopen(f"https://media.githubusercontent.com/media/sweetspotsoundsystem/stemgen-rt/{REVISION}/{relative}",timeout=120) as response:
                    with temporary.open("wb") as output:
                        remaining=size+1
                        while remaining:
                            chunk=response.read(min(1024*1024,remaining))
                            if not chunk:break
                            output.write(chunk);remaining-=len(chunk)
                verify(temporary,size,digest)
                temporary.replace(target)
            finally:
                temporary.unlink(missing_ok=True)
        verify(target,size,digest)
        print(name,size,digest)

if __name__=="__main__":main()
