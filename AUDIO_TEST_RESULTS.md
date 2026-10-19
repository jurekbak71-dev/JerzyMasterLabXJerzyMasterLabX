# Test odsłuchowy MasterLab X — 579ab94

Dwa oryginalne, deterministyczne fragmenty syntetyczne po 22 sekundy. To próbki testowe, nie nagrania zespołu ani benchmark jakości względem Ozone. Stereo, WAV PCM 24-bit, 48 kHz; bez MP3 i bez ditheringu.

- PRZED: materiał źródłowy.
- PO: bezpośredni wynik rzeczywistego Linux VST3 z EQ i limiterem. Bez normalizacji po renderze.
- PO_LUFS_MATCH: ten sam wynik ściszony stałym gainem do LUFS pliku PRZED. Od tego porównania zacznij ocenę barwy i dynamiki.

## Pomiary plików WAV

| Plik | LUFS-I | sample peak dBFS | True Peak dBTP (FFmpeg) | crest dB |
|---|---:|---:|---:|---:|
| 01_rytmiczny_PRZED.wav | -23.00 | -6.00 | -5.97 | 17.62 |
| 01_rytmiczny_PO.wav | -17.04 | -1.00 | 0.04 | 16.84 |
| 01_rytmiczny_PO_LUFS_MATCH.wav | -23.01 | -6.96 | -5.92 | 16.84 |
| 02_przestrzenny_PRZED.wav | -25.53 | -6.00 | -5.82 | 20.66 |
| 02_przestrzenny_PO.wav | -21.82 | -2.08 | -1.86 | 21.08 |
| 02_przestrzenny_PO_LUFS_MATCH.wav | -25.53 | -5.79 | -5.57 | 21.08 |

## Co faktycznie przetestowano

Załadowano skompilowany `.vst3` do hosta Pedalboard, bez edytora. Ustawienia ręczne, nie Master Brain. Kolejność EQ → limiter, ceiling próbek −1 dBFS. Rytmiczny: Drive 7 dB / Release 140 ms. Przestrzenny: Drive 4 dB / Release 220 ms. Dokładne pasma, wersje narzędzi, hash binarki i SHA-256 plików znajdują się w measurements.json.

Przeszły: skończone próbki, niezmienna długość, sample ceiling, powtarzalność renderu, zgodność buforów 256/1024 (maksymalna różnica 0), wyrównanie impulsu i LUFS pary porównawczej (do 0,15 LU). Host wymaga pierwszego pustego wywołania po ustawieniu sample rate, aby odświeżyć kompensację latencji. Następny render impulsu ma przesunięcie 0 próbek. Ten krok jest jawny w skrypcie.

## Wykryte ograniczenie

W pliku rytmicznym PO zmierzono około +0,04 dBTP mimo sample ceiling −1 dBFS. Obecny limiter nie jest True Peak: szczyty między próbkami mogą przekroczyć limit. Nie ukryto tego dodatkowym ściszeniem pliku PO. Ostrożnie z głośnością odsłuchu. Wersja LUFS_MATCH służy porównaniu przy wyrównanej głośności, nie dowodzi poprawnej kontroli True Peak.

Nie wykonano odsłuchowej oceny brzmienia w tym środowisku ani testu FL Studio. To test hostowanego Linux VST3, nie render wykonany w Windows/FL Studio. Wyniki nie dowodzą jakości komercyjnego masteringu i nie obejmują prawdziwego miksu z mikrofonów/instrumentów.

## Powtórzenie

Zbuduj plugin zgodnie z README repozytorium. Utwórz osobne środowisko Python, zainstaluj `tools/audio/requirements.txt`, uruchom:

```
python tools/audio/render_comparisons.py --plugin build-plugin/JerzyMasterLabX_artefacts/Release/VST3/JerzyMasterLabX.vst3 --output output-audio
```

Wymagany również FFmpeg. Zewnętrzne pomiary loudnorm/True Peak są weryfikacją plików; nie oznaczają dodania tych funkcji do DSP wtyczki.
