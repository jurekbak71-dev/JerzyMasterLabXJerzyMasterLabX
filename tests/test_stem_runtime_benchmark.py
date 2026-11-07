"""Independent WAV/metric oracles for the offline-runtime feasibility harness."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import wave

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from benchmark_stem_runtime import read_wav, reconstruction_metrics, validate_stem_outputs


def float_wav(path, values, rate=44100):
    data = struct.pack("<" + "f" * len(values), *values)
    fmt = struct.pack("<HHIIHH", 3, 2, rate, rate * 8, 8, 32)
    path.write_bytes(b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVEfmt " +
                     struct.pack("<I", 16) + fmt + b"data" + struct.pack("<I", len(data)) + data)


class RuntimeMetricTests(unittest.TestCase):
    def test_reused_output_directory_preserves_old_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            evidence = folder / "evidence"
            evidence.mkdir()
            source = evidence / "fixture-44100.wav"
            source.write_bytes(b"previous evidence must not be overwritten")
            model = folder / "model.gguf"
            model.write_bytes(b"unused: refusal must precede execution")
            script = Path(__file__).resolve().parents[1] / "tools" / "benchmark_stem_runtime.py"
            child = subprocess.run([sys.executable, str(script), "--runtime", sys.executable,
                                    "--model", str(model), "--out", str(evidence), "--seconds", ".001",
                                    "--rates", "44100", "--threads", "1"], capture_output=True, timeout=10)
            self.assertNotEqual(child.returncode, 0)
            self.assertEqual(source.read_bytes(), b"previous evidence must not be overwritten")

    def test_resampled_outputs_must_cover_entire_input_duration(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            source = folder / "input.wav"
            float_wav(source, [0.0] * 96000, 48000)  # one stereo second
            for count in (1, 44098, 44102):
                for stem in ("drums", "bass", "other", "vocals"):
                    float_wav(folder / (stem + ".wav"), [0.0] * (2 * count))
                with self.assertRaises(ValueError):
                    validate_stem_outputs(source, folder)
            for stem in ("drums", "bass", "other", "vocals"):
                float_wav(folder / (stem + ".wav"), [0.0] * 88200)
            self.assertEqual(validate_stem_outputs(source, folder), (44100, 44100))

    def test_pcm16_and_float32_read_same_samples(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "input.wav"
            with wave.open(str(path), "wb") as output:
                output.setparams((2, 2, 44100, 0, "NONE", "not compressed"))
                output.writeframes(struct.pack("<hhhh", 16384, -16384, 0, 8192))
            rate, channels, values = read_wav(path)
            self.assertEqual((rate, channels), (44100, 2))
            self.assertEqual(list(values), [.5, -.5, 0, .25])
            float_wav(path, [.5, -.5, 0, .25])
            self.assertEqual(list(read_wav(path)[2]), [.5, -.5, 0, .25])

    def test_exact_sum_and_hand_calculated_residual(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            source = folder / "input.wav"
            float_wav(source, [.5, -.5])
            for stem in ("drums", "bass", "other", "vocals"):
                float_wav(folder / (stem + ".wav"), [.125, -.125])
            self.assertEqual(reconstruction_metrics(source, folder)["residual_rms"], 0)
            float_wav(folder / "vocals.wav", [.25, -.25])
            result = reconstruction_metrics(source, folder)
            self.assertEqual(result["residual_rms"], .125)
            self.assertEqual(result["residual_peak"], .125)
            self.assertAlmostEqual(result["relative_residual_db"], -12.041199826559248)

    def test_inconsistent_rates_and_nonfinite_audio_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            source = folder / "input.wav"
            float_wav(source, [.5, -.5])
            for stem in ("drums", "bass", "other", "vocals"):
                float_wav(folder / (stem + ".wav"), [.125, -.125])
            float_wav(folder / "vocals.wav", [.125, -.125], 48000)
            with self.assertRaises(ValueError):
                reconstruction_metrics(source, folder)
            float_wav(source, [math.nan, .25])
            with self.assertRaises(ValueError):
                read_wav(source)


if __name__ == "__main__":
    unittest.main()
