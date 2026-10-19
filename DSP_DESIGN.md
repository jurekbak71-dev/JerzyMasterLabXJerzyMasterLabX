# Projekt DSP

EQ: biquad peaking według publicznych wzorów RBJ Audio EQ Cookbook, double stanu i współczynników. 16 niezależnych pasm bell. Współczynniki filtrów są wygładzane wykładniczo przez 20 ms. Brak dynamic EQ i linear phase.

Limiter: stereo-linked detektor sample peak, natychmiastowy atak obwiedni, release 10–1000 ms (domyślnie 80 ms), drive 0–24 dB przed ograniczaniem, opóźnienie ceil(fs*0.005), końcowe ograniczenie próbki. Maksimum okna przyszłych próbek względem opóźnionego wyjścia oblicza prealokowana kolejka monotoniczna. Ograniczenie zapobiega przekroczeniom sample ceiling po tym module, ale może zniekształcać transjenty. Wymagana kontrola interpolowanych szczytów, testy aliasingu i pomiary odsłuchowe przed zastosowaniem produkcyjnym.

Bypass: interpolacja dry/wet z czasem 20 ms. Współczynniki EQ mają interpolację; zmiana kolejności wykorzystuje 20 ms crossfade dwóch stale aktywnych, wyrównanych latencją torów. Bypass hosta ma oddzielny opóźniony tor dry i crossfade 20 ms. Ochrona NaN/Inf na wejściu i wyjściu, ScopedNoDenormals we wrapperze JUCE.

Mierniki: output sample peak, nieważony RMS i korelacja blokowa. FFT Hann 4096/8192/16384/32768, średnia energii L/R (zachowuje sygnały przeciwfazowe), normalizacja amplitudy sinusoidy zgodna z oknem. Rozmiar FFT i wygładzanie widma dotyczą tylko analizy GUI, nie DSP ani PDC. Brak kalibrowanej skali PSD, input overlay i peak hold. LUFS, True Peak i LRA nie zostały zaimplementowane; zgodność z BS.1770-5/EBU R128 nie jest deklarowana.

Krzywe edytora wykorzystują faktyczne współczynniki filtrów z audiothread przez prealokowane FIFO, a nie niezależny uproszczony model. Mierzona sinusoidą odpowiedź pojedynczego filtra zgadza się z obliczeniem zespolonym dla 20 kombinacji częstotliwości/sample rate. Krzywa sumaryczna uwzględnia zespolony blend bypassu EQ; nieliniowości limitera i zabezpieczeń nie są przez nią reprezentowane.


Drive i współczynnik release są wygładzane przez 20 ms. Detektor analizuje poziom po drive, natomiast opóźniony dry zachowuje poziom wejściowy; dzięki temu bypass modułu nie pozostawia wzmocnienia drive. Przy zmianach drive końcowy clamp pozostaje zabezpieczeniem sample ceiling. Lookahead i PDC są stałe.

`LimiterMeter` zbiera minimum efektywnej obwiedni gain i maksimum output peak w oknach 10 ms według zegara próbek (granice ceil(n*fs*0.01)). Bufor hosta nie wyznacza granic pomiaru. GR pochodzi z gain detektora po blendzie bypassu i kolejności, nie z ilorazu input/output ani z końcowego clampu. Drive sam w sobie nie jest GR. Podczas przejścia kolejności jest to interpolacja obwiedni dwóch aktywnych torów; nie reprezentuje całkowitej zmiany poziomu EQ/limiter. Wrapper uwzględnia blend bypassu hosta. Prealokowana SPSC kolejka 2048 ramek nie czeka na GUI; pełna kolejka pomija nowe ramki, zachowując ich zegar, więc edytor może narysować przerwę. Reset/prepare rozpoczyna nową generację historii. Edytor przechowuje do 1000 ramek i pokazuje ostatnie 10 sekund audio; brak callbacków zatrzymuje historię.
