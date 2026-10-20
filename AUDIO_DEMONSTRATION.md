# Demonstracja rzeczywistego DSP i Master Brain

Cztery kontrolowane przypadki na prawdziwych śladach The Easton Ellises — Falcon. Każdy ma plik wzorcowy sprzed celowego zaburzenia, wersję z problemem, wynik niezmienionej propozycji Braina i mocniejszą korekcję ręczną. Wszystkie odsłuchowe WAV są wyrównane do **−28 LUFS-I** (tolerancja 0,15 LU). MP3 odtwarza **PRZED → BRAIN → RĘCZNIE**, z przerwami 0,75 s. Imager należy oceniać w stereo, najlepiej na słuchawkach.

Celowe problemy:

1. Bell +10 dB / 120 Hz / Q 0,7. Ręczny Spectral EQ: −10 dB w tym samym paśmie.
2. Bell +10 dB / 3,5 kHz / Q 2,5. Ręczny Dynamic EQ: range −12 dB, threshold −42 dBFS, Q 2,5, attack 0,1 ms, release 70 ms.
3. Środkowe z trzech identycznych powtórzeń perkusji ściszone o około 16 dB. Ręczny kompresor: ratio 6:1, threshold −32 dBFS, attack 0,1 ms, release 60 ms, upward maks. 8 dB. Następnie limiter Drive 12 dB / ceiling −6 dBFS kontroluje szczyty. Miara porównuje RMS tych samych fraz, nie naturalne różnice między stopą i werblem.
4. Side zwiększony 2,5× i dodatkowy przeciwfazowy ton 80 Hz. Ręczny Imager: Mono Bass, crossover 500 Hz, Low Width 0, Mid 0,7×, góra 1×. To usunięcie części problematycznego Side, nie odzyskiwanie utraconej informacji.

To świadomie mocna demonstracja funkcji, a nie uniwersalny preset masteringowy lub ślepy test jakości. Plik wzorcowy jest oryginalnym fragmentem bez dodanego problemu, nie profesjonalnym masterem. Wersje bez końcowego gainu są w `DANE_TECHNICZNE`, wraz z presetem Braina i pełnymi parametrami.

## Prawdziwy Brain — bez kopii algorytmu w Pythonie

`brain_demo` jest zbudowany z tej samej biblioteki co wtyczka. Tworzy prawdziwy `Processor`, wywołuje `startBrainUI`, przekazuje audio przez `processBlock`, odbiera wynik wątku `MasterBrain` i sprawdza, że analiza nie zmieniła żadnego parametru. Dopiero potem wykonuje `applyBrainUI`, zapisuje `BRAIN.jmlx` i pełne wartości parametrów do JSON. Ustawienia: 16 s, Rock/Transparent, intensity 1,5, target −16 LUFS, ceiling −1 dBFS, automatyczny wybór rodzin.

Python nie implementuje reguł Braina i nie wzmacnia jego propozycji. Przenosi dokładnie wszystkie 161 parametrów według nazw do **rzeczywistego VST3** uruchomionego przez Pedalboard, sprawdza odczyt i wykonuje render. Drugi, ręczny render jest jawnie oznaczony. Pomiary i odsłuchy nie ukrywają słabych wyników Braina. Jego propozycja nie jest taką samą operacją jak ręczne usunięcie znanej, celowo wprowadzonej wady.

## Lokalne wyniki Linux

Zmniejszenie odchylenia od wzoru w wybranej mierze (wartość dodatnia = bliżej wzoru):

| Przypadek | Miara | Brain | Ręcznie |
|---|---|---:|---:|
| Nadmiar basu | energia 60–200 Hz względem 500 Hz–2 kHz | 3,75 dB | 8,03 dB |
| Ostra góra | energia 2,8–4,3 kHz względem 500 Hz–1,8 kHz | −0,13 dB | 6,10 dB |
| Nierówna perkusja | różnica RMS trzech identycznych fraz | 0,82 dB | 12,15 dB |
| Przeciwfazowy bas | energia Side/Mid w 20–140 Hz | 7,44 dB | 18,78 dB |

**Brain nie naprawił ostrej góry w tej mierze**, a jego działanie na nierówną głośność jest słabe. Nie zmieniono reguł ani raportu, żeby uzyskać lepszy wynik. Miary opisują wskazane kontrolowane problemy, a nie ogólną jakość muzyczną. Nie przeprowadzono oceny odsłuchowej ani testu FL Studio. Pełny raport: [tests/data/audio-demonstration-linux.json](tests/data/audio-demonstration-linux.json). Windows ma własny raport w paczce po wykonaniu workflow.

Sprawdzenia: źródło z przypiętego commita i SHA-256; zgodność parametrów prawdziwego Braina i VST3; brak zmian przed Apply; PDC bez przesuwania plików; skończone próbki i brak klipu PCM; identyczność powtórzeń i buforów 256/1024; wyrównanie LUFS; co najmniej 1 dB zmniejszenia błędu w **ręcznej**, ukierunkowanej demonstracji. Nie wymaga się zwycięstwa Braina, nie stroi się go do testu. Limiter pozostaje sample-peak, bez gwarancji True Peak.

## Źródło i licencja

The Easton Ellises — Falcon, **CC BY-NC-SA 3.0**, https://creativecommons.org/licenses/by-nc-sa/3.0/. Fragment 6 s powtórzony trzy razy, fade 5 ms, po 0,5 s ciszy na brzegach. Źródłowe AAC 44,1 kHz; wynik stereo WAV 24-bit / 48 kHz nie przywraca danych utraconych w AAC. Zmiany: wycinek/sumowanie stemów, pętla, fade, resampling, gain, celowe zaburzenia i DSP. Wyłącznie użycie niekomercyjne, autorstwo i ta sama licencja; artyści nie firmują wtyczki. Atrybucja i źródło są również w paczce.

## Uruchomienie

```bash
source scripts/cloud-env.sh
"$MASTERLAB_CMAKE" -S . -B build-plugin
"$MASTERLAB_CMAKE" --build build-plugin --target brain_demo --parallel 2
/workspace/tools/audio-tests/bin/python tools/audio/render_demonstration.py \
  --plugin build-plugin/JerzyMasterLabX_artefacts/Release/VST3/JerzyMasterLabX.vst3 \
  --brain-tool build-plugin/brain_demo \
  --output /workspace/artifacts/masterlab-demonstration
```

Workflow Windows buduje i waliduje VST3 oraz eksportuje prawdziwy `brain_demo.exe`. Workflow **Actual Brain and DSP audio demonstration** pobiera oba artefakty z **tego samego builda**, wykonuje test i udostępnia paczkę **MasterLabX-DEMONSTRACJA-PRZED-BRAIN-RECZNIE**. Nie używa fikcyjnej binarki ani plików podgłośnionych zamiast DSP.
