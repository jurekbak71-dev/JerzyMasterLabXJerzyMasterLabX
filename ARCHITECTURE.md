# Architektura

`Source/Dsp.h` nie zależy od JUCE. `Engine::prepare` alokuje dwa bufory opóźnienia; `process` pracuje bez alokacji i mutexów. Opóźnienie pozostaje aktywne przy bypassie. `RoutedEngine` utrzymuje dwie stale przetwarzane kolejności (EQ/limiter oraz limiter/EQ). Oba tory mają identyczną latencję; parametr kolejności wybiera docelowy tor przez liniowy crossfade 20 ms. Stan żadnego toru nie jest kasowany przy przełączeniu. Bufory scratch mają stałe 128 próbek; większe bufory hosta są dzielone na fragmenty bez alokacji. Cena: oba tory zużywają CPU również po zakończeniu przejścia. Nie jest to jeszcze dynamiczny graf.

Wrapper utrzymuje osobny bufor dry o tej samej latencji i przetwarza oba tory także podczas bypassu hosta. `processBlockBypassed` nie omija PDC; dry/wet przechodzi liniowo przez 20 ms. Reset czyści historie, zachowując wybraną kolejność i ostatni cel bypassu hosta. Zmiany sample rate ponownie przygotowują bufory poza callbackiem.

Wrapper obsługuje tylko stereo. APVTS przechowuje parametry hosta i stan niezależnie od GUI, schemaVersion=3 i ID dwóch początkowych instancji (eq-0001, limiter-0001). Parametry pasm mają stabilne ID. FFT wykonuje timer edytora; SPSC FIFO przekazuje próbki z audio. Zamknięcie GUI może przepełnić FIFO (próbki analizy są pomijane), ale nie zatrzymuje DSP. Otwarta jest najwyżej jedna instancja edytora na procesor.

Dalszy rozwój: stabilne ID instancji modułów, stała pula parametrów automatyzacji, niezmienne snapshoty grafu przekazywane przez kolejkę z odzyskiwaniem pamięci poza audio, rozszerzenie crossfade na dynamiczne grafy, migracja serializacji dla przyszłych grafów i spójne snapshoty parametrów. MASTER BRAIN pozostanie poza grafem audio.

## Odczyt stanu

`Source/StateValidation.h` sprawdza cały snapshot przed `replaceState`: schemaVersion=3, ID istniejących dwóch modułów, komplet 86 unikalnych znanych parametrów, brak zagnieżdżonych węzłów parametrów, skończone liczby w zadeklarowanych zakresach i binarne wartości bool oraz całkowite indeksy wyborów. Dane binarne większe niż 1 MiB, puste, ucięte i niepoprawny XML są odrzucane. Nieprawidłowy snapshot nie zmienia żadnego parametru. Poprawny snapshot jest kopiowany do kanonicznego ValueTree. Schema 1 z kompletem wcześniejszych 52 parametrów jest jawnie migrowany: limiterDrive=0 dB i limiterRelease=80 ms. Dwa nowe parametry są dopisane na końcu listy hosta. Nieznane wersje są odrzucane; migracja przyszłego grafu wymaga jawnego kodu migracji. Walidacja i XML należą do operacji stanu hosta, nie do `processBlock`. Nie deklarujemy atomowego przestawienia wszystkich poprawnych parametrów między wątkami; obecny APVTS przekazuje je indywidualnie.

Telemetry limitera korzysta z `LimiterMeter.h`: 100 obserwacji na sekundę audio, FIFO SPSC, brak pracy graficznej i alokacji w callbacku. Przełączanie panelu EQ/limiter nie zmienia DSP.

Schema 3 dopisuje 32 parametry (bandType1..16, bandEnabled1..16) po poprzednich 54. Migracja akceptuje wyłącznie właściwy zestaw pól danej wersji, uzupełnia brakujące wartości fabryczne i zapisuje kanoniczny schema 3. Krzywa DSP otrzymuje również 16 blendów bypassu przez istniejącą kolejkę odpowiedzi.

`Processor.h` oddziela wrapper i stan od definicji edytora w `Plugin.cpp`. UndoManager jest własnością procesora i jest podłączony do APVTS. Operacje snapshot/Undo/preset są operacjami sterującymi poza callbackiem. Host restore używa replaceState i czyści historię, preset użytkownika stosuje poprawnie zwalidowane parametry jako jedną transakcję z powiadomieniami hosta.


Każdy procesor ma niezależne `WindowMeter` i `LoudnessMeter` wejścia/wyjścia. Wszystkie bufory są alokowane w prepareToPlay; SPSC ramka odpowiedzi przenosi spójny snapshot danych do UI. Peak/RMS/correlation używane w innych panelach pochodzą teraz z okna 400 ms, a nie z bloku hosta. Reset mierników z GUI jest żądaniem atomic<bool>, wykonanym na początku callbacku bez resetowania silnika DSP. Stanu pomiarów nie zapisujemy w presetach ani projektach. Mierniki nie zależą od obecności edytora.


Master Brain: zaimplementowano analizę wejścia poza callback, regułowe propozycje EQ/limitera, panel konfiguracji i raportu, zatwierdzanie oraz jedną transakcję Undo. Zakres, algorytm i ograniczenia opisano w [MASTER_BRAIN.md](MASTER_BRAIN.md). Nie jest to kompletna docelowa implementacja v0.4.0.
