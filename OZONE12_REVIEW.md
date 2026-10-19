# Ozone 12 — analiza punktu odniesienia i konsekwencje dla MasterLab X

Data: 18 października 2026. To analiza materiałów dostarczonych przez użytkownika i własnego kodu, nie test odsłuchowy Ozone. Nie deklarujemy zgodności algorytmów, jakości brzmienia ani przewagi.

## Źródła i ich granice

- **A:** przesłany tekst ogólnej strony Ozone 12 — opis edycji Elements/Standard/Advanced, modułowego huba i Master Assistant.
- **B:** przesłany tekst strony opisanej jako Ozone 12 Standard / Features — lista procesorów i narzędzi. Występują w nim rozbieżne liczby 14/16/20, więc nie służy jako rozstrzygająca macierz uprawnień edycji.
- **C:** przesłany tekst strony Ozone 12 Advanced — deklaruje 20 narzędzi, opisuje Bass Control, Stem EQ, Unlimiter, IRC 5 oraz Custom Master Assistant.
- **D:** przesłany obraz trzech paneli Ozone 12 — widoczny łańcuch modułów, panel asystenta i Maximizer. Obraz ma ograniczoną rozdzielczość; nie wyciągamy wniosków o nieczytelnych detalach i ukrytych menu.
- **E:** przesłany przez użytkownika `Ozone_12_Manual_English.pdf`, 156 stron. Przeanalizowano rozdziały dotyczące obsługi, asystenta, EQ, Imagera, Maximizera, Stem EQ, Unlimitera i opcji pomiarowych; sprawdzono również ilustrację EQ. Poniższe odnośniki podają numer drukowany oraz numer strony pliku PDF (drukowany + 9). Nie uruchomiono triala. Adres internetowy podręcznika: https://docs.izotope.com/ozone12/en/index-en.html . Dostarczony PDF pozwala analizować instrukcję mimo wcześniejszej blokady dostępu online.

Opinie użytkowników, cytaty realizatorów i marketingowe obietnice braku zniekształceń nie stanowią pomiaru DSP. Instrukcja wyjaśnia zachowanie opisanych funkcji, ale nie zastępuje testu programu ani pełnej macierzy licencji poszczególnych edycji.

## Co wynika z materiałów

| Obszar Ozone 12 | Potwierdzenie w dostarczonym materiale | Wniosek dla własnego projektu |
| --- | --- | --- |
| Modułowy hub | A/C: własny łańcuch lub łańcuch zaproponowany przez asystenta; D: poziome karty modułów | Potrzebny faktyczny model instancji i wygodna nawigacja, nie tylko karty dwóch stałych procesorów |
| Custom Master Assistant | C: profile gatunkowe, własne referencje, cel LUFS, wybór modułów i siły przetwarzania; D: oddzielny ekran konfiguracji | Oddzielić planowanie od DSP. W MasterLab X zaczynać od jawnych heurystyk; sugestie stosować po akceptacji |
| Maximizer / IRC 5 | B/C: nowy tryb limitowania; D: duży wykres czasowy, gain, output level i True Peak | Zbudować własny limiter z pomiarami true peak, transjentów, zniekształceń i wyrównanym odsłuchem. Nie nazywać własnego algorytmu IRC 5 |
| Bass Control | B/C: energia niskiego pasma, punch i kontrola basu | Osobny procesor basu z mierzalnym działaniem i kontrolą mono, nie dowolny ogólny regulator gain |
| Stem EQ | C: korekcja m.in. wokalu, basu i perkusji w gotowym miksie | Wymaga rozdzielenia źródeł, zweryfikowanych modeli/licencji, budżetu obliczeń i jawnych artefaktów. Nie udawać tego zwykłym pasmowym EQ |
| Unlimiter | C: próba odtworzenia dynamiki wcześniej mocno skompresowanego audio | To odrębny problem estymacji/rekonstrukcji. Zwiększenie gain lub ekspander nie dowodzi odzyskania oryginalnej dynamiki |
| EQ i Dynamic EQ | B: kontrola detali, elementy HUD i reakcja pasm na dynamikę | Duży, użyteczny wykres z węzłami, parametrami wybranego pasma i zgodnością wizualizacji z DSP |
| Imager | B: kontrola szerokości i opisane funkcje Stereoize / Recover Sides; D: osobny moduł | Własny imager musi pokazać korelację, M/S, zgodność mono i zależność od częstotliwości; nie kopiować nazw/algorytmów konkurencji |
| Referencje i Match EQ | B/C: import referencji i dopasowanie tonalne | Potrzebne porównanie wyrównane głośnością, pomiary i kontrolowane propozycje; sam wykres FFT nie realizuje tego workflow |
| Barwa i dynamika | B: Dynamics, Exciter i procesory Vintage | Własne moduły dopiero po ukończeniu fundamentu. Każdy wymaga rzeczywistego DSP i testów, a nie pustego panelu |

## Wygląd i ergonomia widoczne na obrazie D

1. Łańcuch jest stale widoczny na górze. Użytkownik widzi kontekst toru podczas pracy nad jednym modułem.
2. Większość powierzchni zajmuje zadanie aktywnego procesora, np. wykres czasowy limitera. To nie lista wszystkich parametrów naraz.
3. Mierniki są umieszczone w stałym bocznym obszarze, obok bieżącego procesora.
4. Najważniejsze parametry mają dużą czytelność; szczegóły są grupowane przy obszarze, którego dotyczą.
5. Asystent ma osobny etap wyboru celu i procesorów. Zrzut pokazuje przełączniki modułów i kontrolę intensywności, a nie tylko przycisk automatycznego masteringu.

Własny układ MasterLab X korzysta z tych ogólnych zasad ergonomii, ale nie kopiuje kompozycji pikselowej, ikon, fontów ani skóry Ozone.

## Co rzeczywiście poprawiono w MasterLab X

Usunięto GenericAudioProcessorEditor. Dedykowany Spectral Workspace zawiera wybór jednego z 16 pasm, własne kontrolki częstotliwości/gain/Q/ceiling, węzły drag, Q kółkiem, widmo oraz krzywą EQ i osobny obszar stereo. Węzły przekazują gesty automatyzacji hosta i zmieniają te same parametry co kontrolki.

Krzywa EQ jest liczona z kopii aktualnych, wygładzonych współczynników DSP przekazywanej przez prealokowane SPSC FIFO. Pokazuje chwilową odpowiedź liniowego modułu EQ, w tym jego blend bypassu; nie udaje odpowiedzi nieliniowego limitera ani całego toru. Krzywa wybranego filtra jest pokazana osobno. Nie oznacza to ukończenia wszystkich typów filtrów, dynamic EQ czy M/S.

FFT pracuje na energii L/R, więc sygnały przeciwfazowe nie znikają jak przy wcześniejszym L+R. Rozmiar 4096/8192/16384/32768 zmienia się wyłącznie w GUI. Przy 48 kHz FFT 8192 daje odstęp binów ok. 5,86 Hz; nie jest to rozdzielczość gwarantowana niezależnie od sample rate. Kalibracja dotyczy amplitudy sinusoidy względem okna Hann, a nie PSD ani pomiaru loudness.

## Nadal istotnie poniżej zamówionego produktu

Nadal brakuje pełnego grafu i wielu instancji, pełnego panelu dynamiki, True Peak, oversamplingu, LRA, referencji, wyrównanego A/B, pełnych trybów EQ i imagera oraz asystenta. Nowy edytor EQ jest postępem w konkretnym module, nie odpowiednikiem kompletnego Ozone 12 Advanced. Udana kompilacja Windows nie rozstrzyga jakości odsłuchowej ani testów FL Studio.


## Ustalenia z podręcznika E

### Obsługa i porównania

- **Str. 5–9 / PDF 14–18:** skalowanie okna, historia Undo, cztery migawki historii A/B/C/D, bypass i solo modułu, presety modułu oraz panel wejścia/wyjścia. Wyczyszczenie historii nie resetuje ustawień. To osobne operacje wymagające jednoznacznej obsługi.
- **Str. 7 / PDF 16:** menu zawiera osobne Equalizer 1 i Equalizer 2. Nie jest to dowód obsługi dowolnej liczby identycznych modułów. Własny wymóg wielu instancji MasterLab pozostaje niezależnym zadaniem architektonicznym.
- **Str. 117 / PDF 126:** Gain Match ma dwa zachowania zależne od opcji: dopasowanie odsłuchu bypass albo dopasowanie wyjścia przetworzonego do wejścia. Drugie zmienia poziom wyjściowy. W MasterLab trzeba jawnie rozdzielić odsłuch porównawczy i docelowy poziom eksportu; nie wolno ukrywać zmiany gain podczas renderowania.

### EQ: narzędzie pracy, nie tylko wykres

- **Str. 38 / PDF 47:** Analog to minimum-phase IIR, Digital to linear-phase FIR. Dodatkowa regulacja fazy i Surgical dotyczą Digital. Nasz biquad bell realizuje tylko fragment pierwszej kategorii.
- **Str. 39–40 / PDF 48–49:** osobny HUD wybranego pasma i widok wszystkich pasm. Krzywa sumaryczna i wybranego filtra mają różne role. Widmo opisano jako sygnał wyjściowy Ozone. Skala widma i skala gain EQ celowo się różnią; sam overlay nie dowodzi pomiaru wejście/wyjście konkretnego modułu.
- **Str. 41–44 / PDF 50–53:** odsłuch pasma Alt-Solo, dodawanie/usuwanie węzłów, blokada osi Shift, zmiana Q uchwytami, klawiatura, edycja numeryczna, bypass/solo pasma oraz różne rodziny filtrów. Dostępne są również wykresy fazy i opóźnienia grupowego. Nasze drag i Q kółkiem nie pokrywają tego zakresu obsługi.
- **Str. 113 / PDF 122:** okno FFT, uśrednianie, peak hold, skala częstotliwości i tilt zmieniają interpretację obrazu. Domyślny tilt 3 dB/oktawę spłaszcza różowy szum. Nie należy porównywać wysokości widm między programami bez wyrównania tych ustawień i normalizacji.

### Limiter: czego brakuje własnej implementacji

- **Str. 66 / PDF 75:** Learn Input Gain analizuje pięć sekund i ustawia gain do celu LUFS. Instrukcja wyraźnie odradza używanie go jako gwarancji zgodności loudness całego programu.
- **Str. 67–68 / PDF 76–77:** różne tryby IRC mają różne kompromisy CPU, latencji i reakcji na transjenty. IRC 5 opisano jako wewnętrzny podział na cztery pasma z odrębną reakcją czasową. Rozdział jednocześnie opisuje Maximizer jako moduł pełnopasmowy: nie oznacza to edytowalnego przez użytkownika multiband limitera. Opis produktu nie ujawnia kompletnego algorytmu.
- **Str. 68–70 / PDF 77–79:** osobne Gain i Output Level, True Peak, Character, Upward Compress, Soft Clip, Transient Emphasis oraz niezależność stereo dla transjentów i sustain. Wzmianka o oversamplingu 4× dotyczy Soft Clip; nie dowodzi stałego oversamplingu 4× całego Maximizera.
- **Str. 71 / PDF 80:** historia sygnału i redukcji/wzmocnienia pozwala oceniać reakcję procesora w czasie. Nasz limiter ogranicza sample peak, ma stałe 5 ms lookahead, regulowany release i drive oraz historię obwiedni GR; nie realizuje opisanych trybów, true peak ani wyrównanego porównania odsłuchowego.

### Stereo i źródło pomiarów

- **Str. 49–54 / PDF 58–63:** Imager ma do czterech pasm, crossover, solo/bypass pasma, historię korelacji i widmo szerokości. Width reguluje składową Side; nie tworzy stereo z identycznych L/R bez dodatkowego przetwarzania.
- **Str. 52 / PDF 61:** vectorscope pokazuje sygnał po całym torze niezależnie od pozycji Imagera. To ważna różnica między analizą modułu a analizą master output. Nasz obecny vectorscope również otrzymuje output, ale nie zastępuje analizatora per-band.
- **Str. 115 / PDF 124:** opcje obejmują ochronę przed przeciwfazą i wybór detekcji vectorscope. Sam ujemny chwilowy odczyt korelacji nie rozstrzyga o błędzie masteringu; potrzebny jest odsłuch mono i kontekst czasowy.

### Asystent i przetwarzanie źródeł

- **Str. 11–14 / PDF 20–23:** asystent potrzebuje audio; zalecane jest co najmniej osiem sekund głośnego fragmentu. Custom pozwala wybrać moduły, długość analizy do 60 sekund, intensywność, cel LUFS i poziom wyjściowy. Intensywność nie skaluje celu loudness. Cel uzyskany na fragmencie nie gwarantuje integrated LUFS całego utworu.
- **Str. 14 / PDF 23:** wybór nowego targetu po ręcznej edycji może nadpisać ustawienia. W naszym projekcie obowiązuje wymaganie użytkownika: propozycje Master Brain wymagają akceptacji; zachowanie Ozone nie znosi tego wymagania.
- **Str. 79–81 / PDF 88–90:** Stem EQ wykorzystuje separację ML na Vocals/Bass/Drums/Other i niezależną korekcję. Nie można zastąpić go zwykłym filtrem częstotliwościowym i zachować tej samej nazwy funkcji.
- **Str. 82–83 / PDF 91–92:** Unlimiter estymuje transjenty siecią ML; Amount reguluje rekonstrukcję. Nie daje matematycznej gwarancji odzyskania oryginalnego przebiegu po nieliniowym limitowaniu.
- **Str. 117 / PDF 126:** metering rozróżnia RMS, Peak, momentary 400 ms, short-term 3 s i integrated. MasterLab ma obecnie osobne LUFS M/S/I i pomiary RMS/peak w oknie 400 ms; ich zakres i ograniczenia walidacji opisano w DSP_DESIGN.md.

## Kolejność dalszych prac i kryteria odbioru

Poniższa lista jest planem brakujących funkcji, nie deklaracją ich implementacji.

| Priorytet | Zmiana w MasterLab | Warunek odbioru |
| --- | --- | --- |
| 1 | Samodzielny panel limitera: input drive, ceiling, release, historia GR i latencja | Automatyzacja i zapis stanu; historia pochodzi z rzeczywistej obwiedni DSP; poprawny pomiar impulsów i sinusów; zachowanie identyczne przy zamkniętym GUI |
| 2 | Pomiar wejścia i wyjścia oraz bezpieczne porównanie wyrównane poziomem | Zdefiniowane okna czasowe niezależne od bufora hosta; pomiar ciszy/mono/przeciwfazy; kompensacja opóźnienia; porównanie nie zmienia niejawnie eksportowanego mastera |
| 3 | Pełna obsługa pasma EQ: typ filtra, bypass, solo, edycja numeryczna, Undo/Redo | Krzywa zgadza się z pomiarem każdego typu; płynne przełączanie; migracja wcześniejszych zapisów; pojedynczy gest tworzy jedną operację Undo |
| 4 | True Peak i oversampling limitera | Jawna metoda interpolacji, testy sygnałów z peak między próbkami, pomiar aliasingu/opóźnienia i odsłuch przy wyrównanej głośności; dopiero potem etykieta True Peak |
| 5 | Inspector LUFS/LRA i historia | Testy zgodności z BS.1770/EBU, poprawne bramkowanie i reset; integrated mierzone na całym materiale, nie zgadywane z kilku sekund |
| 6 | Model instancji i rozwój stereo | Dodawanie/usuwanie/wiele instancji bez blokowania callback; stan i automatyzacja identyfikują instancję; szerokość i korelacja per-band oraz odsłuch mono |

Profesjonalna ocena brzmienia wymaga własnych pomiarów i legalnie dostępnego Ozone uruchomionego na tych samych plikach. Analiza instrukcji ustala funkcje i ergonomię; nie mierzy transparentności, artefaktów ani przewagi dźwiękowej.

Aktualizacja 19 października 2026: podstawowa implementacja priorytetu 1 jest dostępna (panel, Drive, Release, ceiling, historia GR i PDC). Nie oznacza to jakości ani kompletu funkcji Maximizera. Dokładne znaczenie obwiedni i ograniczenia pomiaru opisano w DSP_DESIGN.md.

Dalsza aktualizacja: dodano sześć typów filtrów, bypass pasma, Undo/Redo, presety plikowe i Master Inspector z LUFS M/S/I porównanym z FFmpeg. Nadal brak pełnej walidacji EBU, LRA, True Peak, dynamicznego grafu i pozostałych modułów.


## Dodatkowy materiał wideo i bieżący asystent

Przejrzano próbki obrazu z przesłanego filmu „What’s new in Ozone 12” (około 7 minut). Widoczne są konfiguracja celu i modułów, analiza oraz podgląd wyników tonalnych/głośności. Nie analizowano narracji ani porównań odsłuchowych. Master Brain wykorzystuje ogólny przebieg konfiguracja → analiza → przegląd → zatwierdzenie, z własnymi regułami i interfejsem. Nie implementuje pokazanych Stem EQ, Bass Control, Unlimiter ani IRC 5. Szczegóły: MASTER_BRAIN.md.

### Szczegółowy przegląd obrazu filmu producenta

Przejrzano plansze z klatkami co 4 sekundy przez cały materiał (7:01). Znaczniki poniżej są przybliżone; nie przeprowadzono odsłuchu ścieżki audio.

| Czas | Widoczne działanie | Wniosek dla MasterLab X |
|---|---|---|
| 0:52–1:24 | Stem EQ: wybór Vocals/Bass/Drums/Other, osobne kolorowe widma, węzły i poziomy stemów | Oddzielny procesor wymaga rzeczywistej separacji; zwykły EQ miksu go nie zastępuje |
| 1:48–2:44 | Bass Control: Balance, Punch, cutoff, zoom przebiegu, Sustain Power i Peak Control z progami | Docelowy Bass Architect powinien łączyć analizę przebiegu i niezależne sterowanie poziomem, atakiem i sustain |
| 2:48–3:36 | Unlimiter: Amount, input/output gain, Learn Threshold, przebieg i linia progu | Rekonstrukcja transjentów to osobny problem DSP, nie ujemny Drive limitera |
| 3:44–4:32 | Maximizer: wybór IRC 5, gain, output, True Peak, historia, Upward Compress, Soft Clip, Transient Emphasis, Stereo Independence | Obecny sample-peak limiter ma znacznie mniejszy zakres; potrzebna niezależna walidacja TP, zniekształceń i transjentów |
| 4:36–5:16 | Custom Assistant: lista celów i ulubionych, wybór modułów, intensity, LUFS/output/TP, czas w taktach, uruchomienie analizy | Zachować wybór uczestniczących modułów i konfigurację przed analizą; obecne sekundy nie są synchronizacją do taktów |
| 5:20–6:04 | Wyniki: Tonal Balance, historia Loudness, Vocal Balance, Dynamics Match, Width Match, Clarity i Stabilizer, bypass/relearn | Raport tekstowy i pięć korekt nie stanowią pełnego odpowiednika; potrzebny graficzny widok celu i pomiarów oraz rzeczywiste procesory |

Wspólna kompozycja widoczna w demonstracjach: poziomy łańcuch modułów nad dużą przestrzenią wykresu, stale dostępne mierniki I/O z prawej, główne kontrolki wokół wykresu, narzędzia odsłuchu Delta/Bypass/Gain Match. To kryteria ergonomii dla własnego interfejsu; nie kopiujemy grafiki ani oznaczeń algorytmów.

## Film recenzencki „Przerażająco funkcjonalny iZotope Ozone 12”

Przesłany plik ma 24:28, obraz 1280×720 i ścieżkę audio. Przejrzano obraz w odstępach 15 sekund przez cały film. Poniższe obserwacje pochodzą z ekranu, nie z transkrypcji ani odsłuchu. Próba uruchomienia lokalnej transkrypcji Whisper nie powiodła się: pobieranie publicznego modelu zostało zablokowane przez proxy HTTP 403. Nie przypisujemy autorowi wypowiedzi niewidocznych w obrazie.

| Przybliżony czas | Obserwacja ekranu | Wymaganie / konsekwencja |
|---|---|---|
| 0:30–1:00, 6:30 | Widok asystenta: obszar docelowego balansu tonalnego i krzywa materiału, osobny wykres loudness, lista gatunków, dodatkowe suwaki | Własny asystent potrzebuje graficznego porównania z celem; obecny wykres pięciu korekt nie jest takim porównaniem |
| 1:30–2:30 | Przejście między Master Rebalance, Stem EQ i EQ; osobne kategorie źródeł | Zachować wyraźne rozróżnienie EQ całego miksu od separacji źródeł |
| 8:00 | Plansza autora wyjaśnia, że używa Delta i nie prezentuje wtedy pełnego brzmienia Ozone | Własny Delta musi być jednoznacznie oznaczony i wyrównany w czasie; nie mylić sygnału różnicowego z wyjściem |
| 9:30–10:30 | Match EQ: osobne Capture/Clear dla Reference i Apply To, krzywa korekcji, Smoothing i Amount | Potrzebne prawdziwe przechwytywanie obu widm, regulacja dopasowania i możliwość odrzucenia wyniku |
| 11:00–11:15 | MP3 Preview, wybór bitrate i Solo Artifacts | Osobny zakres przyszłych narzędzi kontrolnych, nie element aktualnej implementacji |
| 11:30 | Dither: bit depth, amount, noise shaping, bit meter i DC offset | Nie deklarować ditheringu przez samo dodanie etykiety; wymaga DSP i testów widmowych |
| 11:45 | Reference: przebieg, oznaczone fragmenty A…H, gain i wybór pliku | Reference Match powinien mieć nawigację po fragmentach i jawne wyrównanie poziomu |
| 12:30–12:45 | Stem EQ: osobna korekcja Drums i Vocals | Zmiany źródeł wymagają licencjonowanego i zweryfikowanego modelu separacji |
| 13:30–17:15 | Bass Control: cutoff/zoom, Balance/Punch, niezależne Sustain Power i Peak Control | Priorytet rzeczywistego wykresu oraz niezależnych parametrów ataku/sustain |
| 15:15–15:30 | Plansza autora ostrzega przed ustawianiem basu przy aktywnym Gain Match | To stanowisko autora widoczne w filmie, nie dowód na wewnętrzny algorytm Gain Match; przy projektowaniu sprawdzić wpływ kompensacji na decyzje odsłuchowe |
| 16:30 | Plansza autora wiąże słyszane zakłócenia Delta z przemieszczaniem kontrolki i pracą bufora | Zaplanować testy ruchu parametrów, płynności i wyrównania torów Delta, bez zakładania przyczyny na podstawie samego filmu |
| 18:30–20:30 | Maximizer: True Peak, Character, Upward Compress, Soft Clip, Transient Emphasis, stereo independence | Braki obecnego limitera są funkcjonalne, nie kosmetyczne |
| 21:15–23:00 | Pomiar loudness i Unlimiter na mocno ograniczonym materiale, Amount/Threshold/Learn Threshold | Osobny procesor transjentów i testy na przesterowanym/ograniczonym materiale; nie obiecywać odzyskania utraconej informacji |
| 22:45 | Plansza informuje o wyrównaniu głośności odczuwalnej przed/po | Porównania jakości prowadzić przy wyrównanym LUFS i kontrolowanej latencji |

### Kryteria następnych zmian GUI i DSP

1. Stały prawy panel I/O i globalne narzędzia odsłuchu, niezależne od wybranego modułu.
2. Dominujący wykres roboczy z kontrolkami bezpośrednio powiązanymi z pomiarem.
3. Graficzne wyniki asystenta: pomiar i cel z jednostkami, propozycja oddzielona od zastosowanego DSP.
4. Bypass, Delta, mono i porównanie poziomu muszą mieć rzeczywiste tory sygnałowe z kompensacją latencji oraz jednoznaczny status.
5. Najpierw zweryfikować True Peak i jakość limitera; dodawanie pokręteł nazwanych jak w filmie nie dostarcza odpowiadających im algorytmów.

## Uzupełnienie po przeczytaniu całej polskiej transkrypcji

Źródło: dostarczone przez użytkownika automatyczne napisy „Przerażająco funkcjonalny iZotope Ozone 12 – test, analiza i opinia”. Przeczytano cały plik, następnie zestawiono go z wcześniejszym przeglądem obrazu. To analiza tekstu narracji, nie bezpośredni odsłuch ani własny test audio. Napisy nie mają znaczników czasu, zawierają błędy nazw i fragmenty tekstów piosenek. Czasy w poprzedniej tabeli pochodzą wyłącznie z obrazu.

### Co komentarz dodaje do obrazu

- Autor traktuje asystenta jako szybki punkt wyjścia, ale wskazuje też pełną obsługę ręczną. Profile gatunkowe są wyborem estetycznym, nie obiektywnym testem jakości. Nasz asystent musi pozostawić użytkownikowi decyzję i ręczną edycję.
- W demonstracji podnosi wokal, bębny i zmienia barwę wybranych źródeł gotowego miksu. To uzasadnia docelowy Stem Processor; zwykłe filtry M/S nie są separacją stemów. Wypowiedź o podobnych efektach M/S nie oznacza równoważności algorytmów.
- Opisuje nadrzędną wtyczkę i osobne procesory oraz komunikację instancji Match EQ. Nasza architektura powinna umożliwić niezależne wrappery, ale obecny produkt nie implementuje komunikacji między instancjami.
- Match EQ przechwytuje referencję i materiał docelowy, pozwala regulować głębokość i wygładzanie. Autor warunkuje naturalny efekt podobieństwem dynamiki obu materiałów. Nie należy obiecywać uniwersalnego dopasowania dowolnych utworów.
- Według komentarza można załadować do 10 referencji, dzielonych automatycznie lub ręcznie na sekcje. Limit i zachowanie należy potwierdzić instrukcją przed projektowaniem własnego importu; sam film nie jest specyfikacją implementacyjną.
- W Bass Control przebieg przedstawia niskie pasmo, a nie pełny miks. Podaje okolice 100–150 Hz jako typowy kontekst przykładu, nie obowiązkową częstotliwość podziału dla każdej muzyki.
- Kontrola basu musi być oceniana wraz z wpływem na limiter. Autor opisuje wzajemne oddziaływanie Balance, Punch, Sustain Power i Peak Control. Własne testy muszą mierzyć redukcję limitera po korekcji basu, a nie tylko odpowiedź wydzielonego pasma.
- Komentarz mówi o używaniu dopasowania poziomu do porównania przed/po, podczas gdy późniejsza plansza ostrzega przed ustawianiem basu z aktywnym Gain Match. Zachowujemy oba konteksty: decyzje tonalne podejmować także w pełnym torze przy stałym poziomie wejścia, a ocenę jakości wykonywać w kontrolowanym porównaniu z wyrównaną głośnością. Nie wnioskujemy, że każda kompensacja poziomu jest kompresorem.
- Autor opisuje Upward Compress jako podnoszenie cichszych elementów oraz omawia Soft Clip i nacisk na transjenty. To wymagania oddzielnych procesów DSP, nie alternatywne nazwy regulatora Drive.
- W Unlimiter podkreśla celowo ograniczony zakres kontrolek i potrzebę zapasu na pojawiające się transjenty (około 6 dB w przykładzie). Własny procesor ma pokazywać headroom i nie może obiecywać odtworzenia oryginalnych, utraconych szczytów.

### Jak oddzielamy opinię od weryfikowalnego wymagania

Oceny typu „najlepszy”, „naturalny” i „muzycznie brzmiący” należą do recenzenta. Nie stanowią wyniku naszych pomiarów ani podstawy do deklaracji równoważności MasterLab X. Podane w narracji 12–14 dB odstępu i ekstremalne −3,5 LUFS dotyczą opinii/przykładów; nie traktujemy ich jako uniwersalnych progów jakości. Crest factor (peak/RMS) i różnica peak/LUFS wymagają osobnych nazw i jednostek. Nie dodajemy celu −3,5 LUFS do automatycznych zaleceń na podstawie demonstracji.

### Kolejność rozwoju wynikająca z obu filmów i instrukcji

| Priorytet | Konkretna zmiana | Warunek uznania za wykonaną |
|---|---|---|
| 1 | Stały panel I/O, narzędzia odsłuchu i graficzne wyniki asystenta | Prawdziwe pomiary, czytelne jednostki, testy resize i zgodności wykresu z DSP |
| 2 | True Peak i rozwój limitera | Niezależne referencje pomiarowe, inter-sample stress tests, poprawny ceiling, testy latencji i zniekształceń przy 44,1–192 kHz |
| 3 | Stereo Architect i dynamika | Rzeczywista obróbka M/S, odsłuch mono, pomiary korelacji, kompresja i testy automatyki |
| 4 | Bass Architect i kontrola transjentów | Wydzielone pasmo, rzeczywisty przebieg, testy interakcji z limiterem i odsłuch porównawczy |
| 5 | Reference/Match | Import WAV poza callback, pomiary obu materiałów, sekcje, wyrównane A/B, ograniczona i zatwierdzana korekcja |
| 6 | Stem Processor i rekonstrukcja dynamiki | Najpierw wykonalność, licencja modelu, koszt obliczeń i jawne testy artefaktów; osobne procesory z własnymi kryteriami |

Obecny Master Brain realizuje pomiar fragmentu i zatwierdzane heurystyki EQ/limitera. Nie realizuje pełnego Master Assistant Ozone, dopasowania szerokości/dynamiki, korekcji wokalu, Stem EQ ani nauczonego modelu.

## Pełne napisy filmu producenta

Przeczytano cały przesłany plik angielskich napisów „What's new in Ozone 12, your complete mastering suite”. Potwierdza on: separację źródeł w czasie rzeczywistym w Stem EQ; progi, Punch, Balance i wskaźniki sweet spot w Bass Control; sieci neuronowe do tworzenia transjentów w Unlimiter; nowy tryb IRC5; Custom Flow z wyborem gatunków/ulubionych lub profilu z referencji, celu LUFS, modułów, intensywności, czasu analizy oraz zapisu konfiguracji domyślnej. Po analizie można edytować łańcuch z widoku asystenta i modułów.

Deklaracje producenta o braku pompowania/zniekształceń i „odzyskaniu” transjentów są marketingiem, nie gwarancją dla każdego sygnału. Nie ujawniają algorytmu IRC5 ani wag modeli. MasterLab X nie wykorzystuje ich nazw do oznaczania własnego DSP. Docelowa rekonstrukcja transjentów będzie tworzyć estymatę; nie odwróci matematycznie nieodwracalnego ograniczania.

Konkretny krok implementacyjny po analizie obu filmów: Master Brain pokazuje zmierzony udział energii pięciu regionów, jawny profil docelowy gatunku oraz przybliżony balans po proponowanym EQ. Target pochodzi z tych samych reguł co propozycja; wykres nie ma imitować szczegółowej krzywej spektralnej, której obecny pomiar nie dostarcza.
