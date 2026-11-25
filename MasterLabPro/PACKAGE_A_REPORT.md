# Jerzy MasterLabPro — raport odbioru pakietu A

Data audytu: 2026-11-25. Gałąź: `masterlabpro-development`. Projekt: niezależna aplikacja deweloperska C++20/JUCE 8.0.6 w `MasterLabPro/`.

## WYKONANO

- Zaimportowano zaakceptowany pakiet A+C: 235 PNG, manifest główny i manifest źródłowy.
- Dodano niezależny walidator ścieżek, liczby plików, nagłówków PNG, wymiarów i SHA-256 oraz bramkę CTest.
- Zachowano rack 2×5 z dziesięcioma uniwersalnymi slotami, stabilnymi ID instancji, duplikatami Gain, wyborem, usuwaniem i reorderem.
- Dodano test braku alokacji w callbacku, wieloczęstotliwościowy stress test z nieparzystym rozmiarem bufora oraz test przepełnienia ograniczonej kolejki.
- Dodano test interakcji JUCE: prawy przycisk zeruje rzeczywisty parametr pełnym gestem hosta, bez menu; sprawdzono także wybór, duplikaty i reorder.
- Usunięto stale widma po `AnalysisHistory::clear()`; BEFORE/AFTER/audio DELTA i różnica poziomów dB pozostają rozdzielone.
- Zastąpiono deformujące skalowanie paneli renderowaniem nine-slice i usunięto zależny od kodowania symbol z podpisu DELTA.
- Dodano rzeczywiste renderery stanów: empty, 1×Gain, 10×Gain, sygnał GLOBAL, MODULE i DELTA LISTEN; skale 100%, 150% i 200%.
- Dodano benchmark CPU/RAM/PDC oraz osobne bramki ASan+UBSan i TSan.

## ZWERYFIKOWANO

- Patch wejściowy: SHA-256 `fd1aad9f39a00c3afe818cd6e2aaa9502c010a10c71a0c8ef132f24c04a9f456`; `git apply --check` zakończył się powodzeniem przed jednokrotną aplikacją.
- Manifest: 235 wpisów, 235 PNG, zero brakujących i dodatkowych plików, wszystkie hashe i wymiary zgodne.
- Callback audio: brak wywołań globalnego `new` w badanym przetwarzaniu; kolejka odrzuca nadmiar zamiast nadpisywać nieodczytane snapshoty.
- Routing: całkowity wynik dwóch Gainów jest przemienny, ale delta wybranego stopnia zmienia się o 6,0206 dB po reorderze, co potwierdza rzeczywistą zmianę kolejności wykonania.
- FFT pozostawiono na 4096/Hann/hop 1024. Pomiar nie wykazał uzasadnienia kosztu 8192/16384 dla pakietu A.
- Build Windows x64 oraz pełny odbiór artefaktów: patrz sekcje TESTY i ODNOŚNIKI po ostatecznym biegu dokładnego SHA.

## TESTY

- Lokalny Release CTest: 11/11 PASS.
- Lokalny ASan+UBSan z leak detection: 11/11 PASS.
- Lokalny TSan: 11/11 PASS.
- GitHub Actions ASan+UBSan i TSan dla `ba499b6` i `bf2182c`: PASS.
- Pierwszy strict Windows import run `45615336798` dla `a8667d7`: build i testy PASS, artefakty GUI/Smoke/A+C utworzone.
- Run `45617018346` odtworzył stack overflow testów na Windows: `AudioRack` ma 691152 B, `AnalysisHistory` 650488 B; obiekty testowe przeniesiono ze stosu na stertę w `bf2182c`.
- Windows CTest dla `bf2182c`: 20/20 PASS; build aplikacji, benchmark i publikacja artefaktów PASS.
- Odbiór sześciu screenshotów wykazał prawidłowe rozmiary i geometrię, lecz zbyt krótki sygnał testowy nie wypełniał pełnej historii heatmapy; generator dowodowy wydłużono do 10 s czasu symulowanego. Końcowy odbiór tej korekty jest wymagany na najnowszym SHA.

## WYNIKI POMIARÓW

Maszyna lokalna w środowisku wykonawczym, kompilacja GCC 14.2 `-O2`:

| Pomiar | Wynik |
|---|---:|
| FFT 2048 | 40,54 µs |
| FFT 4096 | 86,48 µs |
| FFT 8192 | 192,41 µs |
| FFT 16384 | 387,25 µs |
| Rack 10×Gain, 44,1 kHz / blok 257 | 34,02 ns/próbkę |
| Rack 10×Gain, 48 kHz / blok 64 | 37,43 ns/próbkę |
| Rack 10×Gain, 96 kHz / blok 511 | 34,22 ns/próbkę |
| Rack 10×Gain, 192 kHz / blok 1024 | 35,16 ns/próbkę |
| RSS procesu benchmarku | 2 707 456 B |
| PDC technicznego racka Gain | 0 próbek |

GitHub Windows x64 (`bf2182c`): FFT 2048/4096/8192/16384 = 55,27/88,12/191,19/411,71 µs; rack 10×Gain = 73,08–84,16 ns/próbkę; RSS 4 194 304 B; PDC 0 próbek.

Wyniki zależą od maszyny i nie są obietnicą wydajności na innym komputerze. Pomiar Windows jest publikowany jako osobny JSON CI.

## WYKRYTE I NAPRAWIONE BŁĘDY

1. Brak trwałej kontroli integralności assetów — dodano walidator i fail-fast w strict Windows CI.
2. `AnalysisHistory::clear()` pozostawiał stare widma — test RED (kod 12), reset do -120 dB, następnie GREEN.
3. Ramka panelu była rozciągana, deformując narożniki i refleksy — dodano test geometrii i nine-slice.
4. Podpis `LEVEL Δ dB` ulegał uszkodzeniu kodowania w PNG Windows — zastąpiono jednoznacznym `LEVEL DELTA dB`.
5. Nowe testy Windows przekraczały domyślny stos przez duże modele FFT/racka — potwierdzono rozmiarami obiektów i przeniesiono własność testową na stertę.

## NIEZWERYFIKOWANE

- Ręczny test urządzenia audio i odtwarzania WAV na fizycznej maszynie Windows.
- Długotrwały soak liczony w godzinach; wykonano krótkie, deterministyczne stress testy.
- Zachowanie w DAW/VST3: pakiet A dostarcza samodzielną aplikację deweloperską, nie wtyczkę VST3.
- Jakość dziewięciu przyszłych procesorów: zgodnie ze specyfikacją nie zostały rozpoczęte w pakiecie A.

## NIEWYKONANE

- Master Brain i STEMS — wyłączone z zakresu.
- Dziewięć docelowych procesorów — następny pakiet po akceptacji infrastruktury A.
- Merge do `main`, force-push i release produkcyjny — celowo niewykonane.

## ZMIENIONE PLIKI

- `.github/workflows/masterlabpro-g3.yml`
- `MasterLabPro/CMakeLists.txt`
- `MasterLabPro/assets/**`
- `MasterLabPro/src/AnalysisHistory.h`
- `MasterLabPro/src/MeasurementViews.h`
- `MasterLabPro/src/RackEditor.h`
- `MasterLabPro/src/RackLayout.h`
- `MasterLabPro/tests/assets.py`
- `MasterLabPro/tests/analysis_history.cpp`
- `MasterLabPro/tests/audio_rack.cpp`
- `MasterLabPro/tests/benchmark.cpp`
- `MasterLabPro/tests/gui_interaction.cpp`
- `MasterLabPro/tests/gui_render.cpp`
- `MasterLabPro/tests/layout.cpp`
- `MasterLabPro/tests/realtime.cpp`
- `MasterLabPro/tools/validate_assets.py`

## COMMITY

- `a8667d71f5dc1aceb90d97d0d27c3344dd44f4dc` — import i walidacja A+C.
- `ba499b68e700ce531f8acab9fc0ce9189a943146` — realtime, analiza, GUI, benchmarki i CI.
- `bf2182c4899813baf303f35fe9154c5cedc7c22f` — poprawka stosu testów Windows.
- Końcowy commit raportu, dowodu routingu i pełnej historii heatmapy — patrz najnowszy SHA gałęzi.

## ODNOŚNIKI DO ARTEFAKTÓW

- Import A+C, build Windows PASS: <https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45615336798>
- Bieg ujawniający i dokumentujący stack overflow: <https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45617018346>
- Ostateczny bieg: <https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45618494973>

## ŹRÓDŁA

- Kod i testy w tym repozytorium, gałąź `masterlabpro-development`.
- `MasterLabPro/README.md`.
- Notion: „Jerzy MasterLabPro — plan rozwoju i specyfikacja GUI 1.2”, strona `4235fed1901b81b08df0e02b4422a493`, odczyt 2026-11-25.
- JUCE 8.0.6, źródła przypięte przez `FetchContent`: <https://github.com/juce-framework/JUCE/tree/8.0.6>.
- Dokumentacja GitHub Actions artifacts: <https://docs.github.com/en/actions/using-workflows/storing-workflow-data-as-artifacts>.

## REKOMENDOWANY NASTĘPNY PAKIET

Po formalnej akceptacji pakietu A: pojedynczy pierwszy procesor produkcyjny wraz z jego DSP, stanem, automatyzacją, routingiem nieliniowym, testami audio i dedykowanym panelem A+C. Nie uruchamiać równolegle implementacji dziewięciu procesorów.
