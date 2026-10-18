# Pomiary

Bieżąca maszyna: Linux, GCC 14.2.0, CMake 4.4.4, Release. CTest: 1/1 passed, test obejmuje ceiling, opóźnienie impulsu, deterministyczność bloków 4096 vs 37, NaN/Inf przy 44.1/48/96/192 kHz oraz gain EQ przy 1 kHz. Dodatkowo testowana jest suma dwóch pasm (+12 dB) i brak C++ `new` w badanym wywołaniu DSP. Pełny CTest: 2/2 passed (DSP oraz zapis/odtworzenie parametrów, PDC 240 próbek przy 48 kHz i deterministyczny render bez edytora).

Test GUI przeszedł: trzy otwarcia edytora i trzy rozmiary (750×550, 1000×700, 2000×1400), niepusty rendering programowy na Xvfb/Openbox. To nie test HiDPI ani FL Studio. pluginval 1.0.3, strictness 5: kod wyjścia 0 na rzeczywistym VST3 Linux; opcjonalny zewnętrzny validator SDK nie został uruchomiony. Log: `build-plugin/pluginval.log`.

Benchmark `dsp_benchmark`: 16 pasm + limiter, syntetyczna sinusoida, pięć sekund audio na konfigurację; czas obejmuje również generowanie wejścia. Wyniki dla bufora 256: 44.1 kHz 102.1× realtime, 48 kHz 90.2×, 96 kHz 42.9×, 192 kHz 18.5×. Test jednowątkowy Release na tej maszynie podczas innych prac; nie jest pomiarem FL Studio ani gwarancją na Windows. Pełna tabela powstaje przez `./build-core/dsp_benchmark > build-core/benchmark.csv`.

Nie wykonano długotrwałych testów RAM ani porównań odsłuchowych. Guard C++ `new` nie obejmuje bezpośrednich wywołań malloc ani całego wrappera. Brak testów na rzeczywistych plikach WAV. Podane testy syntetyczne nie dowodzą jakości komercyjnej.
