# Testy FL Studio — wszystkie niewykonane

Wymagany Windows 10/11 x64, prawdziwy build VST3 i FL Studio. Zapisuj wersję hosta, build SHA, sample rate, bufor, parametry i wyniki.

1. Otwórz/zamknij edytor 100 razy; sprawdź resizing i HiDPI.
2. Zapisz FLP ze zmienionymi parametrami, zamknij host, otwórz i porównaj każdy parametr.
3. Zautomatyzuj wszystkie parametry; sprawdź trzaski, kolejność i bypass.
4. Porównaj render offline i realtime przy identycznym stanie początkowym.
5. Sprawdź eksport WAV i brak resetowania ustawień.
6. Zmierz PDC impulsem; sprawdź START/STOP, bufor 32–2048 i 44.1–192 kHz.
7. Uruchom wiele instancji oraz długie odtwarzanie, obserwuj CPU/RAM.

pluginval nie zastępuje tych testów. Testy GUI i jakości dźwięku pozostają obowiązkowe przed wydaniem.

Nowe przypadki do ręcznego wykonania w FL Studio (nadal niewykonane):

- [ ] Dodać/usunąć i bypassować każdy z pięciu procesorów podczas odtwarzania.
- [ ] Zapisać FLP z 138 parametrami, otworzyć ponownie i porównać render.
- [ ] Automatyzować upward, tryb Analog/Digital, podziały stereo i progi Dynamic EQ.
- [ ] Sprawdzić mono po zmianach szerokości i balansu.
- [ ] Otwierać/zamykać siedem paneli i zmieniać rozmiar.
- [ ] Zatwierdzić propozycję pięciu modułów Brain, użyć Undo, zapisać projekt.
- [ ] Sprawdzić stałe PDC 5 ms także po usunięciu wszystkich modułów.

## Do wykonania po aktualizacji routingu i Stabilizera

- [ ] Zmienić kolejność wszystkich sześciu rodzin, zapisać FLP, zamknąć i ponownie otworzyć; porównać ustawienia, kolejność i render.
- [ ] Automatyzować chainOrder i bypassy, sprawdzić PDC oraz odsłuch podczas zmian.
- [ ] Sprawdzić solo/Delta/Amount Imagera i Delta Stabilizera; upewnić się, że odsłuch pomocniczy nie pozostał przypadkowo w eksporcie.
- [ ] Brain: porównać liczbę i kolejność proponowanych modułów, Apply, Undo i ręczne wykluczenie rodziny.

Powyższa lista **nie została wykonana w FL Studio** w środowisku chmurowym.
