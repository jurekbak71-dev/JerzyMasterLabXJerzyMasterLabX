# Pomiary bieżące

Aktualizacja 19 października 2026: CTest 13/13 passed; ASan/UBSan rdzenia 8/8 passed; trzy widoki GUI oraz pluginval strictness 5 na Linux zaliczone. Test Inspector obejmuje C++ new w pełnym callbacku, pomiar bez edytora, reset bez zmiany audio/PDC i zmianę sample rate. Guard nadal nie przechwytuje bezpośredniego malloc ani zewnętrznych bibliotek hosta.

LUFS: 16 niezależnych wyników FFmpeg 7.1.5 ebur128 przy czterech sample rates, 48 odczytów I/M/S, tolerancja 0,11 LU; plik tests/data/loudness-reference.csv. Reprodukcja: `python tests/generate_loudness_reference.py --probe build-plugin/loudness_probe`. Generator nie zmienia CSV bez jawnego --output. Mierniki RMS/peak/M/S mają osobne testy przesuwanego okna i kalibracji. To nie komplet oficjalnych wektorów EBU ani pełna certyfikacja.

Benchmark rdzenia obu aktywnych torów (bez nowych mierników wrappera), bufor 256, pięć sekund syntetycznego audio: 44,1 kHz 32,9× realtime; 48 kHz 28,7×; 96 kHz 15,1×; 192 kHz 6,44×. Wyniki ze współdzielonej maszyny i różnego obciążenia nie dowodzą przyspieszenia kodu względem poprzedniej sesji ani wydajności na Windows. Brak pełnego benchmarku CPU/RAM wrappera i długiej sesji FL Studio.

## Historyczne wyniki wcześniejszych rewizji

Bieżąca maszyna: Linux, GCC 14.2.0, CMake 4.4.4, Release. CTest: 1/1 passed, test obejmuje ceiling, opóźnienie impulsu, deterministyczność bloków 4096 vs 37, NaN/Inf przy 44.1/48/96/192 kHz oraz gain EQ przy 1 kHz. Dodatkowo testowana jest suma dwóch pasm (+12 dB) i brak C++ `new` w badanym wywołaniu DSP. Pełny CTest po dodaniu edytora i analizy: 6/6 passed (DSP oraz zapis/odtworzenie parametrów, PDC 240 próbek przy 48 kHz i deterministyczny render bez edytora; ponadto dokładny crossfade między rozgrzanymi torami, stała latencja, reset i niezależność od segmentacji bloków). Test wrappera obejmuje automatyzację kolejności, rozmiary bufora 1/32/257/4097 i bypass hosta przy długości bufora 8193, mimo hintu 32. Testy rdzenia z ASan/UBSan: 2/2 passed, bez zgłoszeń sanitizerów. Guard C++ new obejmuje również RoutedEngine podczas przejścia.

Test GUI przeszedł: trzy otwarcia edytora i trzy rozmiary (750×550, 1000×700, 2000×1400), niepusty rendering programowy na Xvfb/Openbox. To nie test HiDPI ani FL Studio. pluginval 1.0.3, strictness 5: kod wyjścia 0 na rzeczywistym VST3 Linux; opcjonalny zewnętrzny validator SDK nie został uruchomiony. Log: `build-plugin/pluginval.log`. Pierwsza równoległa walidacja została zakłócona przez współdzielony Xvfb; helper ma teraz blokadę flock, sprawdza uruchomienie własnych procesów i czeka na ich zakończenie. Powtórna walidacja sekwencyjna zakończyła się kodem 0.

Benchmark `dsp_benchmark`: 16 pasm + limiter, syntetyczna sinusoida, pięć sekund audio na konfigurację; czas obejmuje również generowanie wejścia. Aktualny benchmark mierzy `RoutedEngine`, a więc oba stale aktywne tory. Wyniki dla bufora 256: 44.1 kHz 19.7× realtime, 48 kHz 21.0×, 96 kHz 10.8×, 192 kHz 5.6×. To koszt świadomego utrzymywania rozgrzanych torów; starsze wyniki pojedynczego Engine nie opisują aktualnego wrappera. Test jednowątkowy Release na tej maszynie podczas innych prac; nie jest pomiarem FL Studio ani gwarancją na Windows. Pełna tabela powstaje przez `./build-core/dsp_benchmark > build-core/benchmark.csv`.

Nie wykonano długotrwałych testów RAM ani porównań odsłuchowych. Guard C++ `new` nie obejmuje bezpośrednich wywołań malloc ani całego wrappera. Brak testów na rzeczywistych plikach WAV. Podane testy syntetyczne nie dowodzą jakości komercyjnej.

Walidacja stanu: odtworzenie wszystkich 52 parametrów, 14 wadliwych struktur XML (w tym brak/duplikat/nieznane ID, NaN/Inf, wartości poza zakresem, nieobsługiwana wersja), nieprawidłowy/ucięty/zbyt duży blok binarny. Każdy wadliwy preset zawiera również poprawną zmianę innego parametru, aby wykryć częściowe zastosowanie danych. Przywrócony stan daje bitowo identyczny render w trybie realtime i offline przy 44.1/48/96/192 kHz i blokach 1/257/4097, z hintem 32. Render nie zmienia parametrów. Jest to test wrappera JUCE, nie test eksportu FL Studio.

## Dedykowany edytor EQ i analiza

Dodano testy response (20 kombinacji sample rate/częstotliwości, porównanie krzywej z pomiarem amplitudy audio i sprawdzenie rzeczywistego wygładzania współczynników) oraz analysis (amplituda Hann, energia jednego kanału i zachowanie sygnału L=-R przy wszystkich rozmiarach FFT). Nowe testy GUI uruchamiają pętlę komunikatów i sprawdzają kontrolki wybranego pasma, drag, kółko Q, begin/end gestów hosta, obecność rzeczywistej krzywej, resize/reopen oraz zmianę FFT bez zmiany latencji. Poprzedni test niepustego okna nie dawał tych gwarancji.

W bieżącym etapie: CTest 6/6 passed; ASan/UBSan rdzenia 3/3 passed; rozszerzony test GUI i pluginval strictness 5 zakończone kodem 0 na Linux. Test GUI faktycznie przetwarza analizę przy FFT 4096, 8192 i 32768, a test analizy sprawdza wszystkie cztery rozmiary. Nowy kod GUI wymaga osobnego potwierdzenia w workflow Windows oraz FL Studio.


Master Brain: zaimplementowano analizę wejścia poza callback, regułowe propozycje EQ/limitera, panel konfiguracji i raportu, zatwierdzanie oraz jedną transakcję Undo. Zakres, algorytm i ograniczenia opisano w [MASTER_BRAIN.md](MASTER_BRAIN.md). Nie jest to kompletna docelowa implementacja v0.4.0.
