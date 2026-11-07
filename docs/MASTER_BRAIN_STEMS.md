# MasterLabX — Master Brain Stem Remix, Windows x64

## Zasada działania
To jest rzeczywista separacja źródeł **poza wątkiem audio**, a nie filtr dzielący częstotliwości i nazywający pasma wokalem / basem. Do paczki instalacyjnej dołączono natywny silnik **stems.cpp v0.1.6** (MIT), model **HTDemucs v4** GGUF F16 oraz zachowano istniejący MasterLabX i jego procesory DSP.

Wtyczka:
1. **Stem Brain → SEPARATE WAV**: wskazujesz cały utwór WAV, a zewnętrzny proces stems-split wykonuje 4-stem inferencję HTDemucs offline. Nie dotyka wątku audio FL Studio; dane nie są wysyłane do usługi.
2. **LOAD FOUR STEMS**: alternatywnie wskaż folder z zsynchronizowanymi WAV: `drums.wav`, `bass.wav`, `other.wav`, `vocals.wav`; akceptowane są również nazwy wygenerowane przez HTDemucs z dopiskiem przed nazwą stema.
3. Po imporcie **4 stems replace DAW** podmienia sygnał wejściowy do aktualnego łańcucha MasterLabX. Wszystkie dalsze moduły (EQ, Dynamic EQ, Dynamics, Stereo, TruePeak) przetwarzają sumę remiksu.
4. Każdy stem ma: **GAIN**, **TONE**, **WIDTH**, **MUTE**, **SOLO**. Zakresy: gain -24 do +12 dB; tone -1 do +1 (prosty tonal tilt 1.8 kHz, nie wielopasmowy dynamic EQ); width 0 do 2.
5. **Natural balance / Vocal forward**: dwie zachowawcze propozycje proporcji stemów, wykonywane przez prawdziwe parametry hosta. Można je cofnąć przez Undo. `Vocal forward` daje +1.5 dB wokal, -1 dB bębny, -0.5 dB inne.
6. Wybór **DAW mix unchanged** natychmiast przywraca oryginalne wejście hosta; **DAW mix plus stems** to dodatkowe sumowanie, które może spowodować podwojenie i przester — używać celowo i kontrolować True Peak.

## Technika / ograniczenia
- Inferencja odbywa się w procesie dziecku `stems-split.exe`, nie w callbacku audio.
- System wymaga poprawnie rozpakowanego i zainstalowanego ZIP-a poprzez `INSTALL_MASTERLABX.cmd`; instalator kopiuje VST3 do Common Files/VST3 i model do ProgramData/JerzyMasterLabX/StemEngine. **Nie** doinstalowuje Pythona ani PyTorch.
- Stemy odtwarzane są zgodnie z bieżącą pozycją sampli DAW. Zmiana sample rate hosta korzysta z interpolacji liniowej; nie zmienia tempa.
- Bezpieczeństwo pamięci: źródła długości do 10 minut, łączny limit surowych danych około 950 MiB. Duże nagrania wymagają więcej RAM; import wykonuje się w UI, a inferencja w procesie offline.
- Separacja jest estymacją: może wystąpić przenikanie wokalu/perkusji, utrata ataku, artefakty i różnica między oryginalnym masterem a sumą stemów. **Wersja nie rekonstruuje oryginalnych śladów wielośladowych.**
- Może być konieczne ręczne otwarcie raz jeszcze folderu stemów po ponownym wczytaniu projektu w DAW.
- Proporcje stemów i łagodny tonal tilt służą rebalance. Nie są równoznaczne ze specjalistycznym AI de-clip, de-reverb, de-bleed czy de-noise. Istniejący Master Brain nadal oferuje pomiary LUFS, szczytów, stereo, A/B, dynamiczne procesory i limiter.
- Przy naprawie: najpierw rozpoznaj clipping (nie myl z celowym limitingiem), szum, kliknięcia, resonanse i fazę; nie zwiększaj pasma bez uprzedniego sprawdzenia śladów przecieku; na końcu mierz LUFS i **True Peak**, porównując warianty przy wyrównanej głośności. Nie deklaruj odzyskania informacji nieobecnych w oryginale.
- Dotychczasowe presety DSP pozostają zgodne: nowe parametry stemów dodano na końcu tabeli APVTS, stare ID nie zostały przenumerowane.

## Źródła (weryfikowane 2026-11-07)
- [HTDemucs v4, licencja MIT, model 4 źródeł](https://github.com/facebookresearch/demucs)
- [stems.cpp v0.1.6 — natywne CPU/GPU i przykłady CLI](https://github.com/betweentwomidnights/stems.cpp)
- [Stems.cpp — dystrybucja i pochodzenie modeli](https://github.com/betweentwomidnights/stems.cpp/blob/main/docs/DISTRIBUTION.md)
- [iZotope RX 12 Music Rebalance: realne przypadki](https://www.izotope.com/en/learn/stem-separation-music-rebalance)
- [iZotope: zasady usuwania kliknięć, szumu, pogłosu i clipu](https://www.izotope.com/community/blog/how-to-clean-up-audio-and-remove-background-noise)
- [ITU-R BS.1770-5 — pomiar LUFS i True Peak](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I/en)
- [EBU R128 v5.0](https://tech.ebu.ch/publications/r128)

Prawo: kod stems.cpp MIT; model HTDemucs ma osobne pochodzenie i został nauczony także na danych z ograniczeniami badawczymi. Prawo do opracowywania danego nagrania i udostępniania wyników należy zweryfikować niezależnie.
