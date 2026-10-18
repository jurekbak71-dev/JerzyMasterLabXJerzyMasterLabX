# Projekt DSP

EQ: biquad peaking według publicznych wzorów RBJ Audio EQ Cookbook, double stanu i współczynników. 16 niezależnych pasm bell. Współczynniki filtrów są wygładzane wykładniczo przez 20 ms. Brak dynamic EQ i linear phase.

Limiter: stereo-linked detektor sample peak, natychmiastowy atak obwiedni, release 80 ms, opóźnienie ceil(fs*0.005), końcowe ograniczenie próbki. Maksimum okna przyszłych próbek względem opóźnionego wyjścia oblicza prealokowana kolejka monotoniczna. Ograniczenie zapobiega przekroczeniom sample ceiling po tym module, ale może zniekształcać transjenty. Wymagana kontrola interpolowanych szczytów, testy aliasingu i pomiary odsłuchowe przed zastosowaniem produkcyjnym.

Bypass: interpolacja dry/wet z czasem 20 ms. Współczynniki EQ mają interpolację; zmiana kolejności wykorzystuje 20 ms crossfade dwóch stale aktywnych, wyrównanych latencją torów. Bypass hosta ma oddzielny opóźniony tor dry i crossfade 20 ms. Ochrona NaN/Inf na wejściu i wyjściu, ScopedNoDenormals we wrapperze JUCE.

Mierniki: output sample peak, nieważony RMS i korelacja blokowa. FFT Hann 1024, mono suma kanałów. Brak kalibrowanej skali PSD, input overlay i peak hold. LUFS, True Peak i LRA nie zostały zaimplementowane; zgodność z BS.1770-5/EBU R128 nie jest deklarowana.
