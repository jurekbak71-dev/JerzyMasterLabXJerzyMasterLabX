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
