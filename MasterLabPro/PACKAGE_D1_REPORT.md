# D1 — Input Compressor

Development build `7790be20881f331e98575f479cbf11dd0fa49aca` passed Windows x64 CI. Musical acceptance and FL Studio-specific validation are pending.

- Independent ABI 1.0 module, 34 stable parameters, four independent compressor sections and parameter-only serialized state. The optional `masterlabpro_read_gain_reduction_v1` export reads an atomic dB value and does not change the ABI table. The DSP reports zero samples of algorithmic latency.
- Digital uses a stereo peak feed-forward detector, piecewise quadratic soft knee, and separate one-pole attack and release on positive gain reduction in dB. Analog widens the knee, lengthens release with gain reduction, and blends in mild symmetric saturation. This is an original design with no copied external DSP code.
- Low and upper first-order TPT filters define Low = LP₁, Mid = LP₂ − LP₁, High = Input − LP₂. The matched residual paths reconstruct the unprocessed signal directly at unity. Active band gains change the frequency and phase response. First-order slopes deliberately allow band overlap.
- Each band mixes its compressed and uncompressed signal before recombination. SUM then compresses the result. Makeup and analog coloration follow SUM. The internal bypass ramps back to the unprocessed input. Unity defaults are ratio 1:1, 0 dB input/makeup, Digital, active. Analog mode remains an original character option without a specific hardware calibration.
- The VST3 panel uses the existing skinned controls and JUCE parameter attachments. The LOW/MID/HIGH/SUM buttons select the control bank. CROSSOVER exposes both split frequencies. The host's fixed parameter IDs remain tied to automation lanes.

## Local development measurements

At 48 kHz, 128 samples, −6.02 dBFS input, −20 dB threshold, 2:1 and hard knee, measured output was −13.0101 dBFS. The SUM attack reading after 18.7 ms was 4.241 dB (20 ms setting). After 98.7 ms of release the reading was 2.606 dB (100 ms setting). For a 1 kHz sine at 0.9 FS and unity ratio, THD over H2–H20 was 0.0000019% in Digital and 0.2779% in Analog. One local run measured maxima of 29.3 and 68.2 µs per 128 sample block for this signal, respectively. Peak RSS of the entire test process was 8,448 KiB.

The native test exercises all 34 parameter targets concurrently with processing, state restore, aligned graph DELTA, unity impulse reconstruction and the crossover cutoff magnitudes. Pass the CSV destination as a second argument after the library path to export measured static transfer and crossover magnitude/phase. Use `tools/plot_d1.py` to render both.

In a separate local Release run with all four sections at 4:1 and −28 dB, the 128-sample block median was 43–47 µs, p95 was 57–65 µs, and p99 was 94–107 µs across 48/96/192 kHz and both modes. Maximums in that sweep ranged from 407 to 736 µs. A run at 192 kHz has a 667 µs block budget and maximum times were sensitive to scheduling in this environment. This benchmark is whole-module time. The benchmark process peaked at 6,400 KiB RSS, including runtime and driver.

Linux Release, ASan+UBSan with leak detection, and TSan each passed 22/22. The Linux JUCE test used an explicit fixture directory and dynamically loaded the compressor. It passed 100 module re-selections and right-click resets on the existing Gain panel, then generated 1200×750, 1800×1125 and 2400×1500 renders with actual compressor telemetry. The installed Windows test also exercises the native empty-slot popup.

An instrumented native lifetime test passed 200 library load/unload cycles with eight independent instances per cycle. A local CMake/JUCE 8.0.6 Linux x64 VST3 passed pluginval level 5 and the JUCE VST3 host test with eight instances of Gain and eight instances of Input Compressor in separate runs. Each run exercised native module loading, independent gain and state save through the VST3 wrapper.

A separate hosted JUCE test exercised complete change gestures on all 34 VST3 parameters mapped to the compressor. All 34 physical values changed in the saved component state. The test reads the JUCE private trailer separately from MasterLabPro's state payload. This verifies the VST3 interface on Linux, with FL Studio-specific interactions pending manual validation.

The Package C editor regression set a 65 px minimum. Its existing Gain dial measured 64.78 px. D1 recovered the required size by reducing the generic panel header by two pixels, and the Windows editor test then passed. The first D1 VST3 instance run sampled the compressor after 10 blocks. Direct reproduction measured 0.000868 error at that point during the 10 ms Input Gain ramp. At block 46 the sample matched the final value. The VST3 harness now waits 48 blocks for the compressor with the original 0.0001 sample tolerance, and retains the original ten blocks for Gain.

## Prior design context

The earlier [Smart Dynamics audit](../docs/smart-dynamics/PHASE2B.md) documents a stereo peak detector, continuous knee and mild analog colour. D1 retains these concepts and implements a separate signal path. Analog release depends on the current amount of reduction, a design choice distinct from a measured variable-mu tube model. Its mild saturation runs at base rate, so near-Nyquist aliasing and listening acceptance still require separate evaluation.

The published abstract of Giannoulis, Massberg and Reiss compares detector and gain-computer approaches including peak versus RMS and feedforward versus feedback. D1 uses peak/feedforward for a predictable static transfer and a gain-reduction-dependent release in Analog. The complete paper could not be retrieved by the local HTTP client. Manley's public catalogue establishes Variable Mu® as a tube compressor/limiter. D1 transfers no circuit, model, code or copyrighted settings. [JAES 60(6)](https://secure.aes.org/forum/pubs/journal/?ID=174) · [Manley](https://www.manley.com/)

## Stable parameter layout

| Lane positions (zero-based) | Controls | Local IDs |
|---|---|---|
| 0–5 | Input, Makeup, Low/Mid, Mid/High, Analog, Bypass | 1–6 |
| 6–12 | LOW | 100, 110, 120, 130, 140, 150, 160 |
| 13–19 | MID | 101, 111, 121, 131, 141, 151, 161 |
| 20–26 | HIGH | 102, 112, 122, 132, 142, 152, 162 |
| 27–33 | SUM | 103, 113, 123, 133, 143, 153, 163 |

Each bank uses the order threshold, ratio, attack, release, knee, mix, link. Ratio, time and crossover controls map logarithmically. The 640 host IDs retain their fixed lane names. The editor displays descriptor names and physical values, and gestures target the mapped parameter. The module instance retains its lane when the rack is reordered.

## Windows validation

- Windows x64 CMake/JUCE build and CTest passed. The development installer was built and installed on the runner with the compressor DLL. The real editor test passed and produced three scaled renders.
- The installed VST3 passed separate eight-instance Gain and eight-instance Input Compressor tests, plus pluginval strictness 5. Both Linux sanitizer CI jobs passed.
- Code commits `742bf2f`, `18c4158`, `b21e92c`, `7790be2` are on `masterlabpro-development`.
- [GitHub Actions run](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45873262126)
- [Development installer with Input Compressor](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45873262126/artifacts/15144387520)
- [VST3 validation, editor renders and measurement CSVs](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45873262126/artifacts/15143533587)

## Unverified in this environment

- Manual FL Studio checks for Last Tweaked, Browse Parameters, Automation Clips, Link to Controller, and session reopen.
- Musical listening acceptance and absolute realtime deadlines on the target Windows workstation.
