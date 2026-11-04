# Łańcuch modułów i sposób pracy

## Dostępne procesory

Działa sześć rodzin: **Stabilizer, Dynamic EQ, Smart Dynamics, Stereo Architect, Spectral EQ i Limiter**. Przycisk **+ Modules** dodaje lub usuwa procesor; osobny przełącznik Enabled/Active steruje bypassem. Kliknięcie diody karty przełącza bypass, nazwy — otwiera panel. Usunięcie zachowuje ustawienia.

**Przeciąganie dowolnej karty zmienia rzeczywistą kolejność DSP.** Dostępne są wszystkie 720 kolejności sześciu modułów. Kolejność zapisuje się w FLP przez stan wtyczki i w presetach `.jmlx`; ma automatykę i Undo/Redo. Sam test zachowania FL Studio wymaga jeszcze wykonania w tym DAW.

W trybie Parallel każda obecna rodzina jest osobną, wyrównaną czasowo gałęzią. Panel Parallel Branch Mixer udostępnia trim -24…+12 dB oraz wet 0…100% dla każdej gałęzi. Domyślny tryb Legacy 1/N zachowuje wcześniejsze brzmienie i dzieli sumę przez liczbę obecnych gałęzi; opcjonalne Normalize branch weights dzieli przez sumę ich liniowych gainów. Wyłączony, lecz obecny moduł wnosi wyrównany sygnał dry, a usunięty moduł nie tworzy gałęzi. Limiter w Parallel ogranicza tylko własną gałąź, o czym informuje panel.

Smart Dynamics ma jawne profile Legacy/Clean/Strong/Extreme, Punch oraz Delta. Legacy z Punch 0 i Delta off zachowuje wcześniejszy DSP. Profile zmieniają skalę zmierzonej redukcji i upward compression bez ukrytego makeup; Dry/Wet pozostaje rzeczywistą kompresją równoległą. Delta odtwarza wyłącznie wkład `processed - input` po Dry/Wet.

Obecnie istnieje **jedna instancja każdej rodziny**. Drugi EQ i dowolna liczba instancji nadal wymagają implementacji. Domyślny porządek to Stabilizer → Dynamic EQ → Dynamics → Stereo → EQ → Limiter, z pominięciem nieobecnych procesorów. Procesory za limiterem mogą przekroczyć jego ceiling.

Przy zmianie kolejności dwa przygotowane banki DSP wykonują 20 ms crossfade. Stan filtrów i detektorów jest kopiowany do przygotowanej pamięci. Szybkie kolejne żądania czekają na koniec aktualnego crossfade; ostatnia kolejność wygrywa. Nie ma mutexów, tworzenia obiektów ani alokacji w callback, co sprawdza test. PDC pozostaje równy lookahead 5 ms także przy pustym łańcuchu. Crossfade ogranicza skoki; nie gwarantuje niesłyszalności zmiany topologii dla każdego materiału.

## Stereo Architect

Cztery kolorowe pasma Low/Mid/High/Air, trzy przeciągane podziały na logarytmicznym widmie, cztery pionowe suwaki Width 0–2×, osobne **On** i **S** każdego pasma, **Amount**, **Delta**, Balance, Mono Bass i Mono Preview. Kilka solo można łączyć. On wyłącza przetwarzanie szerokości danego pasma, przywracając unity; nie wycisza audio.

Width zmienia Side przy zachowaniu Mid. Amount=0 i wszystkie pasma z wyłączonym przetwarzaniem są neutralne, bez aktywnych solo/mono/balance/Delta. Delta to wejście minus przetworzone wyjście modułu. Przełączenia są wygładzane.

Podział jest komplementarny, pierwszego rzędu: neutralne pasma sumują się do wejścia. Mono Bass ma łagodny spadek, a nie odcięcie Side 24 dB/okt. Poszerzanie może pogorszyć zgodność mono. Korelacja każdego pasma pochodzi z pomiaru po Width, przed odsłuchem solo. **Widmo, półkolisty vectorscope i dolna korelacja pokazują wyjście całej wtyczki**, zgodnie z opisami panelu. Vectorscope składa dodatnie i ujemne Mid w górną półpłaszczyznę. Opcjonalny **Stereoize 0–1** tworzy Side z Mid przez filtrację basu i rotację fazy; domyślnie 0 zachowuje stare brzmienie. Nie tworzy Side w najniższym paśmie. Działa również dla dual mono, gdzie samo Width niczego nie poszerzy. Amount i On pasma obejmują także wygenerowany Side. Przy centralnym Balance, bez solo/Delta, suma mono pozostaje zachowana; poziom i barwa stereo mogą się zmienić. Użyj Mono Preview i porównaj poziomy. W trybie Parallel efekt pojedynczej gałęzi jest dzielony przez liczbę obecnych gałęzi. **W** oznacza zmierzone Side/Mid pasma, **C** — korelację. Szczegóły i testy: [pierwszy etap](docs/stereo-architect/PHASE1.md). Recover Sides i Learn pozostają niezaimplementowane.

## Stabilizer

Własny adaptacyjny korektor z 12 detektorami pasmowymi i 12 filtrami Bell. Co 64 próbki mierzone energie pasm są porównywane ze średnią, z uwzględnieniem jawnego profilu Neutral/Electronic/Acoustic. To nie jest model wytrenowany na nagraniach ani algorytm konkurencji.

**Shape** dopuszcza ograniczone podbicia i tłumienia; **Cut** tylko tłumienia. Limit jednego filtra to ±6 dB przed Amount i regulacją regionów; filtry nakładają się, więc sumaryczna krzywa może mieć większą zmianę. **Amount** skaluje korekcję, **Speed** ustala czas detektora i adaptacji (większa wartość = wolniej), **Smoothing** uśrednia sąsiednie pasma, **Low/Mid/High** sterują trzema regionami. **Tame Transients** osłabia docelową adaptację przy dużym crest chwilowym; nie rekonstruuje transjentów i nie jest osobnym transient shaperem. **Delta** odtwarza prawdziwą różnicę wejście–wyjście.

GUI rysuje charakterystykę rzeczywistych współczynników filtrów i bypassu. Przy przełączaniu kolejności pokazuje aktywny bank; nie jest analizą sumy dwóch banków podczas crossfade. Adaptacja jest taktowana próbkami i daje identyczny wynik przy różnych podziałach bufora. Brak dodatkowej latencji, brak alokacji w process.

## Dynamics i Dynamic EQ

Dynamics: stereo-linked threshold/ratio/attack/release/knee/makeup/mix, ograniczona upward compression z ochroną ciszy, Digital lub Analog. Analog wydłuża detektor, poszerza knee i dodaje łagodną kolorystykę arctangent. Nie ma oversamplingu ani deklaracji emulacji sprzętu. Historia jest serią snapshotów GUI.

Dynamic EQ: trzy niezależne Bell z detekcją input-minus-notch, dodatnim lub ujemnym dynamic range, własnym progiem, attack i release. Nie ma linear phase, automatycznej identyfikacji rezonansów ani zewnętrznego sidechainu. Spectral EQ ma 16 pasm z różnymi kolorami rzeczywistych krzywych i punktów.

## Brain — dobór modułów

Domyślnie **Automatically select needed modules** wybiera spośród sześciu dozwolonych rodzin:

- EQ: przynajmniej jedna sugerowana korekcja ponad 0,25 dB.
- Dynamic EQ: energia problematycznego regionu ponad 1,4× celu.
- Imager: korelacja przynajmniej jednego pasma poniżej 0,2.
- Dynamics: crest ponad 14 dB, albo ponad 10 dB w Aggressive.
- Limiter: potrzebny Drive ponad 0,25 dB lub wejście ponad ceiling.
- Stabilizer: przynajmniej trzy sugerowane szerokie korekcje przekraczają 1,5 dB.

Brain może zatem zaproponować różną liczbę modułów, a nie zawsze sześć. To jawne heurystyki z ograniczonego fragmentu, nie certyfikat jakości. Gdy wyłączysz automatyczny wybór, dodaje wszystkie ręcznie dozwolone rodziny. Wykluczone rodziny zachowują swoje ustawienia. Gdy kontroluje cały łańcuch, proponuje domyślną kolejność z limiterem na końcu; nie optymalizuje jeszcze dowolnych topologii i nie tworzy wielu instancji.

Analiza nie zmienia audio. **Apply changes** stosuje propozycję po akceptacji, jako jedną transakcję Undo. Zmiana parametrów od początku analizy unieważnia starą propozycję. Estymata głośności nie obejmuje interakcji Dynamics, Dynamic EQ, Imagera i Stabilizera — konieczny jest pomiar i odsłuch pełnego wyniku.

## Stan i granice wydania

**Schema 5, 161 parametrów**; pierwsze 138 ID i indeksów zachowane. Schema 1/2/3/4 migrują z nowymi opcjami Imagera w ustawieniu neutralnym, domyślną kolejnością i Stabilizerem poza łańcuchem. Cały kandydat jest walidowany przed zastosowaniem; niepełne stany, złe identyfikatory, NaN/Inf, wartości poza zakresem oraz ułamkowe bool/choice/int są odrzucane.

Limiter nadal jest sample-peak, bez True Peak. Pełny Maximizer ze zrzutu, druga instancja EQ, Exciter, Master Rebalance, Match EQ i cztery moduły Vintage pozostają do wykonania. Nie przeprowadzono testu FL Studio ani odsłuchowego porównania z Ozone.
