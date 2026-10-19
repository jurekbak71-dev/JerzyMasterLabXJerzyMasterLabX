#!/usr/bin/env python3
"""Compare original synthetic fixtures with FFmpeg ebur128; optionally write golden data.
Example: python tests/generate_loudness_reference.py --probe build-core/loudness_probe
Use --output tests/data/loudness-reference.csv to regenerate after reviewing differences.
FFmpeg is a reference tool only and is not linked into or distributed with the plugin.
"""
import argparse
import csv
import re
import subprocess
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--probe', type=Path, required=True)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
probe = args.probe.resolve()
rows = []
print(subprocess.check_output(['ffmpeg', '-version'], text=True).splitlines()[0])
with tempfile.TemporaryDirectory(prefix='masterlab-loudness-') as directory:
    for rate in (44100, 48000, 96000, 192000):
        for kind in range(4):
            raw = Path(directory) / f'{rate}-{kind}.f32'
            ours = list(map(float, subprocess.check_output(
                [str(probe), str(rate), str(kind), str(raw)], text=True).strip().split(',')))
            run = subprocess.run([
                'ffmpeg', '-hide_banner', '-f', 'f32le', '-ar', str(rate), '-ac', '2',
                '-i', str(raw), '-af', 'ebur128=peak=none', '-f', 'null', '-'
            ], capture_output=True, text=True, check=True)
            integrated = float(re.findall(r'I:\s+(-?\d+(?:\.\d+)?) LUFS', run.stderr)[-1])
            momentary, short_term = map(float, re.findall(
                r'M:\s*(-?\d+(?:\.\d+)?)\s+S:\s*(-?\d+(?:\.\d+)?)', run.stderr)[-1])
            expected = [integrated, momentary, short_term]
            print(rate, kind, 'MasterLab:', ours, 'FFmpeg:', expected, flush=True)
            if any(abs(a-b) > .11 for a, b in zip(ours, expected)):
                raise RuntimeError('Loudness mismatch exceeds 0.11 LU')
            rows.append([rate, kind, *expected])
if args.output:
    with args.output.open('w', newline='') as output:
        writer = csv.writer(output, lineterminator='\n')
        writer.writerow(['sample_rate', 'fixture', 'integrated', 'momentary', 'short_term'])
        writer.writerows(rows)
