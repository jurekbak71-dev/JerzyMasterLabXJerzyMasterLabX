# Projekt DSP

EQ: biquad peaking według publicznych wzorów RBJ Audio EQ Cookbook, double stanu i współczynników. Jedno aktywne pasmo; tablica 16 filtrów nie oznacza 16 zaimplementowanych pasm. Brak dynamic EQ i linear phase.

Limiter: stereo-linked detektor sample peak, natychmiastowy atak obwiedni, release 80 ms, opóźnienie ceil(fs*0.005), końcowe ograniczenie próbki. To nie pełny algorytm predykcyjnego lookahead: obwiednia nie oblicza maksimum przyszłego okna względem każdej opóźnionej próbki. Ograniczenie zapobiega przekroczeniom sample ceiling po tym module, ale może zniekształcać transjenty. Wymagane monotoniczne maksimum w oknie, kontrola interpolowanych szczytów, testy aliasingu i pomiary odsłuchowe przed zastosowaniem produkcyjnym.

Bypass: interpolacja dry/wet z czasem 20 ms. Nie wygładza częstotliwości/Q/wzmocnienia EQ ani zmiany kolejności. Ochrona NaN/Inf na wejściu i wyjściu, ScopedNoDenormals we wrapperze JUCE.

Mierniki: output sample peak, nieważony RMS i korelacja blokowa. FFT Hann 1024, mono suma kanałów. Brak kalibrowanej skali PSD, input overlay i peak hold. LUFS, True Peak i LRA nie zostały zaimplementowane; zgodność z BS.1770-5/EBU R128 nie jest deklarowana.
