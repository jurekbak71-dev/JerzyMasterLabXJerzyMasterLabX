# MASTERLABX — GŁÓWNE POLECENIE WYKONAWCZE
Wersja 1.0 | 2026-11-08 | Źródło: instrukcje użytkownika w rozmowie oraz projekt Notion
STATUS: użytkownik wydał hasło START. Ten dokument jest poleceniem pracy nad CAŁĄ wtyczką, nie tylko Audio Restoration.

## MISJA
Dostarczyć rzeczywiście działającą, wydajną i możliwie oryginalną technologicznie wtyczkę Windows x64 VST3 **JerzyMasterLabX**, pracującą jako MASTER i INSERT, z profesjonalnym DSP, LIVE STEMS, separacją offline, Master Brain, ANALOGIZER, poprawionymi Analog Color/Exciter, Dynamic EQ, restoration/denoiser i analityką BEFORE/AFTER. Za sukces uznaje się tylko udowodnione kryteria akceptacji; nie utożsamiać „CI green” z jakością odsłuchową ani poprawnym działaniem FL Studio.

## AUTONOMICZNY PROTOKÓŁ PRACY
1. Zawsze na początku sesji przeczytaj zatwierdzony przewodnik Notion https://app.notion.com/p/4085fed1901b817db779c41c0375f6bc (raz na sesję), następnie bieżący zapis projektu https://app.notion.com/p/4115fed1901b81c4a099d97f56279132 i stan GitHub (repo, PR, aktualny HEAD, CI). Bieżąca instrukcja użytkownika ma pierwszeństwo.
2. Bądź architektem C++20/JUCE/VST3, DSP, ML/ONNX, inżynierem audio real-time, mastering/restoration, GUI/UX, testerem wydajności i integracji. Korzystaj z odpowiednich dostępnych narzędzi i źródeł: GitHub, Notion, oficjalne JUCE/Steinberg/VST3/ONNX Runtime, badania DAFx/AES, EBU R128/ITU BS.1770, sprawdzone projekty open source i dokumentacja producentów. **Nie można literalnie przeszukać całego internetu**: szukaj maksymalnie szeroko, ale celowo, datuj źródła, porównuj implementacje, weryfikuj licencje kodu ORAZ wag modeli.
3. Przed modyfikacją analizuj rzeczywisty kod, zależności, parametry i stan zapisany w projektach FL Studio. Ustal konkretną przyczynę błędu, minimalną poprawkę, test regresyjny i plan walidacji. Nie używaj fikcyjnych testów, danych ani atrap DSP. Preferuj oryginalne rozwiązania tylko jeśli wykazują przewagę względem istniejących technik.
4. Zapisuj zmiany jako przeglądalne, małe commity na istniejącej gałęzi/PR. Testuj w CI Windows: CTest, integracja rzeczywistego ONNX, pluginval, test ładowania zależności DLL, deterministyczne benchmarki. Jeśli dostępny rzeczywisty host, testuj FL Studio i dokumentuj konfigurację; jeśli nie — oznacz test FL Studio NIEZWERYFIKOWANE.
5. W callbackie audio **zero alokacji, blokad, join, czekania, dysku, sieci, inferencji ONNX i nieograniczonych pętli**. Wątki pomocnicze: kontrola kolejek, priorytetów, resetów, underrun, opóźnienia PDC i transportu, jednorazowe przygotowanie. Bezpieczne przełączenia i crossfade bez trzasków, klików i utraty automatyzacji.
6. Zachowaj istniejące ID i indeksy parametrów APVTS, migrację stanu, projekty DAW, topologię starego routera oraz wsteczną zgodność presetów. Nowe parametry dopisuj wyłącznie na końcu tablicy i jawnie mapuj indeksy; testuj round-trip i migrację. Nie usuwaj istniejących działających funkcji bez analizy skutków.
7. Po każdym etapie podawaj WYKONANO, ZWERYFIKOWANO, TESTY (z konkretnymi wynikami), NIEZWERYFIKOWANE, NIEWYKONANE, ŹRÓDŁA. Nie ukrywaj porażek. Samodzielnie kontynuuj w obrębie bieżącego wywołania, ale nie obiecuj pracy w tle bez uruchomionej automatyzacji.

## ZAKRES I KOLEJNOŚĆ
### P0 — STAN BAZOWY I REPRODUKCJA
Przejrzyj HEAD, konfigurację, testy, workflow, artefakty oraz źródła i dokumenty. Osobno ustal funkcje w pełni działające, częściowo działające i brakujące. Zapisz mierzalne KPI. Zidentyfikuj ryzyka wydajności, jakości, ergonomii i licencji.

### P1 — STEMS LIVE W CIĘŻKICH PROJEKTACH FL STUDIO
Napraw brak startu po pierwszym kliknięciu, zatrzymujące się liczniki WET, niedostarczanie stemów pod obciążeniem i niesłyszalne regulatory. Zmierz rzeczywistą wydajność i jitter: inference time, max/percentyle, backlog, stale/missing, queue drops, epoch resets, underruns, CPU oraz latency. Przetestuj single/parallel modele, ONNX threads, przetwarzanie przy różnych rozmiarach bloków, sample rates i zmianach transportu. Oddziel dry fallback od faktycznego WET. Zapewnij bezpieczne zachowanie przy przeciążeniu i możliwość wybrania wariantu CPU/jakość, nie udawaj separacji, gdy aktualnie działa wyłącznie dry. Zweryfikuj jakość 4 stemów na prawdziwych utworach, porównaj z separacją offline.

### P2 — MASTER BRAIN: RZECZYWISTY ROUTER
Rozszerz 6-modułowy router (720 legacy permutations) na konfigurowalne 11 już obecnych rodzin z pięcioma dotychczasowymi post-chain: Harmonic Exciter, Analog Color, Bass Architect, Transient Sculptor, Final True-Peak; dodaj ANALOGIZER i AUDIO RESTORATION jako oddzielne opcjonalne węzły, jeśli uzasadnione. Zaimplementuj realne zmienianie kolejności i włączanie/wyłączanie (nie tylko przyciski GUI), prawidłowe latency alignment, równoległe/szeregowe warianty tam gdzie możliwe, automatyzację, migrowanie legacy routing, bezpieczny crossfade. Uczciwie oznacz niewymienialne pozycje, jeśli wymagane technicznie.

### P3 — NAPRAW ANALOG COLOR / HARMONIC EXCITER
Usunąć twardy przester pasm i nieskuteczne gałki harmonicznych. Osobno zmierzyć i poprawić krzywe Drive, Gain Compensation, Bias, Mix, Even/Odd, 4 band amounts; wykazać rzeczywiste H2/H3/H4 na FFT i brak niekontrolowanego clippingu. Wdrożyć odpowiednie anti-aliasing (ADAA lub oversampling) z kontrolą latencji i kosztu CPU. Używać naturalnych krzywych, gain-match A/B i null/delta. Weryfikować na sinusie, wielotonach i materiale muzycznym.

### P4 — ANALOGIZER (OSOBNY MODUŁ)
Cel: analogowe ciepło, harmoniczne i głębia z Tube/Transistor/Tape, de-fizz bez bezmyślnego lowpass oraz przestrzeń z kompatybilnością mono. Dla Tape wykorzystaj rzeczywistą wiedzę z https://github.com/jurekbak71-dev/JerzyTapeDriveVST3 (source/tapedrive_dsp.h i testy) i NIE kopiuj bez analizy. Dla masteringu domyślnie bez szumu, pęknięć, wow, flutter i dropouts; mogą być osobnymi opcjami kreatywnymi. Nieużywany węzeł jest wyłączony z aktywnego łańcucha i nie przetwarza DSP (pomijalny koszt sprawdzenia węzła); brak dodatkowej latencji przy nieobecnym procesorze. Crossfade bez trzasków, brak alokacji na audio thread, testy RMS/peak/aliasing/mono/stereo/CPU.

### P5 — DYNAMIC EQ, KOMPRESOR I MODUŁY MASTERING
Rozszerz Dynamic EQ z 3 do większej liczby faktycznych pasm, wstępnie 8 (liczba do zatwierdzenia/rewizji na podstawie wydajności). Nie pozoruj rozbudowy w GUI: rejestracja APVTS, algorytm, detektory, wizualizacje i Master Brain mają obsługiwać wszystkie pasma. Najpierw usuń obecne kosztowne przeliczanie dwóch biquadów na KAŻDĄ próbkę KAŻDEGO pasma, zamień na stabilne control-rate update i interpolację; żadne wyłączone pasmo nie obciąża DSP. Przy innych sekcjach: realne i bezpieczne filtry, dynamics, limiter, true peak, stereo architect, transient sculptor, stabilizer, bass architect, exciter i ich interakcje. Audio nie może być gorsze tylko dlatego, że głośniejsze.

### P6 — AUDIO RESTORATION / DENOISER
Dodać rzeczywisty moduł z opcjonalnymi niezależnymi sekcjami: De-Hiss (Learn, Noise Print, delikatne widmowe tłumienie i minimalizacja musical noise), De-Hum (50/60 Hz + harmoniczne), De-Click/De-Crackle (impulse detection and localized reconstruction), De-Clip (przybliżona rekonstrukcja, bez fikcyjnych obietnic), ewentualny model neuronowy jeśli CPU/licencja i jakość potwierdzone. Sterowanie Threshold/Reduction/Quality/Artifact Protect/Mix, odsłuch DELTA (usuwany sygnał), freeze i A/B. Unikać niszczenia wokali, talerzy i transjentów oraz modeli mowy stosowanych bez dowodów do muzyki. Oddziel tryb real-time INSERT od jakościowego offline/master high-latency i reportuj PDC. Wszystkie funkcje muszą faktycznie przetwarzać audio i mieć testy.

### P7 — TRYBY MASTER I INSERT
Jeden wspólny silnik, różne sprawdzalne profile: MASTER (LUFS, True Peak, LRA, final output, oversampling i opcjonalny lookahead) i INSERT (zachowanie transjentów, niższa latencja, niski CPU, ścieżki mono/stereo i miks kanałowy). Profile nie mogą dyskretnie nadpisywać ustawień; zgodność automatyzacji i projektów FL Studio. Latencję zgłaszać hostowi z testami kompensacji, nie zmieniać arbitralnie w callbacku.

### P8 — GRAFIKA I UX
Główna konsola: prawdziwe widma BEFORE (wejście) i AFTER (wyjście) w dwóch odróżnialnych kolorach, opcjonalne DIFFERENCE w dB i teoretyczny EQ RESPONSE osobno. Dynamic EQ: heatmap energii pasm/akcji gain w czasie. Kompresor: heatmap energii BEFORE/AFTER z osobną historią pełnopasmowej GR, bez udawania multiband, którego nie ma. Odczyty i kontrolki muszą być rzeczywistymi parametrami, nie atrapy. GUI czytelne, profesjonalne, skalowalne, ekonomiczne CPU: FFT poza callbackiem, rzadkie aktualizacje, brak obciążenia przy zamkniętym panelu.

### P9 — STEMS OFFLINE, INSTALATOR I WYDANIE
SEPARATE WAV musi faktycznie rozdzielać na DRUMS/BASS/OTHER/VOCALS, zapisać 4 poprawne pliki i umożliwić odtwarzanie w pluginie od 0:00; komunikaty o wymaganym silniku i modelach, błędy i licencje. Weryfikuj sumy pobrań i wersje zależności. Dostarcz instalator/ZIP Windows x64, VST3, model terms i instrukcję FL Studio, migrację starych projektów oraz sumy SHA-256.

## KRYTERIA AKCEPTACJI (NIE WOLNO OZNACZYĆ DONE BEZ DOWODÓW)
- Plugin instaluje się i jest widoczny jako efekt VST3 w rzeczywistym FL Studio; procesBlock/master/insert poprawnie audio; wszystkie parametry i przyciski działają.
- Każdy aktywny moduł DSP i gałka mają mierzalny wpływ na sygnał, a bezczynne węzły nie obciążają toru.
- LIVE dostarcza realne neuronowe WET bez zaniku w reprodukowalnych ciężkich projektach; pod obciążeniem czytelna degradacja/tryb bezpieczny, nie mylony z prawidłową separacją.
- Jednoznaczne testy routingu dowolnej dopuszczonej kolejności; stany i automatyzacja legacy nie utracone.
- Obiektywne FFT/THD+N/aliasing, true-peak, LUFS, latency, profile MASTER/INSERT, testy CPU oraz odsłuch gain-matched A/B. Zdefiniowane progi i udokumentowane wyjątki; bez wymyślonych wartości.
- DENOISER i De-Fizz wykazują redukcję rzeczywistego zakłócenia oraz kontrolę artefaktów na odpowiednim materiale testowym, a nie jedynie zmianę EQ.
- CTest, pluginval, CI Windows i testy hostowe; zachowanie po restartach, odtwarzaniu, zamykaniu, zmianach sample rate i ciężkim obciążeniu; ZIP z prawdziwym VST3 i checksumami.
- Notion i GitHub aktualizowane z dowodami i niepewnością. Każdy raport zawiera: WYKONANO / ZWERYFIKOWANO / TESTY / NIEZWERYFIKOWANE / NIEWYKONANE / ŹRÓDŁA.

## GRANICE
Nie deklaruj sukcesu na podstawie planu, prototypu, modelu niezintegrowanego, zmyślonych pomiarów czy samej kompilacji. Zapisz niepewność, gdy środowisko FL Studio nie jest dostępne. Nie kopiuj cudzych modeli bez analizy licencji i pochodzenia. Preferuj jakość użytkową, mierzalność i zachowanie stabilności nad dekoracyjną „innowacyjnością”.
