"""Plot the static transfer curves measured by the native D1 test executable."""
import argparse
import csv
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("measurements")
parser.add_argument("plot")
args = parser.parse_args()
curves = defaultdict(list)
with open(args.measurements, newline="", encoding="utf-8") as stream:
    for row in csv.DictReader(stream):
        curves[row["mode"], row["knee_db"]].append(
            (float(row["input_dbfs"]), float(row["output_dbfs"]))
        )

fig, axes = plt.subplots(1, 2, figsize=(10.5, 4.5), layout="constrained")
for mode, axis in enumerate(axes):
    axis.plot([-45, 0], [-45, 0], ":", color="#777", label="Unity")
    for knee in ("0", "12"):
        points = curves[str(mode), knee]
        axis.plot(*zip(*points), label=f"Knee set to {knee} dB")
    axis.set(xlim=(-45, 0), ylim=(-45, 0), xlabel="Input (dBFS)",
             ylabel="Output (dBFS)", title=("Digital", "Analog")[mode])
    axis.grid(alpha=0.2)
    axis.legend()
fig.suptitle("Input Compressor · measured static SUM transfer\n48 kHz · threshold −20 dB · ratio 4:1")
fig.savefig(args.plot, dpi=160)
plt.close(fig)

fig, (amplitude, phase) = plt.subplots(2, 1, figsize=(10.5, 7), sharex=True, layout="constrained")
with open(args.measurements + ".bands.csv", newline="", encoding="utf-8") as stream:
    rows = list(csv.DictReader(stream))
hz = [float(row["hz"]) for row in rows]
for name in ("low", "mid", "high"):
    amplitude.semilogx(hz, [float(row[name + "_db"]) for row in rows], label=name.upper())
    phase.semilogx(hz, [float(row[name + "_phase_deg"]) for row in rows], label=name.upper())
amplitude.set(ylabel="Magnitude (dB)")
phase.set(xlabel="Frequency (Hz)", ylabel="Phase (degrees)")
for axis in (amplitude, phase):
    axis.grid(alpha=0.2)
    axis.legend()
fig.suptitle("Measured crossover impulse response · 48 kHz · 180 Hz / 3.2 kHz")
path = Path(args.plot)
fig.savefig(path.with_name(path.stem + "-crossover" + path.suffix), dpi=160)
