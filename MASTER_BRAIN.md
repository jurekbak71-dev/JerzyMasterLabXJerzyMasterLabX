# Master Brain — asystent regułowy

> Aktualizacja bieżącej implementacji: [MODULAR_WORKFLOW.md](MODULAR_WORKFLOW.md). Dodano Stabilizer, dowolną kolejność sześciu rodzin, nowe sterowanie Imagera i automatyczny wybór modułów przez Brain. Poniższe starsze opisy ustalonej kolejności, pięciu rodzin i schema 4 opisują poprzedni etap. Nadal jedna instancja na rodzinę i brak True Peak.


Otwórz **MASTER BRAIN**, wybierz gatunek, styl, długość analizy (8/16/30/60 sekund), docelowy LUFS i ceiling próbek. Zaznacz EQ i/lub limiter. Naciśnij **ANALYSE AUDIO** i odtwórz reprezentatywny fragment w czasie rzeczywistym. Zatrzymanie transportu zatrzymuje zbieranie próbek. Anulowanie nie zmienia DSP.

Asystent mierzy wejście przed procesorami: LUFS Integrated, peak próbek, RMS, korelację, przekroczenia i energię pięciu regionów widma. FFT i obliczanie propozycji działają na osobnym wątku; audio callback wyłącznie kopiuje próbki do przygotowanej kolejki. Przepełnienie unieważnia analizę zamiast akceptować niepełne dane. Analiza nie zależy od otwartego edytora.

Wynik zawiera proponowane korekty EQ, Drive limitera, uzasadnienie i ostrzeżenia. Można zmienić profil i przeliczyć propozycję z zebranych danych. **APPLY PROPOSAL** wymaga świadomego kliknięcia. Całość można cofnąć jednym Undo. Ręczna zmiana parametrów DSP od rozpoczęcia analizy blokuje zastosowanie starej propozycji; wykonaj nową analizę. Podczas zatwierdzania wstrzymaj automatykę hosta.

## Reguły i ograniczenia

To heurystyki DSP, nie wytrenowana sieć ani algorytm iZotope. Dziewięć profili gatunkowych to ręcznie ustalone proporcje energii. Pięć szerokich korekt jest ograniczonych do ±3 dB; Intensity zmienia korekcję tonalną. Wybór EQ zastępuje ustawienia wszystkich 16 pasm, pozostałe wyłącza. Wyłączenie wyboru EQ zachowuje istniejący korektor. Styl Analog oznacza łagodniejsze ustawienia, nie dodaje saturacji.

Drive wynika z różnicy LUFS i przybliżonego wpływu EQ, z konserwatywnym ograniczeniem przewidywanej redukcji. Nie gwarantuje osiągnięcia celu. Za głośne wejście wymaga ściszenia w DAW. Limiter jest sample-peak: brak True Peak, oversamplingu i odpowiednika IRC. Nie ma separacji stemów, uczenia profilu z referencji ani automatycznej klasyfikacji gatunku. Analiza fragmentu nie zastępuje pomiaru całego utworu.

Ustawienia i raport analizy są tymczasowe. Zatwierdzone parametry DSP zapisują się standardowo w projekcie hosta i presetach `.jmlx`; sam raport nie jest serializowany. Szybki eksport offline może przepełnić kolejkę analizy — analizuj podczas zwykłego odtwarzania.

## Walidacja

Testy obejmują kombinacje gatunków/stylów/intensywności, granice korekcji, wykluczenie modułów, ciszę i błędne dane. Test procesora sprawdza rzeczywiste osiem sekund analizy, brak automatycznych zmian, brak alokacji C++ w callback podczas zbierania, odrzucenie nieaktualnej propozycji, Apply/Undo, anulowanie i przepełnienie. Test GUI otwiera panel w trzech rozmiarach, zatwierdza i cofa propozycję. Nie wykonano testu FL Studio ani porównania brzmieniowego z Ozone.

## Graficzny balans tonalny

Panel wyników pokazuje pięć zmierzonych udziałów energii względem celu gatunkowego (0 dB). Biała linia to wejście; turkusowa to **EQ ESTIMATE**, czyli przybliżenie po EQ obliczone z rzeczywistych charakterystyk filtrów w pięciu częstotliwościach reprezentatywnych. To nie pomiar przetworzonego wyjścia i nie uwzględnia limitera. Udziały są ponownie normalizowane do całkowitej energii analizowanych regionów. Szczegółowy raport zawiera procenty wejścia/celu/estymaty oraz korekty EQ. Przy EQ wykluczonym estymata uwzględnia zachowany korektor.

Oś ma zakres ±18 dB i przycina odchylenia poza skalą; liczby w raporcie pozostają dostępne. Pięć punktów oznacza szerokie regiony (20–80, 80–250, 250–2000, 2000–6000, 6000–20000 Hz), nie pełną krzywą FFT. Profil docelowy jest heurystyką estetyczną, nie wzorcem jakości. Po Apply trzeba zmierzyć i odsłuchać pełne wyjście. Testy porównują estymowane udziały z energią sinusów faktycznie przepuszczonych przez proponowane filtry przy 44,1/48/96/192 kHz.

Aktualizacja modułowa: Brain obsługuje teraz wybór pięciu procesorów i ich obecność/bypass. Zasady nowych propozycji i ograniczenia estymaty opisuje [MODULAR_WORKFLOW.md](MODULAR_WORKFLOW.md). Informacje powyżej o wyborze wyłącznie EQ/limitera dotyczą wcześniejszej rewizji.
