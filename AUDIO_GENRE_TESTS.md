# Siedem porównań audio — MasterLab X

Każdy folder zawiera `PRZED.wav`, `PO.wav`, `PO_LUFS_MATCH.wav` oraz MP3 `ODSLUCH_PRZED_POTEM_PO_MATCH.mp3`. WAV: stereo 24-bit / 48 kHz. MP3 najpierw odtwarza PRZED, potem 0,75 s ciszy i PO z wyrównaną głośnością.

| Folder | Materiał |
|---|---|
| 01_sama_perkusja | Rzeczywisty stem Drums |
| 02_perkusja_i_bas | Suma rzeczywistych stemów Drums + Bass |
| 03_zespol_rock_z_wokalem | Oryginalny miks zespołu electro-rock z wokalem |
| 04_sam_wokal | Rzeczywisty stem Vox |
| 05_rock_bez_wokalu | Suma Drums + Bass + Other |
| 06_elektronika | Własny syntetyczny rytmiczny fragment |
| 07_elektronika_przestrzenna | Własny syntetyczny fragment przestrzenny / ambient |

Pierwsze pięć to **ten sam sześciosekundowy fragment** The Easton Ellises — **Falcon**, powtórzony trzy razy. Na brzegach zastosowano fade 5 ms i 0,5 s ciszy na początku/końcu; pliki mają 19 s. Materiał źródłowy to AAC 44,1 kHz, więc zapis WAV 48 kHz nie odtwarza danych utraconych wcześniej. Ostatnie dwa mają 22 s i powstają w syntezie skryptu.

Źródło: [stempeg, przypięty commit a41eaab](https://github.com/faroit/stempeg/blob/a41eaabb6761c9cdc3a079c45a1a2ccd575afea6/README.md#reading-audio). Nagranie i wersje po obróbce: **CC BY-NC-SA 3.0** — autorstwo The Easton Ellises, użycie niekomercyjne i zachowanie licencji. Szczegóły/atrybucja oraz lista zmian są także w pobieranej paczce. Własna synteza przykładów 06/07: CC0 1.0. Plik źródłowy jest pobierany z przypiętego commita i sprawdzany przez SHA-256; nie jest dodawany do repozytorium.

## Jak porównywać

1. Najpierw porównaj **PRZED** z **PO_LUFS_MATCH**, żeby oddzielić zmianę brzmienia od wzrostu głośności.
2. **PO** jest bezpośrednim wynikiem VST3; po renderze nie jest normalizowany ani ponownie limitowany.
3. Input przygotowano stałym gainem do peak −6 dBFS. Presety DSP ustawiono ręcznie oddzielnie dla materiałów; to nie jest test automatycznego Master Brain.
4. Ceiling −1 dBFS dotyczy szczytów próbek. Limiter nie jest True Peak; raport jawnie podaje szczyty międzypróbkowe. Plik MATCH po dodatkowym gainie nie jest objęty ceiling modułu.

Raport `measurements.json` zawiera wszystkie znormalizowane parametry, ręczne ustawienia, SHA-256 binarki/źródeł/WAV/MP3, LUFS-I, LRA, sample peak, True Peak, RMS, crest i korelację. Skrypt wymaga: skończonych próbek, braku klipujących próbek, prawidłowego PDC, powtarzalności renderu, zgodności buforów 256/1024 i LUFS match do 0,15 LU. Nie wykonuje oceny odsłuchowej i nie stanowi testu FL Studio.

## Powtarzalny render

Lokalny Linux:

```bash
/workspace/tools/audio-tests/bin/python tools/audio/render_genre_tests.py \
  --plugin build-plugin/JerzyMasterLabX_artefacts/Release/VST3/JerzyMasterLabX.vst3 \
  --output /workspace/artifacts/masterlab-genre-tests
```

Wymagane zależności przypina `tools/audio/requirements.txt`; na Linux można korzystać z systemowego ffmpeg. Windows: uruchom workflow **Seven audio comparisons WAV and MP3** i podaj ID udanego builda sześciomodułowej wersji VST3. Workflow pobiera **rzeczywistą zwalidowaną binarkę Windows**, renderuje w Pedalboard i publikuje artefakt **MasterLabX-7-testow-audio-WAV-MP3**. Raport w artefakcie opisuje platformę tego renderu. Do pobrania służy artefakt Actions, nie lokalne ścieżki z tej instrukcji.

Lokalne wyniki tego etapu: [tests/data/audio-render-genres-linux.json](tests/data/audio-render-genres-linux.json). Wszystkie siedem przypadków przeszło sprawdzenia; różnica powtórzeń i 256 vs 1024 wyniosła 0, a różnica LUFS po wyrównaniu nie przekroczyła 0,01 LU. Te wyniki dotyczą Linux; Windows ma własny raport w paczce po zakończeniu workflow.
