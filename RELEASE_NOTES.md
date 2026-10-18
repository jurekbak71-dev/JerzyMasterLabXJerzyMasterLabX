# 0.1.0 — prace rozwojowe, nie wydanie produkcyjne

Pierwszy rdzeń DSP i wrapper JUCE, testy CTest, projekt workflow Windows/MSVC/pluginval/pakowanie. Brak potwierdzonego pliku binarnego Windows i testu FL Studio. Nie publikowano release.

Otwarte wymagania Phase A: pełny graf modułów z ID/serializacją/instancjami, True Peak, GUI tests, kalibracja analizy, spójne snapshoty parametrów między wątkami i weryfikacja CI Windows.

Kolejny etap: crossfade 20 ms przy zmianie kolejności bez resetowania DSP, zachowanie PDC w bypassie hosta i testy automatyzacji przy buforach większych niż hint hosta. Testy rdzenia pod ASan/UBSan: 2/2 passed.

Odczyt stanu waliduje schemat i komplet 52 parametrów przed zastosowaniem. Wadliwy lub nieobsługiwany preset zachowuje dotychczasowe ustawienia. Dodano testy pełnego roundtrip oraz porównania realtime/offline przy czterech sample rates.
