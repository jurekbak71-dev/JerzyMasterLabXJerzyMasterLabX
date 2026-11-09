#!/usr/bin/env python3
"""Offline CLI feasibility measurements. Does NOT qualify a streaming backend.

Uses existing stems.cpp v0.1.6 CLI; no model is downloaded or redistributed here.
Install psutil==7.1.3 for sampled child CPU/RSS metrics. Synthetic input proves
execution/finite outputs/reconstruction only, never musical separation quality.
"""
import argparse
from array import array
import hashlib
import json
import math
from pathlib import Path
import platform
import re
import struct
import subprocess
import sys
import time
import wave

STEMS = ("drums", "bass", "other", "vocals")


def read_wav(path):
    """Read PCM16 or IEEE float32 RIFF, including extensible format headers."""
    raw = Path(path).read_bytes()
    if len(raw) < 12 or raw[:4] != b"RIFF" or raw[8:12] != b"WAVE":
        raise ValueError(f"Not a RIFF WAV: {path}")
    offset, fmt, data = 12, None, None
    while offset + 8 <= len(raw):
        tag, size = struct.unpack_from("<4sI", raw, offset)
        offset += 8
        chunk = raw[offset:offset + size]
        if len(chunk) != size:
            raise ValueError("Truncated WAV chunk")
        if tag == b"fmt ":
            fmt = chunk
        if tag == b"data":
            data = chunk
        offset += size + (size & 1)
    if fmt is None or data is None or len(fmt) < 16:
        raise ValueError("Missing WAV format/data")
    encoding, channels, rate, _, align, bits = struct.unpack_from("<HHIIHH", fmt)
    if encoding == 0xFFFE and len(fmt) >= 40:
        encoding = struct.unpack_from("<H", fmt, 24)[0]
    if channels not in (1, 2) or rate <= 0 or align != channels * bits // 8 or len(data) % align:
        raise ValueError("Inconsistent WAV layout")
    if (encoding, bits) not in ((1, 16), (3, 32)):
        raise ValueError("Only PCM16/float32 WAV is supported by this harness")
    values = array("h" if encoding == 1 else "f")
    values.frombytes(data)
    if sys.byteorder != "little":
        values.byteswap()
    if encoding == 1:
        values = array("f", (x / 32768.0 for x in values))
    if not all(math.isfinite(x) for x in values):
        raise ValueError("Nonfinite WAV samples")
    return rate, channels, values


def reconstruction_metrics(source, folder):
    rate, channels, original = read_wav(source)
    tracks = []
    for name in STEMS:
        stem_rate, stem_channels, samples = read_wav(Path(folder) / (name + ".wav"))
        if (stem_rate, stem_channels, len(samples)) != (rate, channels, len(original)):
            raise ValueError("Stem rate/channels/length do not match reference")
        tracks.append(samples)
    if not original:
        raise ValueError("Empty reference")
    error, energy, peak = 0.0, 0.0, 0.0
    for i, sample in enumerate(original):
        residual = sum(track[i] for track in tracks) - sample
        error += residual * residual
        energy += sample * sample
        peak = max(peak, abs(residual))
    return {"residual_rms": math.sqrt(error / len(original)), "residual_peak": peak,
            "relative_residual_db": 10 * math.log10(max(1e-30, error / max(1e-30, energy)))}


def validate_stem_outputs(source, folder):
    source_rate, source_channels, source_data = read_wav(source)
    source_frames = len(source_data) // source_channels
    outputs = [read_wav(Path(folder) / (name + ".wav")) for name in STEMS]
    output_rate, channels, data = outputs[0]
    if channels != 2 or not data or not source_frames or any(
            (item[0], item[1], len(item[2])) != (output_rate, 2, len(data)) for item in outputs):
        raise ValueError("Inconsistent/empty separated outputs")
    frames = len(data) // channels
    if abs(frames - source_frames * output_rate / source_rate) > 1:
        raise ValueError("Separated duration does not cover the complete input")
    return output_rate, frames


def write_fixture(path, rate, seconds):
    """Deterministic diagnostic waveform, not a music/multitrack quality set."""
    samples = array("h")
    for i in range(round(rate * seconds)):
        t = i / rate
        transient = .15 * math.exp(-80 * (t % .5)) * math.sin(2 * math.pi * 80 * t)
        common = .15 * math.sin(2 * math.pi * 110 * t) + .08 * math.sin(2 * math.pi * 880 * t)
        for phase in (0, .3):
            value = common + transient + .04 * math.sin(2 * math.pi * 330 * t + phase)
            samples.append(round(value * 32767))
    if sys.byteorder != "little":
        samples.byteswap()
    with wave.open(str(path), "wb") as output:
        output.setparams((2, 2, rate, 0, "NONE", "not compressed"))
        output.writeframes(samples.tobytes())


def digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def measure(command, log, timeout):
    import psutil
    started = time.perf_counter()
    peak_rss, cpu_seconds = 0, 0.0
    with log.open("w", encoding="utf-8") as output:
        child = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)
        process = psutil.Process(child.pid)
        try:
            while True:
                try:
                    memory = process.memory_info()
                    peak_rss = max(peak_rss, memory.rss, getattr(memory, "peak_wset", 0))
                    cpu = process.cpu_times()
                    cpu_seconds = max(cpu_seconds, cpu.user + cpu.system)
                except psutil.NoSuchProcess:
                    pass
                if child.poll() is not None:
                    break
                if time.perf_counter() - started > timeout:
                    raise TimeoutError(f"Runtime exceeded {timeout}s; see {log}")
                time.sleep(.02)
        finally:
            if child.poll() is None:
                child.kill()
            child.wait()
    wall = time.perf_counter() - started
    if child.returncode:
        raise RuntimeError(f"Runtime returned {child.returncode}; see {log}")
    text = log.read_text(encoding="utf-8", errors="replace")
    reported = re.search(r"separated in ([0-9.]+) s", text)
    return {"process_wall_seconds": wall, "sampled_cpu_seconds": cpu_seconds,
            "peak_rss_bytes": peak_rss, "sampling_period_seconds": .02,
            "reported_inference_seconds": float(reported[1]) if reported else None}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--seconds", type=float, default=9)
    parser.add_argument("--rates", type=int, nargs="+", default=[44100, 48000])
    parser.add_argument("--threads", type=int, nargs="+", default=[1, 0])
    parser.add_argument("--timeout", type=float, default=1800)
    args = parser.parse_args()
    if not 0 < args.seconds <= 30 or any(rate not in (44100, 48000, 96000) for rate in args.rates):
        parser.error("Use 0–30s and 44.1/48/96 kHz diagnostic input")
    runtime, model = args.runtime.resolve(), args.model.resolve()
    # Refuse reuse before writing anything; prior stems must never become new evidence.
    args.out.mkdir(parents=True, exist_ok=False)
    report = {"kind": "offline_cli_feasibility_not_live_qualification", "platform": platform.platform(),
              "processor": platform.processor(), "logical_cpus": __import__("os").cpu_count(),
              "model_sha256": digest(model), "runtime_sha256": digest(runtime),
              "input": "deterministic synthetic tones/transients; not music quality reference",
              "limits": ["wall includes loading and I/O; inference clock is CLI-reported/rounded",
                         "CPU/RSS sampled every 20ms; sampled CPU may omit final interval",
                         "no streaming hop, lookahead, state continuity, leakage/SDR or FL Studio proof",
                         "CLI converts to model rate; 48/96k reconstruction needs a resampling oracle"],
              "cases": []}
    for rate in args.rates:
        source = args.out / f"fixture-{rate}.wav"
        write_fixture(source, rate, args.seconds)
        for threads in args.threads:
            case = args.out / f"rate-{rate}-threads-{threads}"
            case.mkdir(exist_ok=False)
            command = [str(runtime), "-m", str(model), "-i", str(source.resolve()), "-o", str(case.resolve()),
                       "--device", "cpu", "--threads", str(threads), "--shifts", "0", "--seed", "0", "--float32"]
            metrics = measure(command, case / "runtime.log", args.timeout)
            result = {"input_rate": rate, "threads": threads, "seconds": args.seconds, "command": command,
                      "input_sha256": digest(source), **metrics,
                      "process_rtf": metrics["process_wall_seconds"] / args.seconds}
            infer = metrics["reported_inference_seconds"]
            result["reported_inference_rtf"] = infer / args.seconds if infer is not None else None
            # Require all four real finite outputs, including at rates resampled by CLI.
            output_rate, output_frames = validate_stem_outputs(source, case)
            result["output_rate"] = output_rate
            result["output_samples"] = output_frames
            result["reconstruction"] = reconstruction_metrics(source, case) if rate == output_rate else None
            report["cases"].append(result)
            (args.out / "measurements.json").write_text(json.dumps(report, indent=2, allow_nan=False), encoding="utf-8")
            print(json.dumps(result, allow_nan=False), flush=True)


if __name__ == "__main__":
    main()
