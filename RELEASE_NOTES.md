# 0.1.0 — prace rozwojowe, nie wydanie produkcyjne

Pierwszy rdzeń DSP i wrapper JUCE, testy CTest, projekt workflow Windows/MSVC/pluginval/pakowanie. Użytkownik potwierdził zrzutem udany workflow Windows dla b4405d0; kolejne rewizje wymagają własnego wyniku CI. Brak testu FL Studio. Nie publikowano release.

Otwarte wymagania Phase A: pełny graf modułów z ID/serializacją/instancjami, True Peak, pełne testy GUI/HiDPI i FL Studio, kalibracja analizy, spójne snapshoty parametrów między wątkami i weryfikacja CI Windows.

Kolejny etap: crossfade 20 ms przy zmianie kolejności bez resetowania DSP, zachowanie PDC w bypassie hosta i testy automatyzacji przy buforach większych niż hint hosta. Testy rdzenia pod ASan/UBSan: 2/2 passed.

Odczyt stanu waliduje schemat i komplet parametrów przed zastosowaniem. Wadliwy lub nieobsługiwany preset zachowuje dotychczasowe ustawienia. Dodano testy pełnego roundtrip oraz porównania realtime/offline przy czterech sample rates.

Dedykowany Spectral Workspace zastąpił listę parametrów JUCE. Dodano edycję węzłów EQ, Q kółkiem, krzywe z aktualnych współczynników DSP, panel wybranego pasma, tooltipy i FFT 4096–32768 energii L/R z wygładzaniem. Analiza przesłanych materiałów Ozone 12 jest w OZONE12_REVIEW.md; analiza przesłanej instrukcji została dopisana z odnośnikami do stron, test porównawczy audio pozostaje niewykonany. To nadal rozwojowa część Phase A.


19 października 2026: dodano Limiter Workspace z Drive 0–24 dB, Release 10–1000 ms, ceiling, rzeczywistą historią obwiedni GR i peak wyjścia oraz odczytem PDC. Nowe parametry są automatyzowane, wygładzane i zapisane w schema 2. Schema 1 migruje jawnie do ustawień Drive 0 dB/Release 80 ms. Pomiar 10 ms nie zależy od host buffer size; GUI nie steruje działaniem DSP. Nie dodano True Peak ani LUFS.

Walidacja tego etapu w Linux: Release VST3 zbudowany, CTest 7/7, test obu widoków GUI przy trzech rozmiarach i ponownych otwarciach, pluginval strictness 5 (exit 0), rdzeń ASan/UBSan 4/4. Nowa kompilacja Windows/MSVC i testy FL Studio nie zostały potwierdzone lokalnie.
