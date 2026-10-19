# Projekt DSP

EQ: biquad Bell, Low Shelf, High Shelf, High Pass, Low Pass i Notch według publicznych wzorów RBJ Audio EQ Cookbook; double stanu i współczynników. 16 niezależnych pasm. HP/LP mają stałe 12 dB/oktawę, Q oznacza rezonans (nie nachylenie S). Współczynniki filtrów są wygładzane wykładniczo przez 20 ms. Brak dynamic EQ i linear phase.

Limiter: stereo-linked detektor sample peak, natychmiastowy atak obwiedni, release 10–1000 ms (domyślnie 80 ms), drive 0–24 dB przed ograniczaniem, opóźnienie ceil(fs*0.005), końcowe ograniczenie próbki. Maksimum okna przyszłych próbek względem opóźnionego wyjścia oblicza prealokowana kolejka monotoniczna. Ograniczenie zapobiega przekroczeniom sample ceiling po tym module, ale może zniekształcać transjenty. Wymagana kontrola interpolowanych szczytów, testy aliasingu i pomiary odsłuchowe przed zastosowaniem produkcyjnym.

Bypass: interpolacja dry/wet z czasem 20 ms. Współczynniki EQ mają interpolację; zmiana kolejności wykorzystuje 20 ms crossfade dwóch stale aktywnych, wyrównanych latencją torów. Bypass hosta ma oddzielny opóźniony tor dry i crossfade 20 ms. Ochrona NaN/Inf na wejściu i wyjściu, ScopedNoDenormals we wrapperze JUCE.

Mierniki: input/output sample peak, nieważony RMS, korelacja, balans energii i RMS Mid/Side w dokładnym przesuwanym oknie ceil(fs*0.4) próbek. FFT Hann 4096/8192/16384/32768, średnia energii L/R (zachowuje sygnały przeciwfazowe), normalizacja amplitudy sinusoidy zgodna z oknem. Rozmiar FFT i wygładzanie widma dotyczą tylko analizy GUI, nie DSP ani PDC. Brak kalibrowanej skali PSD, input overlay i peak hold. LUFS M/S/I opisano poniżej. True Peak i LRA nie zostały zaimplementowane; pełna zgodność z BS.1770-5/EBU R128 nie jest deklarowana.

Krzywe edytora wykorzystują faktyczne współczynniki filtrów z audiothread przez prealokowane FIFO, a nie niezależny uproszczony model. Mierzona sinusoidą odpowiedź pojedynczego filtra zgadza się z obliczeniem zespolonym dla 20 kombinacji częstotliwości/sample rate. Krzywa sumaryczna uwzględnia zespolony blend bypassu EQ; nieliniowości limitera i zabezpieczeń nie są przez nią reprezentowane.


Drive i współczynnik release są wygładzane przez 20 ms. Detektor analizuje poziom po drive, natomiast opóźniony dry zachowuje poziom wejściowy; dzięki temu bypass modułu nie pozostawia wzmocnienia drive. Przy zmianach drive końcowy clamp pozostaje zabezpieczeniem sample ceiling. Lookahead i PDC są stałe.

`LimiterMeter` zbiera minimum efektywnej obwiedni gain i maksimum output peak w oknach 10 ms według zegara próbek (granice ceil(n*fs*0.01)). Bufor hosta nie wyznacza granic pomiaru. GR pochodzi z gain detektora po blendzie bypassu i kolejności, nie z ilorazu input/output ani z końcowego clampu. Drive sam w sobie nie jest GR. Podczas przejścia kolejności jest to interpolacja obwiedni dwóch aktywnych torów; nie reprezentuje całkowitej zmiany poziomu EQ/limiter. Wrapper uwzględnia blend bypassu hosta. Prealokowana SPSC kolejka 2048 ramek nie czeka na GUI; pełna kolejka pomija nowe ramki, zachowując ich zegar, więc edytor może narysować przerwę. Reset/prepare rozpoczyna nową generację historii. Edytor przechowuje do 1000 ramek i pokazuje ostatnie 10 sekund audio; brak callbacków zatrzymuje historię.

Każde pasmo ma blend bypassu 20 ms i stale przetwarzany filtr. Krzywa pojedynczego pasma to (1-mix)+mix*H(z), a sumaryczna jest iloczynem tych odpowiedzi przed blendem globalnego EQ. Wybór typu wygładza współczynniki; nie deklarujemy idealnie bezartefaktowej modulacji wszystkich ekstremalnych ustawień. Testy obejmują 96 pomiarów sinusoidalnych sześciu typów przy czterech sample rates oraz przełączanie skrajnych ustawień Q/gain.


## Centralne pomiary

`WindowMeter` utrzymuje sumy L²/R²/LR i monotoniczną kolejkę peak w przesuwanym oknie 400 ms. Nie skanuje całego bufora. Sygnał stereo jest mierzony jako sqrt((sum L²+sum R²)/(2*N)); M=(L+R)/2, S=(L-R)/2. Balans to (energia R-energia L)/(suma energii). Przed zapełnieniem okna mianownik odpowiada liczbie dostępnych próbek. Licznik przekroczeń zlicza pozycje stereo z co najmniej jednym kanałem >1, nie wykrywa wcześniejszego clippingu w źródle. Reset jest O(1): stara zawartość jest ignorowana do ponownego zapełnienia.

`LoudnessMeter` implementuje stereofoniczne K-weighting (shelf i RLB high-pass z przeliczeniem częstotliwości próbkowania), 400 ms M i 3 s S. Bloki pomiarowe są wyznaczone zegarem audio co 100 ms, z ważeniem ich dokładnej liczby próbek. Integrated wykorzystuje bloki 400 ms z krokiem 100 ms: bramka absolutna -70 LUFS i względna -10 LU. Współczynniki obejmują dwie niezależne pary filtrów L/R; energie kanałów są sumowane, nie uśredniane jak RMS.

Integrated przechowuje sumy energii i liczbę obserwacji w histogramie 0,01 LU z drzewem Fenwicka. Odczyt bramki i aktualizacja są ograniczone O(log 20000), bez sortowania ani skanowania historii w callbacku. Kwalifikacja na granicy jednego koszyka jest przybliżona; nie deklarujemy idealnie dokładnej bramki dla każdego możliwego rozkładu na granicy. Nie ma limitu liczby bloków wynikającego z rozmiaru historii. Lazy generation reset zeruje logiczny histogram bez alokacji i czyszczenia całej tablicy. Zakres histogramu -70..+130 LUFS pokrywa ograniczone wejście `clean`.

Nie wykonujemy LRA, true peak, kanałów surround ani certyfikacji całego standardu. Referencje to oryginalne syntetyczne sygnały z `LoudnessFixtures.h`, porównane z FFmpeg 7.1.5 ebur128: 16 przypadków, 48 odczytów I/M/S, tolerancja 0,11 LU. CSV zawiera wynik FFmpeg zaokrąglony do 0,1 LU. Testy obejmują ciszę, relatywne/absolutne bramkowanie, reset i zmienny host buffer. FFmpeg nie jest zależnością runtime ani częścią pluginu.


Master Brain: zaimplementowano analizę wejścia poza callback, regułowe propozycje EQ/limitera, panel konfiguracji i raportu, zatwierdzanie oraz jedną transakcję Undo. Zakres, algorytm i ograniczenia opisano w [MASTER_BRAIN.md](MASTER_BRAIN.md). Nie jest to kompletna docelowa implementacja v0.4.0.

Nowe DSP (`ModularDsp.h`): cztery komplementarne pasma M/S z płynnymi szerokościami, stereofoniczny detektor peak kompresora z miękkim knee i bounded upward (+ fade -90/-70 dBFS), Digital lub łagodna kolorystyka Analog, trzy niezależne detektory Dynamic EQ (input minus notch) sterujące filtrami Bell. Soft knee ma testy ciągłości na obu granicach. Bypass 20 ms; dry/wet kompresora wygładzany. Dodatnia range Dynamic EQ podbija, ujemna tłumi. Szczegółowe ograniczenia (1st-order splits, brak oversamplingu) i kolejność: MODULAR_WORKFLOW.md.
