# Zakres wszystkich narzędzi — punkt odniesienia Ozone 12 Advanced

Instrukcja Ozone wymienia 20 rodzin procesorów (rozdziały 5–24). Asystent, mierniki i narzędzia odsłuchu są dodatkowymi panelami, a nie kolejnymi szeregowymi procesorami. Poniższa lista jest zakresem rozwoju, nie deklaracją gotowej funkcjonalności. Obecnie działają Spectral EQ, Dynamic EQ, Smart Dynamics, Stereo Architect, wczesny sample-peak limiter, Inspector i regułowy Master Brain. Szczegóły i ograniczenia: MODULAR_WORKFLOW.md.

| Rodzina odniesienia | Docelowy panel MasterLab X | Stan |
|---|---|---|
| Bass Control | Bass Architect | Do implementacji |
| Clarity | Clarity Engine | Do implementacji |
| Dynamics | Smart Dynamics | Digital/Analog, downward/upward, soft knee, mix; bez multiband/oversamplingu |
| Dynamic EQ | Dynamic EQ | 3 niezależne pasma z detektorami, statyczny gain i dynamic range |
| Equalizer | Spectral EQ | Minimum phase, 16 pasm; pozostałe tryby przyszłe |
| Exciter | Harmonic Exciter | Do implementacji |
| Imager | Stereo Architect | 4 pasma M/S, 3 podziały, Mono Bass/Preview, korelacja per-band |
| Impact | Transient Sculptor | Do implementacji |
| Low End Focus | Low End Dynamics | Do implementacji |
| Master Rebalance | Stem Balance | Do implementacji, zależność od separacji |
| Match EQ | Reference Match EQ | Do implementacji |
| Maximizer | Ultimate Limiter | Wczesny sample-peak; brak TP i oversamplingu |
| Spectral Shaper | Spectral Repair | Do implementacji |
| Stabilizer | Adaptive Balance | Do implementacji |
| Stem EQ | Stem EQ | Do implementacji, zależność od separacji |
| Unlimiter | Transient Recovery | Do implementacji, weryfikacja wykonalności |
| Vintage Compressor | Analog Compressor | Do implementacji |
| Vintage EQ | Analog EQ | Do implementacji |
| Vintage Limiter | Analog Limiter | Do implementacji |
| Vintage Tape | Tape Laboratory | Do implementacji |

Dodatkowo: Master Brain (obecnie reguły i akceptacja), Master Inspector (obecne podstawowe mierniki/LUFS), Reference/Compare, globalne mono/Delta/gain match, Codec Preview i Dither. Samo dodanie zakładek nie oznacza wykonania procesorów. Każdy nowy procesor wymaga DSP, parametrów, trwałego stanu, bypassu, pomiarów i testów offline/hosta.

Pierwszeństwo przed rozbudową paneli ma test rzeczywistego VST3 i usunięcie wykrytych problemów. Test audio 579ab94 wykazał przekroczenie międzypróbkowe +0,04 dBTP mimo ceiling próbek −1 dBFS. Raport: AUDIO_TEST_RESULTS.md. Tego wyniku nie ukryto normalizacją pliku PO.
