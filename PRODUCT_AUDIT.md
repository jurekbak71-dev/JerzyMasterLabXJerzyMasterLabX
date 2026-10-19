# Audyt produktu — Jerzy MasterLab X

Stan audytu: 18 października 2026. Obecny build jest prototypem inżynierskim. Nie spełnia jeszcze pełnego zakresu FIRST WORKING MASTER ani aspiracji produktu klasy profesjonalnego pakietu masteringowego. Pomyślna kompilacja, pluginval i testy deterministyczności potwierdzają określone własności techniczne, nie jakość brzmienia, użyteczność ani kompletność produktu.

## Aktualizacja po dostarczeniu materiałów

Użytkownik przesłał trzy teksty stron produktu i obraz paneli Ozone 12. Na ich podstawie powstał OZONE12_REVIEW.md. Poprawny wskazany podręcznik to https://docs.izotope.com/ozone12/en/index-en.html — nadal 403. Zaimplementowano dedykowany edytor EQ, gesty na wykresie, odpowiedź z faktycznych współczynników DSP oraz FFT 4096–32768 energii L/R. Poniższa tabela i lista problemów zachowują zapis wcześniejszego stanu audytowanego przy cfbf759; punkty 1–3 zostały częściowo lub w całości zaadresowane w zakresie opisanym w GUI_SPEC.md. Pozostałe wymagania produktu nadal obowiązują.

## Granice pierwotnego porównania z Ozone 12

Próby pobrania poniższych oficjalnych źródeł zostały zablokowane przez proxy (403). Nie przeprowadzono jeszcze zweryfikowanego przeglądu aktualnego Ozone 12 Advanced ani testu jego wersji demonstracyjnej. Ten dokument nie przedstawia listy jego funkcji jako sprawdzonej i nie deklaruje równoważności algorytmów.

Źródła do odczytu po udostępnieniu sieci:

- https://www.izotope.com/en/products/ozone.html
- https://www.izotope.com/en/products/ozone/features.html
- https://docs.izotope.com/ozone12/en/index.html

W przeglądzie trzeba ustalić edycję i wersję, źródła/zrzuty dla każdego wniosku, ekrany i interakcje, analizę wejścia/wyjścia, sposób pracy łańcucha, EQ, dynamikę, limiting, stereo, referencje, asystenta i ograniczenia realtime/offline. Materiały marketingowe nie stanowią dowodu jakości DSP; instrukcja nie zastępuje testu wtyczki.

## Fakty zweryfikowane w naszym kodzie

| Obszar | Obecny stan | Brak wobec zamówionej specyfikacji |
| --- | --- | --- |
| Główna powierzchnia pracy | GenericAudioProcessorEditor i ręcznie rysowany nagłówek | Dedykowane panele, hierarchia parametrów, obsługa myszy na wykresach, zoom 75–200%, Undo/Redo, A/B |
| Łańcuch | Dwie ustalone kolejności EQ/limiter z crossfade | Dodawanie/usuwanie/powielanie instancji, rzeczywisty model modułów, serializacja grafu |
| EQ | 16 filtrów bell stereo, interpolacja współczynników | Węzły drag, Q kółkiem, krzywe indywidualne i sumaryczna, półki/HP/LP/notch, M/S/L/R, solo/delta, dynamic/linear phase |
| Limiter | Sample peak, okno lookahead, stały release, końcowy clamp | True Peak, oversampling, adaptacyjny release, testy intersample peaks, THD/aliasingu i jakości odsłuchowej |
| Analiza | FFT 1024 ze średniej kanałów, output peak/RMS/correlation blokowe, punkty vectorscope | Input/output overlay, tryby L/R/M/S, kalibracja, peak hold, freeze, historia, true peak, LUFS/LRA i zgodność ze standardami |
| Mastering workflow | Automatyzacja, zapis parametrów, bypass i PDC | Referencje WAV, wyrównane głośnością A/B, porównanie ustawień, użyteczne presety, historia i Undo/Redo |
| Walidacja | Testy syntetyczne, wrapper, GUI smoke, pluginval; udany workflow Windows widoczny na zrzucie użytkownika | FL Studio/FLP/eksport WAV, długi odsłuch, pliki rzeczywiste, zgodność mierników z wektorami wzorcowymi |

## Konkretne problemy, które trzeba rozwiązać

1. **Widok EQ nie pokazuje charakterystyki EQ.** Linia FFT obrazuje próbki analizy; nie jest krzywą przenoszenia filtra. Użytkownik nie może na niej projektować korekcji. 16 pasm na przewijanej liście nie tworzy profesjonalnego edytora masteringowego.
2. **Ograniczona rozdzielczość basu.** FFT 1024 przy 48 kHz ma odstęp 46,875 Hz. Logarytmiczna oś zaczynająca się od 20 Hz nie zwiększa tej rozdzielczości. Powielanie najbliższego binu na wielu punktach wykresu nie daje wiarygodnej analizy subbasu.
3. **Suma kanałów może ukrywać energię.** Analizowana jest średnia L+R; przy L=-R widmo znika mimo niezerowego sygnału stereo. Trzeba jawnie rozróżnić L/R/M/S oraz sumowanie energii kanałów.
4. **Pomiar blokowy nie jest pomiarem loudness.** Obecne RMS i korelacja zależą od granic i długości bufora. Nie zastępują standaryzowanych okien czasowych, ważenia K, bramkowania, LUFS ani LRA.
5. **Ceiling nie jest True Peak.** Clamp próbek nie dowodzi braku szczytów międzypróbkowych. Nie oceniono słyszalnych skutków obwiedni gain ani zabezpieczenia clamp.
6. **Kolejność limiter/EQ może przekroczyć ceiling wyjścia.** Ostatni EQ może podbić sygnał po limiterze. UI musi jednoznacznie przedstawiać lokalny ceiling modułu i końcowe szczyty, bez obietnicy ochrony całego toru w dowolnej konfiguracji.
7. **Test niepustego okna nie bada użyteczności.** Nie dowodzi poprawności przebiegów analizy, ergonomii, działania gestów, HiDPI, zgodności z hostem ani braku nakładania elementów we wszystkich skalach.
8. **Poprawny zapis parametrów nie oznacza ukończonych presetów.** Brakuje biblioteki presetów, organizacji, porównywania, Undo/Redo i spójnych snapshotów dla całego przyszłego grafu.

## Własny projekt następnego etapu

Nie kopiować pikseli, ikon, nazw własnych algorytmów ani układów graficznych konkurencji. Zrealizować własny czytelny interfejs wynikający z czynności użytkownika:

- Pasek globalny: preset, Undo/Redo, A/B, bypass, wyrównanie głośności odsłuchu, jawna latencja.
- Łańcuch: faktyczne instancje modułów ze stabilnymi ID, dodawanie/usuwanie, bypass, drag i informacja o aktywnym panelu.
- Główna powierzchnia EQ: duży wykres, oddzielne widma wejścia/wyjścia, rzeczywista charakterystyka filtrów obliczona z tych samych współczynników co DSP, węzły pasm i kompaktowe szczegóły wybranego pasma.
- Inspector: czytelne poziomy wejścia/wyjścia, uczciwe jednostki i okna pomiaru, ostrzeżenia clippingu, historia. Nie wyświetlać atrap LUFS/True Peak.
- Panel limitera: gain wejścia, ceiling, release, bypass, rzeczywista redukcja i historia, a dopiero po implementacji i testach tryb True Peak/oversampling.
- Dane pomiarowe i DSP pozostają niezależne od otwierania GUI; liczenie FFT i grafiki poza callbackiem.

## Kryteria odbioru przed nazywaniem tego ukończoną v0.1

- Zweryfikowany przegląd oficjalnego Ozone 12 Advanced z przypisaniem wniosków do źródeł; oddzielna ocena własnego produktu.
- Dedykowany edytor zamiast listy GenericAudioProcessorEditor. Zrzuty przy 75/100/150/200% i testy gestów, ponownego otwierania oraz resize.
- Edycja EQ na wykresie rzeczywiście zmienia DSP i automatykę. Zgodność krzywej z pomiarem impulsowym/sinusoidalnym, w tym blisko Nyquista.
- Analizator nie gubi energii przeciwfazowej i ma sprawdzoną skalę oraz rozdzielczość.
- Dynamiczny łańcuch i wiele instancji przetestowane w zakresie stanu, automatyzacji, latencji i bezpieczeństwa realtime.
- Oceniony limiter: pomiary szczytów, zniekształceń, transjentów i odsłuch przy wyrównanej głośności. Bez deklarowania przewagi nad Ozone bez testów.
- Rzeczywisty artefakt Windows plus potwierdzone testy FL Studio opisane w FL_STUDIO_TESTS.md. Oddzielić status kompilacji od oceny produktu.

Aktualizacja 19 października 2026: powyższy audyt jest historyczny. Bieżący kod zawiera dedykowane GUI, sześć typów EQ, bypass pasm, Undo/Redo, presety oraz Master Inspector z oknem 400 ms i LUFS M/S/I (testy względem FFmpeg). Bieżący zakres i ograniczenia opisują README.md, GUI_SPEC.md i DSP_DESIGN.md.
