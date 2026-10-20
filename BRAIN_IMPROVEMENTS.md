# Master Brain — weryfikowane propozycje

Brain ma priorytet rozwojowy. Nie jest modelem uczenia maszynowego ani kopią algorytmów iZotope. Proponuje rzeczywiste ustawienia dostępnych procesorów i wymaga zatwierdzenia przyciskiem Apply. Analiza ani próbny render nie zmieniają parametrów hosta. Stare propozycje nie nadpisują ręcznych zmian; Apply jest jedną transakcją Undo.

## Co zmieniono

- Zamiast polegać wyłącznie na pięciu szerokich regionach: 60 komórek widma co 1/6 oktawy, energia L/R, lokalna prominencja i odsetek ramek z utrzymującym się nadmiarem. FFT dobierany do sample rate, okno Hann. Częściowo pokrywające się komórki FFT są integrowane z uwzględnieniem ich szerokości.
- Częstotliwość, Q oraz tłumienie EQ/Dynamic EQ wynikają z wykrytego problemu. Próg detektora korzysta z energii pasma. Osłony pokrycia widma, trwałości, crest i korelacji ograniczają pomylenie pojedynczego tonu, stopy lub przeciwfazowego basu z rezonansem. Są to heurystyki, a nie rozpoznanie intencji muzyka.
- Opadające widmo i odcięcie kodeka nie mogą bez ograniczeń zwiększać prominencji: ograniczono ekstrapolację poziomu sąsiedztwa, a powyżej 4 kHz zastosowano bliższych sąsiadów. Detekcja lokalnych pików wymaga wiarygodnego otoczenia; nie zapewnia identycznej czułości przy granicach widma.
- Dynamika: RMS w oknach 400 ms oraz sekcjach 3 s. Kompresja i bounded upward są proponowane przy dużym rozrzucie i sąsiednich zmianach poziomu, zamiast na podstawie wyłącznie crest całego fragmentu. Nie jest to standardowy pomiar LRA. Celowy cichy refren lub przerwa też mogą uruchomić regułę — użytkownik musi odsłuchać propozycję.
- Stereo wykorzystuje korelację i energię Side/Mid w czterech pasmach. Przy trwałej przeciwfazie niskich częstotliwości może poszerzyć zakres Mono Bass. Tłumienie Side nie rekonstruuje utraconego sygnału.
- W trybie automatycznym Brain pomija niepotrzebne rodziny. Nie nakłada domyślnie Stabilizera na celowane EQ. Nadal dostępna jest jedna instancja każdej z sześciu rodzin; nie implementowano dowolnej liczby instancji ani wszystkich modułów Ozone.

## Rzeczywisty preview DSP

Worker zachowuje maksymalnie pierwsze 16 s przechwyconego wejścia. Buduje oddzielny SerialGraph z pełnym snapshotem 161 parametrów i propozycją. Host i preview używają wspólnego `GraphConfiguration.h`, tego samego DSP i PDC. Wykluczone rodziny zachowują ustawienia wejściowe.

Po renderze mierzy LUFS-I, sample peak, rozrzut sekcji oraz p95 maksymalnej redukcji limitera w blokach 128 próbek. To pomiary fragmentu, nie gwarancja całego utworu. Przy dużych skokach poziomu porównuje ograniczony wariant mocniejszej kompresji i wybiera go tylko po zmierzeniu mniejszego rozrzutu. Drive limitera jest korygowany na podstawie zmierzonej głośności i budżetu redukcji (Transparent 4 / Analog 5 / Aggressive 7 / Atmospheric 3 dB). Jeżeli wejście jest za głośne, dozwolony Dynamics może zastosować jawne ujemne Makeup; bez ukrytej normalizacji. Budżet jest kryterium wyboru, nie twardą gwarancją każdego szczytu. Niezrealizowany cel zostaje ujawniony.

Maksymalnie sześć próbnych renderów; przerwanie sprawdzane co 128 próbek. Render i alokacje są poza callbackiem oraz poza mutexem snapshotu GUI. Callback nadal tylko kopiuje próbki do stałego FIFO. Przy 48 kHz bufor preview stereo zajmuje około 6,1 MB, przy 384 kHz około 49,2 MB, plus mierniki/FFT/graf. Analiza dłuższa niż 16 s raportuje ograniczenie zakresu preview. Przy błędzie weryfikacji Apply jest blokowane.

Wynik GUI odróżnia zmierzone pełne DSP od szacowanej odpowiedzi statycznego EQ. Wybór gatunku pozostaje orientacyjnym wykresem, nie uniwersalnym wzorcem jakości. Świadomie zrezygnowano z automatycznych podbić wynikających wyłącznie z tej krzywej.

## Walidacja

`brain_rules_tests`: trwały rezonans kontra krótki transjent, zdrowy sygnał, zmiany sekcji, reset analizatora, nieprawidłowe liczby, wykluczone moduły i unikalne ID. `brain_tests`: prawdziwy capture, preview zgodny z wyjściem Processor, przerwanie, brak alokacji C++ w callbacku, brak automatycznych zmian, rewizja, ochrona przed nieaktualną propozycją i Undo.

`render_demonstration.py`: cztery identyczne kontrolowane wady jak w poprzednim teście, bez poprawiania ustawień Braina. Pełne JSON, presety i WAV. Porównanie poziomów −28 LUFS oddziela korekcję od zwykłego pogłośnienia. Zachowane stare propozycje pochodzą z `58d3e1d` i są odtwarzane przez bieżący DSP — nie deklarujemy uruchomienia starej binarki.

`validate_brain.py`: niezmodyfikowane nagranie, perkusja, własna synteza oraz dodatkowe rezonanse 2,2 / 4,8 / 6,5 kHz. To dodatkowe częstotliwości, ale nie niezależny korpus muzyczny. Potrzebne pozostają różne utwory, testy ślepe, pompowanie/transjenty, zgodność mono i długie sesje FL Studio. Limiter nadal nie zapewnia certyfikowanego True Peak.

Wyniki konkretnego uruchomienia znajdują się w `tests/data/brain-improved-demonstration-linux.json` oraz `tests/data/brain-validation-linux.json`. Pliki demonstracyjne mają odrębnie opisaną licencję CC BY-NC-SA 3.0; nie służą komercyjnej dystrybucji.

## Zmierzone wyniki Linux

Zmniejszenie odchylenia od tego samego wzoru w wybranej, niezależnej od poziomu mierze (dB; większa dodatnia wartość oznacza mniejsze odchylenie):

| Kontrolowana wada | Poprzednie ustawienia Brain | Nowy Brain |
|---|---:|---:|
| Nadmiar basu | 3,75 | 6,48 |
| Rezonans 3,5 kHz | −0,13 | 5,69 |
| Nierówny poziom powtórzeń perkusji | 0,82 | 13,23 |
| Przeciwfazowy bas / nadmierne Side | 7,44 | 19,85 |

Dodatkowe rezonanse: 2,2 kHz → 5,32 dB, 4,8 kHz → 4,35 dB, 6,5 kHz → 4,02 dB. Trzy wejścia bez dodanych wad nie otrzymały automatycznego EQ ani kompresji. Wszystkie sprawdzone rendery były deterministyczne, a wyniki dla buforów 256/1024 próbek identyczne. Nie jest to dowód poprawy każdego nagrania ani jakości odsłuchowej.
