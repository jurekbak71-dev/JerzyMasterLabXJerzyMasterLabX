# D1 — Input Compressor

Development validation is ongoing.

- Independent ABI 1.0 module, 34 stable parameters, four independent compressor sections and parameter-only serialized state. The optional `masterlabpro_read_gain_reduction_v1` export reads an atomic dB value and does not change the ABI table. The DSP reports zero samples of algorithmic latency.
- Digital uses a stereo peak feed-forward detector, piecewise quadratic soft knee, and separate one-pole attack and release on positive gain reduction in dB. Analog widens the knee, lengthens release with gain reduction, and blends in mild symmetric saturation. This is an original design with no copied external DSP code.
- Low and upper first-order TPT filters define Low = LP₁, Mid = LP₂ − LP₁, High = Input − LP₂. The matched residual paths reconstruct the unprocessed signal directly at unity. Active band gains change the frequency and phase response. First-order slopes deliberately allow band overlap.
- Each band mixes its compressed and uncompressed signal before recombination. SUM then compresses the result. Makeup and analog coloration follow SUM. The internal bypass ramps back to the unprocessed input. Unity defaults are ratio 1:1, 0 dB input/makeup, Digital, active. Analog mode remains an original character option without a specific hardware calibration.
- The VST3 panel uses the existing skinned controls and JUCE parameter attachments. The LOW/MID/HIGH/SUM buttons select the control bank. CROSSOVER exposes both split frequencies. The host's fixed parameter IDs remain tied to automation lanes.

## Local development measurements

At 48 kHz, 128 samples, −6.02 dBFS input, −20 dB threshold, 2:1 and hard knee, measured output was −13.0101 dBFS. The SUM attack reading after 18.7 ms was 4.241 dB (20 ms setting). After 98.7 ms of release the reading was 2.606 dB (100 ms setting). For a 1 kHz sine at 0.9 FS and unity ratio, THD over H2–H20 was 0.0000019% in Digital and 0.2779% in Analog. One local run measured maxima of 29.3 and 68.2 µs per 128 sample block for this signal, respectively. Peak RSS of the entire test process was 8,448 KiB.

The native test exercises all 34 parameter targets concurrently with processing, state restore, aligned graph DELTA, unity impulse reconstruction and the crossover cutoff magnitudes. Pass the CSV destination as a second argument after the library path to export measured static transfer and crossover magnitude/phase. Use `tools/plot_d1.py` to render both.

## Prior design context

The earlier [Smart Dynamics audit](../docs/smart-dynamics/PHASE2B.md) documents a stereo peak detector, continuous knee and mild analog colour. D1 retains these concepts and implements a separate signal path. Analog release depends on the current amount of reduction, a design choice distinct from a measured variable-mu tube model. Its mild saturation runs at base rate, so near-Nyquist aliasing and listening acceptance still require separate evaluation.

## Evidence pending

- Windows CI and the D1 development installer, installed VST3 validation, visual review of scaled captures.
- Actual FL Studio automation and musical listening acceptance.
- Controlled CPU/RAM comparison with equivalent workloads and separate allocation attribution.
