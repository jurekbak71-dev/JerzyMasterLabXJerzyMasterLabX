# D1 — Input Compressor

Development validation is ongoing.

- Independent ABI 1.0 module, 34 stable parameters, four independent compressor sections and parameter-only serialized state. The optional `masterlabpro_read_gain_reduction_v1` export reads an atomic dB value and does not change the ABI table. The DSP reports zero samples of algorithmic latency.
- Digital uses a stereo peak feed-forward detector, piecewise quadratic soft knee, and separate one-pole attack and release on positive gain reduction in dB. Analog widens the knee, lengthens release with gain reduction, and blends in mild symmetric saturation. This is an original design with no copied external DSP code.
- Low and upper first-order TPT filters define Low = LP₁, Mid = LP₂ − LP₁, High = Input − LP₂. The matched residual paths reconstruct the unprocessed signal directly at unity. Active band gains change the frequency and phase response. First-order slopes deliberately allow band overlap.
- Each band mixes its compressed and uncompressed signal before recombination. SUM then compresses the result. Makeup and analog coloration follow SUM. The internal bypass ramps back to the unprocessed input. Unity defaults are ratio 1:1, 0 dB input/makeup, Digital, active. Analog mode remains an original character option without a specific hardware calibration.
- The VST3 panel uses the existing skinned controls and JUCE parameter attachments. The LOW/MID/HIGH/SUM buttons select the control bank. CROSSOVER exposes both split frequencies. The host's fixed parameter IDs remain tied to automation lanes.

## Local development measurements

At 48 kHz, 128 samples, −6.02 dBFS input, −20 dB threshold, 2:1 and hard knee, measured output was −13.0101 dBFS. The SUM attack reading after 18.7 ms was 4.241 dB (20 ms setting). After 98.7 ms of release the reading was 2.606 dB (100 ms setting). For a 1 kHz sine at 0.9 FS and unity ratio, H3/fundamental was 1.04e−9 in Digital and 0.002778 in Analog. These figures describe H3, not full THD.

## Evidence pending

- Windows CI and the D1 development installer, installed VST3 validation, visual review of scaled captures.
- Actual FL Studio automation and musical listening acceptance.
- Full crossover plots, full THD and a controlled CPU/RAM comparison.
