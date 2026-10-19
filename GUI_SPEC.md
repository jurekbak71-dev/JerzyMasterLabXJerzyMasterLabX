# GUI

Dedykowany Spectral Workspace zastąpił GenericAudioProcessorEditor. Zawiera dwie karty obecnego toru z przeciąganiem kolejności, duży wykres, oddzielny Stereo Inspector i panel wybranego pasma zamiast przewijanej listy 52 parametrów. Układ jest własny; analiza Ozone 12 i granice źródeł są w OZONE12_REVIEW.md.

## Zaimplementowane zachowanie

- Wybór 16 pasm bell. Kontrolki częstotliwości, gain, Q i ceiling oraz bypass obu modułów.
- Przeciąganie punktu zmienia częstotliwość i gain; kółko nad wybranym punktem reguluje Q. Gesty begin/end trafiają do hosta. Zmiana wyboru pasma nie zmienia pozostałych pasm.
- Złota krzywa: odpowiedź EQ z faktycznych współczynników DSP i blendu bypassu, przekazana SPSC FIFO. Cień: odpowiedź wybranego filtra. To nie charakterystyka całego toru z limiterem.
- Turkusowe widmo: energia L/R, FFT Hann 4096–32768, domyślnie 8192, interpolacja binów i wygładzanie czasowe opadania. Skala widma -100–0 dBFS jest niezależna od osi EQ ±18 dB.
- FFT i grafika pracują poza callbackiem. Zmiana rozmiaru FFT nie zmienia latencji audio. Nie ma atrap przebiegów.
- Vectorscope oraz blokowe output peak/RMS/correlation. Ostrzeżenie, gdy EQ znajduje się za limiterem, ponieważ może podnieść poziom ponad ceiling modułu.
- Tooltips kontrolek. Resize 750×550–2000×1400. Testy używają rzeczywistej pętli komunikatów JUCE, a nie tylko jednorazowego paint.

## Walidacja

Testy GUI otwierają edytor trzykrotnie, zmieniają rozmiar, wybierają pasmo 16, zmieniają kontrolki, przeciągają węzeł, regulują Q kółkiem, sprawdzają gesty hosta i obecność krzywej wynikającej z audio. Przełączają wszystkie cztery rozmiary FFT bez zmiany PDC. Zrzut pochodzi z syntetycznego sygnału testowego, nie z makiety.

## Otwarte wymagania

Zoom 75–200% i HiDPI, osiem pełnych widoków, dodawanie/usuwanie/powielanie modułów, Undo/Redo i A/B, input/output overlay, freeze/peak hold, historia loudness, edycja pozostałych typów filtrów i trybów kanałowych. Zmiana wymiarów nie jest równoznaczna z ukończonym zoomem.


## Limiter Workspace

Oddzielny przycisk widoku, trzy kontrolki Drive/Release/Sample Ceiling, bypass modułu, historia GR i output peak, maksimum widocznego zakresu oraz odczyt PDC. Dane 10 ms są generowane w DSP; repaint nie tworzy wartości pomiarów. Okno pokazuje do 10 s audio, obwiednia jest ograniczana wizualnie do 24 dB (wartość maksimum nadal pokazuje głębszą redukcję). Wskaźnik braku nowego audio jest statusem dostarczania danych, nie rozpoznaniem transportu hosta. Linie nie łączą luk utraconych ramek. Generacja i zegar wykrywają reset historii. Po ponownym otwarciu edytor odbiera pozostałe ramki kolejki; pełna historia sesji nie jest gwarantowana.

Test GUI przełącza widoki, sprawdza widoczność i powiązanie nowych kontrolek z parametrami, obecność niezerowej redukcji na wykresie i trzy rozmiary okna. Zapisuje również `build-plugin/limiter-smoke.png`; workflow Windows archiwizuje oba zrzuty.
