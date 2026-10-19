# GUI

Dedykowany Spectral Workspace zastąpił GenericAudioProcessorEditor. Zawiera dwie karty obecnego toru z przeciąganiem kolejności, duży wykres, oddzielny Stereo Inspector i panel wybranego pasma zamiast przewijanej listy 52 parametrów. Układ jest własny; analiza Ozone 12 i granice źródeł są w OZONE12_REVIEW.md.

## Zaimplementowane zachowanie

- Wybór 16 pasm, sześć rzeczywistych typów filtrów i niezależny bypass pasma. Kontrolki częstotliwości, gain, Q i ceiling oraz bypass obu modułów.
- Przeciąganie punktu zmienia częstotliwość i gain; kółko nad wybranym punktem reguluje Q. Gesty begin/end trafiają do hosta. Zmiana wyboru pasma nie zmienia pozostałych pasm.
- Złota krzywa: odpowiedź EQ z faktycznych współczynników DSP i blendu bypassu, przekazana SPSC FIFO. Cień: odpowiedź wybranego filtra. To nie charakterystyka całego toru z limiterem.
- Turkusowe widmo: energia L/R, FFT Hann 4096–32768, domyślnie 8192, interpolacja binów i wygładzanie czasowe opadania. Skala widma -100–0 dBFS jest niezależna od osi EQ ±18 dB.
- FFT i grafika pracują poza callbackiem. Zmiana rozmiaru FFT nie zmienia latencji audio. Nie ma atrap przebiegów.
- Vectorscope oraz output peak/RMS/correlation z okna 400 ms. Ostrzeżenie, gdy EQ znajduje się za limiterem, ponieważ może podnieść poziom ponad ceiling modułu.
- Tooltips kontrolek. Resize 750×550–2000×1400. Testy używają rzeczywistej pętli komunikatów JUCE, a nie tylko jednorazowego paint.

## Walidacja

Testy GUI otwierają edytor trzykrotnie, zmieniają rozmiar, wybierają pasmo 16, zmieniają kontrolki, przeciągają węzeł, regulują Q kółkiem, sprawdzają gesty hosta i obecność krzywej wynikającej z audio. Przełączają wszystkie cztery rozmiary FFT bez zmiany PDC. Zrzut pochodzi z syntetycznego sygnału testowego, nie z makiety.

## Otwarte wymagania

Zoom 75–200% i HiDPI, osiem pełnych widoków, dodawanie/usuwanie/powielanie modułów, A/B, input/output overlay, freeze/peak hold, historia loudness, edycja pozostałych typów filtrów i trybów kanałowych. Zmiana wymiarów nie jest równoznaczna z ukończonym zoomem.


## Limiter Workspace

Oddzielny przycisk widoku, trzy kontrolki Drive/Release/Sample Ceiling, bypass modułu, historia GR i output peak, maksimum widocznego zakresu oraz odczyt PDC. Dane 10 ms są generowane w DSP; repaint nie tworzy wartości pomiarów. Okno pokazuje do 10 s audio, obwiednia jest ograniczana wizualnie do 24 dB (wartość maksimum nadal pokazuje głębszą redukcję). Wskaźnik braku nowego audio jest statusem dostarczania danych, nie rozpoznaniem transportu hosta. Linie nie łączą luk utraconych ramek. Generacja i zegar wykrywają reset historii. Po ponownym otwarciu edytor odbiera pozostałe ramki kolejki; pełna historia sesji nie jest gwarantowana.

Test GUI przełącza widoki, sprawdza widoczność i powiązanie nowych kontrolek z parametrami, obecność niezerowej redukcji na wykresie i trzy rozmiary okna. Zapisuje również `build-plugin/limiter-smoke.png`; workflow Windows archiwizuje oba zrzuty.


## Studio — przebudowa interfejsu

Wspólny `StudioTheme.h` rysuje własne pokrętła ze wskazówką, podziałką i pierścieniem, zakładki z widocznym zaznaczeniem oraz większe wartości numeryczne. Układ ma pasek tożsamości, stale widoczny łańcuch z zaznaczonym modułem, większy obszar wykresu i cztery karty parametrów. Kliknięcie karty zmienia widok; przeciągnięcie nadal zmienia kolejność. Domyślny rozmiar to 1200×800, minimalny pozostaje 750×550.

Limiter oddziela obszar redukcji (0–24 dB) od historii wyjściowego peak (-60–0 dBFS). Wypełnienie i linie pochodzą z tej samej telemetrycznej historii. Oś czasu pokazuje ostatnie 10 sekund audio. Boczne bary podpisane Peak/RMS reprezentują metryki 400 ms całego stereo, nie kanały L/R i nie normowany loudness. Przy małym oknie znikają bary, pozostają wartości liczbowe. EQ ma wypełnione widmo, wykres odpowiedzi i odczyt korelacji z paskiem -1..+1. Grafika i gradienty pozostają w rendererze programowym.

Testy GUI używają ponad 10 sekund syntetycznego sygnału o zmiennej amplitudzie. Sprawdzają także kliknięcie karty oraz brak przecięć widocznych kontrolek w obu widokach przy trzech rozmiarach. Zrzuty w README są aktualnymi wynikami tych testów, nie renderami koncepcyjnymi.

Lista typu filtra i przycisk Band wiążą się z aktualnie wybranym pasmem. HP/LP/Notch nie używają gain; kontrolka gain jest wtedy wyłączona, a węzeł pozostaje na osi 0 dB. Krzywe uwzględniają bypass pasma, nie tylko globalny EQ. Test GUI sprawdza wszystkie typy na paśmie 16 i izolację od pasma 1.

Undo/Redo jest oparte na UndoManager/APVTS. Węzeł EQ obejmuje frequency i gain w jednej transakcji; kółko Q i zmiana kolejności mają osobne granice. Presety `.jmlx` zawierają cały stan, mają limit 1 MiB i tę samą walidację/migrację co stan hosta. Zapis plików i systemowy chooser pracują wyłącznie na wątku UI. Wczytanie jest transakcją Undo; przywrócenie stanu hosta resetuje historię. Host automation może być rejestrowana w historii; nie deklarujemy oddzielnego systemu historii wyłącznie dla ręcznych zmian.


## Master Inspector

Trzecia zakładka pokazuje rzeczywiste wejście/wyjście, peak i RMS, LUFS M/S/I, stereo, headroom i przekroczenia full scale. Duże wartości w dolnych kartach to integrated wejścia/wyjścia, korelacja i balans energii. System pomiaru jest niezależny od GUI. RESET METERS nie jest resetem parametrów ani audio; jest wykonywany na wątku audio jako operacja O(1). W najmniejszym oknie część szczegółów (M/S i odczyty pomocnicze) jest ukrywana, ale integrated pozostaje widoczne. Preset, Undo/Redo i trzy zakładki mieszczą się przy minimalnej szerokości. Testy obejmują wszystkie trzy widoki i brak przecięć kontrolek przy trzech rozmiarach.


Master Brain: zaimplementowano analizę wejścia poza callback, regułowe propozycje EQ/limitera, panel konfiguracji i raportu, zatwierdzanie oraz jedną transakcję Undo. Zakres, algorytm i ograniczenia opisano w [MASTER_BRAIN.md](MASTER_BRAIN.md). Nie jest to kompletna docelowa implementacja v0.4.0.
