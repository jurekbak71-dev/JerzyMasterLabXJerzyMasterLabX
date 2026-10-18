# 0.1.0 — prace rozwojowe, nie wydanie produkcyjne

Pierwszy rdzeń DSP i wrapper JUCE, testy CTest, projekt workflow Windows/MSVC/pluginval/pakowanie. Użytkownik potwierdził zrzutem udany workflow Windows dla b4405d0; kolejne rewizje wymagają własnego wyniku CI. Brak testu FL Studio. Nie publikowano release.

Otwarte wymagania Phase A: pełny graf modułów z ID/serializacją/instancjami, True Peak, pełne testy GUI/HiDPI i FL Studio, kalibracja analizy, spójne snapshoty parametrów między wątkami i weryfikacja CI Windows.

Kolejny etap: crossfade 20 ms przy zmianie kolejności bez resetowania DSP, zachowanie PDC w bypassie hosta i testy automatyzacji przy buforach większych niż hint hosta. Testy rdzenia pod ASan/UBSan: 2/2 passed.

Odczyt stanu waliduje schemat i komplet 52 parametrów przed zastosowaniem. Wadliwy lub nieobsługiwany preset zachowuje dotychczasowe ustawienia. Dodano testy pełnego roundtrip oraz porównania realtime/offline przy czterech sample rates.

Dedykowany Spectral Workspace zastąpił listę parametrów JUCE. Dodano edycję węzłów EQ, Q kółkiem, krzywe z aktualnych współczynników DSP, panel wybranego pasma, tooltipy i FFT 4096–32768 energii L/R z wygładzaniem. Analiza przesłanych materiałów Ozone 12 jest w OZONE12_REVIEW.md; pełna instrukcja i test porównawczy audio pozostają niezweryfikowane. To nadal rozwojowa część Phase A.
