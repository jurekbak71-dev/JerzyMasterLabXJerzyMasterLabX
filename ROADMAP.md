# Plan

- Phase A: ukończyć i przetestować wszystkie wymagania v0.1.0 opisane w RELEASE_NOTES.md, zwłaszcza bezpieczne zmiany DSP i stan hosta; dopiero potem oznaczyć wydanie.
- v0.2: dynamika, stereo, dynamic EQ, bass i transient; pełne mierniki ze standardowymi wektorami testowymi.
- v0.3: kolorystyka analogowa, spectral repair, clarity, reference i analiza utworów.
- v0.4: wyjaśnialne sugestie Master Brain; weryfikacja wykonalności i licencji stem separation.
- v1.0: testy hostów, odsłuchy, CPU/RAM, audyt licencji i paczki instalacyjne.

Bez deklaracji przewagi brzmieniowej bez rzetelnych porównań.


Postęp bieżący: 16 pasm/sześć typów EQ i bypass, limiter z historią GR i kontrolą Drive/Release, Undo/Redo, pliki presetów, Inspector wejścia/wyjścia i LUFS M/S/I, testy alokacji callbacku i referencje FFmpeg. Wszystkie moduły v0.2–v0.4 pozostają osobnymi zadaniami implementacyjnymi, nie ukończonymi pozycjami.

Następne bramki Phase A: dynamiczny model instancji i routingu, walidacja całego toru na rzeczywistych WAV, długie sesje, FL Studio/FLP/PDC/offline WAV, audyt licencji. Rozwój jakości limitera: true-peak detection/control, oversampling, aliasing i testy odsłuchowe. Pomiary: rozszerzone wektory EBU, LRA i historia loudness. Dopiero potem oznaczenie pełnego wydania.


Master Brain: zaimplementowano analizę wejścia poza callback, regułowe propozycje EQ/limitera, panel konfiguracji i raportu, zatwierdzanie oraz jedną transakcję Undo. Zakres, algorytm i ograniczenia opisano w [MASTER_BRAIN.md](MASTER_BRAIN.md). Nie jest to kompletna docelowa implementacja v0.4.0.
