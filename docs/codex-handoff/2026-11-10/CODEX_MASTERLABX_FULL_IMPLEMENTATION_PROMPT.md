# CODEX: MasterLabX — audyt regresji, przebudowa DSP/GUI, Windows VST3

> **Autoryzacja:** użytkownik wydał START w rozmowie 2026-11-10. START upoważnia do rozpoczęcia prac nad projektem, ale **nie** do scalania zmian do `main`, usuwania funkcji czy publikowania jako ukończonego produktu bez weryfikacji i akceptacji. Ten PR służy wyłącznie przekazaniu zlecenia. Nie jest dowodem uruchomienia Codexa.
>
> **Niezbędny materiał graficzny:** `MasterLabX_Codex_GUI_DSP_Package_2026-11-10.zip` (7 wygenerowanych obrazów, 4 zrzuty użytkownika, manifest). PNG **nie są dołączone do tego PR**; użytkownik musi dołączyć ZIP do zadania Codexa. Opisy i sumy SHA-256: `ASSET_HANDOFF_MANIFEST.md`.

## Źródła decyzji

- Repozytorium: https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX
- Notion, zatwierdzony przewodnik: https://app.notion.com/p/4085fed1901b817db779c41c0375f6bc
- Notion, Concept C i wymagania 10.11.2026: https://app.notion.com/p/4105fed1901b810e9472e11439017087
- Notion, Stem Brain Live: https://app.notion.com/p/4115fed1901b81c4a099d97f56279132
- PR #18 / #19 / #20: sprawdź faktyczny stan, powiązanie commitów i odrębne cele. Nie pracuj bezmyślnie na `main`.
- Jerzy REBALANCE: https://app.notion.com/p/4145fed1901b8186b2cad05a270f48b5 — **osobny projekt, poza zakresem MasterLabX**.

## Cel i kryterium zakończenia

MasterLabX ma być **rzeczywiście użyteczną, wysokiej jakości wtyczką do masteringu oraz obróbki na insertach**, z poprawnym DSP i GUI wszystkich modułów. Obecna wersja według użytkownika jest gorsza muzycznie, wizualnie i technicznie; **to zgłoszenie, nie niezależnie potwierdzona diagnoza**. Zakończenie zadania wymaga dowodów: pomiarów, testów regresji, rzeczywistych screenshotów i VST3 Windows x64 oraz ręcznej akceptacji użytkownika w FL Studio. Nie obiecuj „idealności”.

## A. Najpierw audyt — bez przypadkowych poprawek

1. Odczytaj `AGENTS.md`, kod C++/JUCE, CMake, zależności, licencje, PR #18/#19/#20, testy, realne CI i historyczne binaria. Znajdź commit odpowiadający wersji użytkownika; gdy niemożliwe, oznacz NIEZWERYFIKOWANE.
2. Odczytaj **wszystkie** aktualne wymagania w Notion, w tym szczegółowe wymogi Stereo Architect. Nie wymyślaj funkcjonalności na podstawie nazwy modułu.
3. Stwórz tabelę zatwierdzone / zaimplementowane / potwierdzone testem / brak / niezweryfikowane dla rzeczywistych ekranów i DSP: Console, EQ, Dynamic EQ, Dynamics, Stereo Architect, Stabilizer, Analog Exciter, Analog Color, Bass, Transient, Restoration, Final Limiter/True Peak, Inspector, Master Brain, Stem Brain Live i innych występujących w kodzie.
4. Reprodukuj błędy, ustal ich przyczyny i plan testów RED→GREEN; porównaj ze starszą stabilną wersją. Nie usuwaj działających funkcji. Zadbaj o zgodność VST3 IDs, APVTS, zapisów projektów, presetów, automatyzacji.

## B. Naprawa i walidacja DSP

1. Przegląd pełnego toru audio: kolejność, routing szeregowy/równoległy, sumowanie, gain staging, headroom, mono/stereo, korelacja i faza, DC, denormale, bezpieczeństwo realtime, unikanie locków i alokacji w callbacku, buffer size 1 / nietypowe wartości, Fs 44.1/48/96/192 kHz, PDC, wyłączanie i transport.
2. **Final Limiter / True Peak:** niezależne pomiary reconstructed/inter-sample peaks, limit ceiling dBTP, Drive, Release, lookahead, stereo-link, clipping, fixed/variable latency, real bypass. Nie myl sample peak z true peak; nie uznawaj samego odczytu UI za test. Sinus, impulse, intersample stress, multitone, silence, level steps, sample rates, losowe granice bloków. Ustal jawne tolerancje pomiaru względem referencji.
3. **Analog Exciter:** sprawić, aby Warmth i reszta nastaw wywoływały mierzalne, muzycznie użyteczne zmiany. Tryby Tube/FET/Tape muszą mieć rzeczywiście odrębne charakterystyki harmoniczne i dynamikę. Sprawdź współdziałanie istniejących modułów Harmonic Exciter / Analog Color (lub innych rzeczywistych bloków) zamiast zakładać, że gotowe jest ich połączenie. Centralne EXCITE to **makro intensywności harmonicznej zależne od pozostałych ustawień**, nie sam Gain. Zbadaj H2–H5, THD/THD+N, IMD, aliasing, spectrum, crossover, Wet/Delta, output, gain compensation.
4. Oversampling 2x/4x/8x tylko jeśli faktycznie zaimplementowany, poprawny, ze zbadanym aliasingiem i kompensacją latencji. Nie pokazuj aktywnego przycisku funkcji, której kod nie realizuje.
5. **Stereo Architect:** sprawdź niezależne decyzje w Notion; testy szerokości pasm, M/S, mono compatibility, korelacji, phase i crossover, ale tylko w zakresie rzeczywistych i zatwierdzonych funkcji.
6. **Master Brain/Stem Brain Live:** oddziel funkcje faktycznie zaimplementowane od planów, dopilnuj licencji AI, runtime deadline, underflow/overflow, fallback, crash przy odtwarzaniu stanu i zgodność FL Studio.
7. Waliduj mastering i insert osobno. Bez dynamicznie „wyglądających” atrap i bez głośniejszego wyjścia jako jedynej miary poprawy.

## C. GUI wszystkich modułów

1. Konsekwentnie wdrożyć zatwierdzony **Concept C**: blacha malowana proszkowo, metalowe kontrolki 3D, czytelne szkło/LCD i wyraźne niebieskie/zielone/czerwone LED; zachować odrębne zatwierdzone projekty danego modułu. Nie wystarczy przemalować starego układu.
2. **Analog Exciter:** użytkownik dostarczył layout „Analog Luxury” (osobny PNG referencyjny). Ogromne centralne metalowe EXCITE, wokół czytelne mniejsze kontrolki; Tube/FET/Tape, pomiary harmonicznych i wejścia/wyjścia, Wet/Delta, kompaktowy profesjonalny układ. Wszystkie przyciski muszą być realnie połączone z DSP; brak regresji APVTS.
3. Każdy oscyloskop i analizator w całym pluginie: prawdziwy sygnał/FFT, stabilny trigger, technicznie właściwe zakresy osi i jednostki, antyaliasing graficzny, ograniczenie rysowania do ramki (`clip region`), kontrola odświeżania i skoków przy resize/transport. Wyraźnie odróżnij waveform, spectrum i pomiar harmonicznych. Referencja Maximus służy **czytelności**, nie kopiowaniu UI.
4. Miernik gain reduction: wskazówka z rzeczywistego GR, zero bez ruchu, prawidłowa skala i kierunek, odpowiednie uśrednianie bez fałszywych odczytów.
5. Elastyczna geometria bez kolizji i ogromnych pustych pól. Screenshoty każdego modułu w 750×550, 1000×700 i 1400×1000 (o ile host faktycznie wspiera te rozmiary); stan sygnału, ciszy i bypass; kontrola HiDPI.

## D. Assety w paczce — ograniczenia

- `assets/analog_exciter_main_excite_knob.png`, `assets/analog_exciter_secondary_knob.png`: **statyczne obrazy jednego ustawienia**. Nie udają kompletnego atlasu 101 klatek. Opracuj rzeczywisty obrót/marker/LED zależny od parametru; może być osobna maska 3D i dynamiczne rysowanie.
- `assets/control_switch_active_reference.png`: brak stanów OFF, hover, focus, disabled.
- `assets/led_states_sprite_reference.png`: cztery LED na jednym arkuszu, wymagają precyzyjnego wydzielenia i weryfikacji.
- `assets/oscilloscope_appearance_reference.png`, `assets/harmonic_analyzer_appearance_reference.png`, `assets/gain_reduction_meter_appearance_reference.png`: zawierają **wrysowane przykładowe dane/wskazówkę**, których nie wolno renderować jako rzeczywistej telemetrii. Przygotuj puste ramki i nakładaj realne przebiegi/słupki/wskazówkę z DSP.
- `references/`: zrzuty użytkownika, wyłącznie materiał do porównań; nie kopiuj chronionych interfejsów.
- Kontrola PNG: RGBA, alpha, rozmiar, ostrość, marginesy, proporcje, brak artefaktów, checksum SHA-256. Osadź w istniejącym pipeline CMake/JUCE, bez sztucznego zwiększania rozmiaru artefaktu.

## E. Proces, testy, wydanie

1. Osobna gałąź implementacyjna, czytelne małe commity. Testy jednostkowe i integracyjne DSP; CTest, pluginval i aktualne CI. Dokumentuj deterministyczne wejścia/wyjścia, tolerancje i wyniki.
2. Zbadaj realne parametry techniczne (latencję, CPU, memory, streaming, reset stanu), testy hosta i bezpieczeństwo callbacku. Zrzuty z **rzeczywistego GUI** dla wszystkich ekranów, nie makiety.
3. Windows x64 Release VST3, kompletne zależności, instalator/ZIP, instrukcja, suma SHA-256, kontrole architektury PE, CRC oraz licencji; nie publikuj „produkcyjnego” bez zaliczenia kryteriów.
4. Test w rzeczywistym FL Studio na komputerze użytkownika — tylko jeśli wykonany, inaczej NIEZWERYFIKOWANE. Wymagaj ręcznej akceptacji brzmienia i wyglądu przed merge.
5. Notion: zapisuj faktycznie wykonane etapy, źródła i daty według zatwierdzonego przewodnika. Nie zapisuj sekretów; nie twórz nowych baz/struktur, nie łącz REBALANCE.

## Wymagany raport

**WYKONANO / ZWERYFIKOWANO / TESTY / NIEZWERYFIKOWANE / NIEWYKONANE / ŹRÓDŁA.** Podaj konkretne pliki/linie, commit, CI, logi, screenshoty, wyniki pomiarów. Zatrzymaj się i zgłoś blokery, jeżeli nie możesz udowodnić jakości. **Nie scalaj do main bez odrębnego pozwolenia.**
