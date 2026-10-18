# Architektura

`Source/Dsp.h` nie zależy od JUCE. `Engine::prepare` alokuje dwa bufory opóźnienia; `process` pracuje bez alokacji i mutexów. Opóźnienie pozostaje aktywne przy bypassie. `RoutedEngine` utrzymuje dwie stale przetwarzane kolejności (EQ/limiter oraz limiter/EQ). Oba tory mają identyczną latencję; parametr kolejności wybiera docelowy tor przez liniowy crossfade 20 ms. Stan żadnego toru nie jest kasowany przy przełączeniu. Bufory scratch mają stałe 128 próbek; większe bufory hosta są dzielone na fragmenty bez alokacji. Cena: oba tory zużywają CPU również po zakończeniu przejścia. Nie jest to jeszcze dynamiczny graf.

Wrapper utrzymuje osobny bufor dry o tej samej latencji i przetwarza oba tory także podczas bypassu hosta. `processBlockBypassed` nie omija PDC; dry/wet przechodzi liniowo przez 20 ms. Reset czyści historie, zachowując wybraną kolejność i ostatni cel bypassu hosta. Zmiany sample rate ponownie przygotowują bufory poza callbackiem.

Wrapper obsługuje tylko stereo. APVTS przechowuje parametry hosta i stan niezależnie od GUI, schemaVersion=1 i ID dwóch początkowych instancji (eq-0001, limiter-0001). Parametry pasm mają stabilne ID. FFT wykonuje timer edytora; SPSC FIFO przekazuje próbki z audio. Zamknięcie GUI może przepełnić FIFO (próbki analizy są pomijane), ale nie zatrzymuje DSP. Otwarta jest najwyżej jedna instancja edytora na procesor.

Dalszy rozwój: stabilne ID instancji modułów, stała pula parametrów automatyzacji, niezmienne snapshoty grafu przekazywane przez kolejkę z odzyskiwaniem pamięci poza audio, rozszerzenie crossfade na dynamiczne grafy, migracja serializacji dla przyszłych grafów i spójne snapshoty parametrów. MASTER BRAIN pozostanie poza grafem audio.

## Odczyt stanu

`Source/StateValidation.h` sprawdza cały snapshot przed `replaceState`: schemaVersion=1, ID istniejących dwóch modułów, komplet 52 unikalnych znanych parametrów, brak zagnieżdżonych węzłów parametrów, skończone liczby w zadeklarowanych zakresach i binarne wartości bool. Dane binarne większe niż 1 MiB, puste, ucięte i niepoprawny XML są odrzucane. Nieprawidłowy snapshot nie zmienia żadnego parametru. Poprawny snapshot jest kopiowany do kanonicznego ValueTree. Nieznane wersje nie są interpretowane jako v1; migracja przyszłego grafu wymaga jawnego kodu migracji. Walidacja i XML należą do operacji stanu hosta, nie do `processBlock`. Nie deklarujemy atomowego przestawienia wszystkich poprawnych parametrów między wątkami; obecny APVTS przekazuje je indywidualnie.
