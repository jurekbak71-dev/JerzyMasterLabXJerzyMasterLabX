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
