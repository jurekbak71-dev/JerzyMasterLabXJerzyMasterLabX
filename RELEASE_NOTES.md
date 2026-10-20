# 0.1.0 — prace rozwojowe, nie wydanie produkcyjne

## Master Brain: analiza i sprawdzanie wyniku DSP

60 komórek widma, trwałość rezonansów, adaptacyjne EQ/Dynamic EQ, pomiar poziomów sekcji, lepszy dobór kompresji/upward oraz Stereo. Brain wykonuje do sześciu próbnych renderów poza audio callback, mierzy pełny tor i koryguje głośność w granicach budżetu redukcji. Brak zmian parametrów przed Apply; zachowane Undo i ochrona ręcznych nastaw. [Opis, wyniki i ograniczenia](BRAIN_IMPROVEMENTS.md).

## W przygotowaniu — routing, Imager, Stabilizer

- Dowolna kolejność sześciu działających rodzin, 20 ms crossfade dwóch przygotowanych banków, stałe PDC, zapis/presety/Undo/automatyka.
- Imager: duże widmo z przeciąganiem podziałów, kolorowe pionowe Width, per-band On/Solo, Amount, prawdziwe Delta, półkolisty vectorscope wyjścia.
- Stabilizer: własna adaptacja 12 pasm, Shape/Cut, trzy jawne profile, Amount/Speed/Smoothing/Low/Mid/High/Tame/Delta, wykres działających filtrów.
- Brain automatycznie dobiera potrzebne rodziny i liczbę procesorów spośród dostępnych sześciu; Apply nadal wymaga akceptacji.
- Schema 5 (161 parametrów), migracja 1–4. Szczegóły i jawne ograniczenia: MODULAR_WORKFLOW.md.


Pierwszy rdzeń DSP i wrapper JUCE, testy CTest, projekt workflow Windows/MSVC/pluginval/pakowanie. Użytkownik potwierdził zrzutem udany workflow Windows dla b4405d0; kolejne rewizje wymagają własnego wyniku CI. Brak testu FL Studio. Nie publikowano release.

Otwarte wymagania Phase A: pełny graf modułów z ID/serializacją/instancjami, True Peak, pełne testy GUI/HiDPI i FL Studio, kalibracja analizy, spójne snapshoty parametrów między wątkami i weryfikacja CI Windows.

Kolejny etap: crossfade 20 ms przy zmianie kolejności bez resetowania DSP, zachowanie PDC w bypassie hosta i testy automatyzacji przy buforach większych niż hint hosta. Testy rdzenia pod ASan/UBSan: 2/2 passed.

Odczyt stanu waliduje schemat i komplet parametrów przed zastosowaniem. Wadliwy lub nieobsługiwany preset zachowuje dotychczasowe ustawienia. Dodano testy pełnego roundtrip oraz porównania realtime/offline przy czterech sample rates.

Dedykowany Spectral Workspace zastąpił listę parametrów JUCE. Dodano edycję węzłów EQ, Q kółkiem, krzywe z aktualnych współczynników DSP, panel wybranego pasma, tooltipy i FFT 4096–32768 energii L/R z wygładzaniem. Analiza przesłanych materiałów Ozone 12 jest w OZONE12_REVIEW.md; analiza przesłanej instrukcji została dopisana z odnośnikami do stron, test porównawczy audio pozostaje niewykonany. To nadal rozwojowa część Phase A.


19 października 2026: dodano Limiter Workspace z Drive 0–24 dB, Release 10–1000 ms, ceiling, rzeczywistą historią obwiedni GR i peak wyjścia oraz odczytem PDC. Nowe parametry są automatyzowane, wygładzane i zapisane w schema 2. Schema 1 migruje jawnie do ustawień Drive 0 dB/Release 80 ms. Pomiar 10 ms nie zależy od host buffer size; GUI nie steruje działaniem DSP. Nie dodano True Peak ani LUFS.

Walidacja tego etapu w Linux: Release VST3 zbudowany, CTest 7/7, test obu widoków GUI przy trzech rozmiarach i ponownych otwarciach, pluginval strictness 5 (exit 0), rdzeń ASan/UBSan 4/4. Nowa kompilacja Windows/MSVC i testy FL Studio nie zostały potwierdzone lokalnie.


Studio GUI: przebudowano oba widoki, typografię, nawigację modułów i kontrolki; rozdzielono osie historii limitera, powiększono odczyty output i GR, dodano karty parametrów oraz peak/RMS i pasek korelacji z istniejących pomiarów. Zachowano identyfikatory parametrów, stan i DSP. Testy lokalne: build Linux VST3, CTest 7/7, ponowne otwieranie i resize obu widoków, test przecięć kontrolek i kliknięcia modułu, pluginval strictness 5 — zaliczone. Status Windows nowej rewizji wymaga wyniku Actions.

EQ schema 3: sześć typów filtrów na każdym z 16 pasm, płynny bypass pasma, krzywe aktualnej odpowiedzi i migracja schema 1/2. Dodano testy DSP filtrów, integralności wyborów i kontroli GUI. Linux CTest 8/8 oraz GUI zaliczone. HP/LP mają 12 dB/oktawę; nie dodano Linear Phase, Dynamic EQ ani regulowanego slope.

Dodano Undo/Redo, zapis i odczyt pełnych presetów `.jmlx` oraz testy plików, transakcji i odrzucania błędnych danych. Podział wrappera do `Processor.h`. Linux CTest 9/9, GUI i pluginval strictness 5 zaliczone.


Master Inspector: stałe okno 400 ms dla input/output peak/RMS/correlation/M/S/balance, headroom i licznik przekroczeń. LUFS M/S/I z ważeniem K i dwiema bramkami, histogram integrated 0,01 LU i reset bez zmiany DSP. Pomiar działa bez GUI. FFmpeg ebur128: 16 referencji przy 44,1–192 kHz, 48 odczytów I/M/S w tolerancji 0,11 LU. Nie dodano LRA ani True Peak i nie deklarujemy pełnej certyfikacji standardu. CTest 13/13, ASan/UBSan 8/8, trzy widoki GUI i pluginval strictness 5 na Linux zaliczone. Windows tej rewizji wymaga osobnego CI.


Master Brain: zaimplementowano analizę wejścia poza callback, regułowe propozycje EQ/limitera, panel konfiguracji i raportu, zatwierdzanie oraz jedną transakcję Undo. Zakres, algorytm i ograniczenia opisano w [MASTER_BRAIN.md](MASTER_BRAIN.md). Nie jest to kompletna docelowa implementacja v0.4.0.

Walidacja Master Brain w Linux: Release VST3, CTest 15/15, rdzeń ASan/UBSan 9/9, rzeczywisty test GUI (otwieranie, trzy rozmiary, analiza/Apply/Undo), pluginval strictness 5 — zaliczone. Build Windows jest weryfikowany oddzielnie w Actions.

Potwierdzony Windows/MSVC dla `144aa41`: workflow `39221591050` zakończony sukcesem, wraz z testami DSP, GUI i pluginval 1.0.3. Wygenerowany artefakt `JerzyMasterLabX-v0.1.0-Windows-x64` (ID `12172186866`, 3 304 975 bajtów). To rozwojowa paczka VST3 z Master Brain, nie wydanie o pełnym zakresie Ozone. Brak testu FL Studio i porównania brzmieniowego.

Master Brain — balans tonalny: dodano wykres wejścia względem celu gatunkowego oraz estymatę wpływu EQ z rzeczywistych charakterystyk filtrów, liczby energii w raporcie i test zgodności z renderowanymi sinusami przy czterech sample rates. Wykres pokazuje pięć regionów, nie pełną krzywą widmową; estymata nie obejmuje limitera. Linux Release VST3, CTest 15/15, GUI w trzech rozmiarach i pluginval strictness 5 zaliczone. Zaktualizowano analizę pełnych napisów obu filmów.

Soft Studio GUI: osadzony font Manrope, większe wartości, uproszczona hierarchia tekstu, wspólny panel parametrów, zaokrąglona nawigacja i moduły, przełączniki suwakowe. Strzałki list zastąpiono miniaturowymi okrągłymi przyciskami diodowymi z rzeczywistymi stanami hover/focus/menu. Licencja fontu dołączana do paczki Windows. Zmiany dotyczą GUI; identyfikatory parametrów i stan DSP zachowano.

Etap modułowy: Stereo Architect (4 pasma, mono/balance/korelacje), Smart Dynamics (Digital/Analog, downward/upward), 3-pasmowy Dynamic EQ, dodawanie/usuwanie i bypass pięciu procesorów, pełny wybór procesorów Brain, kolorowe krzywe wszystkich pasm Spectral EQ. Nowy stan schema 4 / 138 parametrów z migracją schema 1–3. Zakres i ograniczenia: MODULAR_WORKFLOW.md.

Walidacja lokalna etapu modułowego: Release Linux VST3, CTest 17/17, rdzeń ASan/UBSan 10/10, siedem paneli GUI / trzy rozmiary / trzy otwarcia, pluginval strictness 5 — zaliczone. Render rzeczywistego Linux VST3 z nowymi modułami: powtarzalność, bufory 256/1024, sample ceiling i zgodność LUFS-match zaliczone. Po wyłączeniu nowych modułów sześć WAV wcześniejszego testu jest identycznych bajt w bajt z wersją 579ab94. Windows tej rewizji wymaga osobnego wyniku Actions. FL Studio i ocena odsłuchowa pozostają niewykonane.
