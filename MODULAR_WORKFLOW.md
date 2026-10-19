# Łańcuch modułów i nowe procesory

## Dodawanie, usuwanie i bypass

Przycisk **+ Modules** otwiera listę pięciu procesorów. Każdy ma oddzielne **Add/Remove from chain** oraz **Enabled**. Usunięcie zachowuje ustawienia i usuwa kartę z widocznego łańcucha; bypass pozostawia kartę. Kliknięcie diody na karcie przełącza bypass, kliknięcie nazwy otwiera panel. Nowe panele mają także osobne przełączniki **In chain** i **Active**. Wszystkie te stany podlegają automatyce i zapisowi hosta/preseta.

Obecnie dostępna jest jedna instancja każdej z pięciu rodzin, nie dowolna liczba kopii. Kolejność nowych etapów jest ustalona:

**Dynamic EQ → Smart Dynamics → Stereo Architect → Spectral EQ → Limiter**.

Dotychczasowe przeciąganie zamienia dwa ostatnie procesory (EQ i limiter). Nie jest to jeszcze dowolny graf ani swobodne przestawianie nowych trzech modułów. Gdy EQ jest za limiterem, może podnieść finalny poziom ponad jego ceiling.

Obecność i bypass sterują przejściem 20 ms, bez tworzenia/usuwania obiektów DSP w callback. Przetwarzanie wygaszonego modułu zatrzymuje się po zejściu blendu do progu numerycznego. Cały plugin zachowuje PDC lookahead limitera 5 ms także przy pustym łańcuchu; nowe procesory nie dodają latencji. To pozwala przełączać je bez przestawiania PDC podczas odtwarzania.

## Stereo Architect

Cztery pasma: Low/Mid/High/Air. Trzy regulowane podziały, szerokość 0–2× dla każdego pasma, Balance, Mono Bass i Mono Preview. Width zmienia Side przy zachowaniu Mid, bez dodawania sztucznego sygnału stereo. Przy neutralnym balansie zmiana szerokości zachowuje sumę mono. Mono Preview centruje oba kanały również przy zmienionym balansie.

Podział korzysta z komplementarnych filtrów pierwszego rzędu: neutralne szerokości sumują się do wejścia. **Mono Bass ma łagodną charakterystykę**, nie jest ostrym odcięciem całego Side poniżej crossovera. To nie filtry 24 dB/oktawę. Poszerzanie może pogorszyć korelację, dlatego dostępne są pomiary per-band i odsłuch mono.

Wizualizacje: cztery korelacje i stosunki energii S/M z wygładzaniem 400 ms; przy większej szerokości panelu również vectorscope pełnego wyjścia łańcucha. Nie należy utożsamiać tego vectorscope z izolowanym wyjściem modułu. W kompaktowym widoku pozostają pomiary pasmowe.

## Smart Dynamics

Stereo-linked kompresor z threshold, ratio, attack, release, knee, makeup i dry/wet. **Upward maximum** ogranicza dodatkowe wzmocnienie poniżej **Upward threshold**; efekt wygasa poniżej −70 do −90 dBFS, aby nie podnosić ciszy. Używaj makeup i upward świadomie: podnoszą także szum materiału powyżej tego progu.

Digital stosuje regulację wzmocnienia. Analog wydłuża odpowiedź detektora, poszerza knee i dodaje łagodną, znormalizowaną charakterystykę arctangent. To własny charakter DSP, nie emulacja konkretnego urządzenia. **Nie ma oversamplingu** i nie deklarujemy kontroli aliasingu ani jakości komercyjnego modelowania analogowego.

Wykres przedstawia docelową charakterystykę statyczną (przed nieliniową kolorystyką), pozycję detektora i rzeczywistą redukcję/wzmocnienie upward. Historia jest serią snapshotów GUI, nie gwarantowaną historią próbek ani osią dokładnego czasu audio.

## Dynamic EQ

Trzy niezależne pasma Bell: frequency, static gain, Q, threshold, dynamic range, attack, release i band active. Własny detektor band-pass (wejście minus notch) mierzy każde pasmo. Powyżej threshold siła działania rośnie przez zakres 12 dB detektora, do zadanej wartości dynamic range. Wartość ujemna tłumi, dodatnia podbija. Nie ma automatycznego wykrywania rezonansów, trybu linear phase ani zewnętrznego sidechainu.

Krzywe powstają ze współczynników działających filtrów i blendów, a odczyty pokazują aktualną część dynamiczną w dB. To oddzielny moduł; statyczny Spectral EQ zachowuje 16 pasm, z których każde ma własny kolor krzywej, punktu i oznaczeń wybranego pasma. Biała linia oznacza sumaryczną charakterystykę samego Spectral EQ.

## Master Brain

W menu **processors** wybiera się dowolny zestaw z pięciu modułów (domyślnie wszystkie). Analiza wejścia nie zmienia DSP. Propozycja obejmuje ustawienia oraz dodanie/włączenie wybranych procesorów; wymaga **Apply changes** i jest jedną transakcją Undo. Niewybrane moduły zachowują ustawienia. Ręczna zmiana dowolnego parametru od rozpoczęcia analizy unieważnia możliwość zastosowania starej propozycji.

Nowe reguły: Stereo korzysta z korelacji i energii M/S czterech pasm z całego fragmentu (podziały analizy 140/2500/8000 Hz); zachowuje szerokość przy dobrej korelacji, zwęża problematyczne pasma przy małej/ujemnej; kompresor dobiera łagodne ratio z crest factor, bez makeup, a upward jest ograniczony do 2 dB w Aggressive. Styl Analog proponuje tryb Analog; pozostałe zachowują ręczny wybór trybu. Dynamic EQ proponuje do 2 dB tłumienia regionów o nadmiernej energii. Są to heurystyki, nie trenowana AI. Estymata loudness limitera **nie obejmuje interakcji nowych procesorów**; po Apply trzeba zmierzyć i odsłuchać pełne wyjście.

## Zgodność stanu i granice wydania

Schema 4: 138 parametrów. Pierwsze 86 ID/indeksów pozostają niezmienione. Schema 1/2/3 migrują z nowymi procesorami poza łańcuchem, zachowując dotychczasowy tor. Cały kandydat stanu jest walidowany przed zastosowaniem. Parametry wykraczające poza zakres i ułamkowe bool/choice są odrzucane.

Limiter pozostaje sample-peak, bez True Peak. Nie wykonano testu FL Studio ani porównań odsłuchowych z Ozone. Pełny zakres 20 procesorów pozostaje roadmapą; ten etap dodaje trzy żądane moduły i sterowanie pięcioma procesorami.
