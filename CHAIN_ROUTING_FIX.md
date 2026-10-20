# Naprawa obsługi łańcucha i routing równoległy

## Obsługa

`+ Modules → nazwa → Add to chain` dodaje i aktywuje procesor oraz otwiera jego panel. Ustawienia są zachowywane. Jeśli EQ albo Dynamic EQ miał wszystkie pasma wyłączone, przy dodawaniu włączane jest pierwsze pasmo. Przeciąganie kontrolki wyłączonego pasma EQ/Dynamic EQ uaktywnia edytowane pasmo. Pozostałe pasma zachowują swoje ustawienia. Neutralny EQ (gain 0), stereo width 1 lub kompresor poniżej progu mogą prawidłowo nie zmieniać brzmienia.

Kontrolka EQ w głównym panelu uwzględnia obecność w łańcuchu. Jej włączenie przywraca EQ do łańcucha. Samo oglądanie panelu nie dodaje procesora ani nie zmienia DSP. Mała lampka na karcie przełącza bypass; środek karty wybiera panel. Przeciągnij środek karty na inną kartę, aby zmienić kolejność. Prawy przycisk otwiera menu Move earlier / Move later / Remove. Przenoszenie i dodawanie mają Undo.

W nagłówku wybierz:

- **Serial** — każdy moduł otrzymuje wyjście poprzedniego, zgodnie z kolejnością kart.
- **Parallel** — każdy obecny moduł otrzymuje to samo wejście stereo. Wyjścia są wyrównane do stałego PDC 5 ms i uśredniane z równymi wagami 1/N. Nie ma dodatkowej ukrytej ścieżki dry. Obecna, ale bypassowana gałąź wnosi opóźniony sygnał dry. Usunięta gałąź nie uczestniczy w sumie. Kolejność niezależnych gałęzi nie zmienia ich sumy; zapisany układ kart nadal można zmieniać.

W Parallel limiter ogranicza własną gałąź, a nie całą sumę. Ceiling nie jest gwarancją końcowego poziomu. Nie dodano dowolnych zagnieżdżonych grup szeregowo-równoległych ani indywidualnych wag gałęzi. Nadal jedna instancja każdego z sześciu dostępnych procesorów.

## Przyczyny naprawionych zachowań

Dotychczas Add to chain zmieniało tylko `Present`. Zapisany bypass i wyłączone przez wcześniejszą propozycję Brain pasma mogły pozostać wyłączone. Panel EQ pokazywał flagę `eq`, nawet jeśli `eqPresent` było fałszywe. Pozornie aktywny panel nie musiał wtedy należeć do toru audio. Dodawanie i sygnalizację obecności skoordynowano; DSP nie zastąpiono atrapą.

Wewnętrzne przeciąganie nie używa już migawki całego edytora jako źródła JUCE DragAndDrop. Obsługuje bezpośrednio gest down/drag/up na kartach, podświetla cel, a menu kontekstowe udostępnia alternatywne przenoszenie. Test GUI wykonuje teraz cały taki gest, zamiast wywoływać wyłącznie `itemDropped`.

## Implementacja i stan

`SerialGraph` utrzymuje przygotowane poza callbackiem opóźnienia gałęzi, bufory 128 próbek i dwa banki do crossfade 20 ms przy zmianie trybu/kolejności/składu równoległego. W callbacku nie dodano mutexów, FFT ani alokacji. PDC pozostaje stałe również dla pustego łańcucha i bypassu. Automatyzowalny `routingMode` dopisano na końcu listy, zachowując dotychczasowe indeksy. Schema 6 ma 162 parametry; schema 1–5 są migrowane z domyślnym Serial. Nowy stan i presety zachowują tryb, kolejność i parametry.

## Sprawdzanie

`chain_functional_tests` przetwarza rzeczywisty sygnał przez Processor: wszystkie sześć modułów, oba tryby, 44,1/48/96/192 kHz, dodanie po bypassie i wyłączeniu wszystkich pasm, mierzalna różnica względem dry, przywrócenie dry po bypassie, jedna transakcja Undo, zapis routingu. Równoległy EQ + Dynamics porównywany jest z niezależnie obliczoną średnią dwóch osobnych procesorów.

`serial_graph_tests` obejmuje 720 permutacji i przełączanie Serial/Parallel podczas audio, audyt alokacji, skończone wyniki i stałą latencję. Test GUI sprawdza gest przenoszenia, Undo, wybór routingu, reakcje kontrolek, wszystkie panele i skalowanie. Testy stanu obejmują także migrację schema 5. Test FL Studio wymaga osobnego sprawdzenia w tym DAW; nie deklarujemy jego wykonania.

## Instalacja aktualizacji

Zamknij DAW i wymień cały bundle `JerzyMasterLabX.vst3` na folder z nowej paczki Windows x64. Domyślna lokalizacja: `C:\Program Files\Common Files\VST3`. Uruchom ponownie DAW i odśwież listę wtyczek. Starsze projekty zachowują swoje nastawy; dodaj ponownie moduł albo włącz EQ, jeśli poprzednia propozycja pozostawiła go poza łańcuchem.
