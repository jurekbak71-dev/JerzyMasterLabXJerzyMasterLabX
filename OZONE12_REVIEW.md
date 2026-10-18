# Ozone 12 — analiza punktu odniesienia i konsekwencje dla MasterLab X

Data: 18 października 2026. To analiza materiałów dostarczonych przez użytkownika i własnego kodu, nie test odsłuchowy Ozone. Nie deklarujemy zgodności algorytmów, jakości brzmienia ani przewagi.

## Źródła i ich granice

- **A:** przesłany tekst ogólnej strony Ozone 12 — opis edycji Elements/Standard/Advanced, modułowego huba i Master Assistant.
- **B:** przesłany tekst strony opisanej jako Ozone 12 Standard / Features — lista procesorów i narzędzi. Występują w nim rozbieżne liczby 14/16/20, więc nie służy jako rozstrzygająca macierz uprawnień edycji.
- **C:** przesłany tekst strony Ozone 12 Advanced — deklaruje 20 narzędzi, opisuje Bass Control, Stem EQ, Unlimiter, IRC 5 oraz Custom Master Assistant.
- **D:** przesłany obraz trzech paneli Ozone 12 — widoczny łańcuch modułów, panel asystenta i Maximizer. Obraz ma ograniczoną rozdzielczość; nie wyciągamy wniosków o nieczytelnych detalach i ukrytych menu.
- **E:** właściwy adres podręcznika wskazany przez użytkownika: https://docs.izotope.com/ozone12/en/index-en.html . Próba odczytu nadal zwraca 403. Nie przeczytano jeszcze pełnej instrukcji ani nie uruchomiono triala.

Opinie użytkowników, cytaty realizatorów i marketingowe obietnice braku zniekształceń nie stanowią pomiaru DSP. Pełna tabela edycji i szczegółowe zachowanie każdego trybu wymagają podręcznika lub testu programu.

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

Nadal brakuje pełnego grafu i wielu instancji, rzeczywistego panelu dynamiki/limitera z historią GR, True Peak, oversamplingu, LUFS/LRA, referencji, wyrównanego A/B, Undo/Redo, pełnych trybów EQ i imagera oraz asystenta. Nowy edytor EQ jest postępem w konkretnym module, nie odpowiednikiem kompletnego Ozone 12 Advanced. Udana kompilacja Windows nie rozstrzyga jakości odsłuchowej ani testów FL Studio.
