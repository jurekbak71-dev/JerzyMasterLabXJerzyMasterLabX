# GUI

Test ponownego otwierania i renderowania przy trzech rozmiarach przeszedł na Xvfb/Openbox; nie obejmuje testów HiDPI.

Aktualnie: ciemny Master Hub, output FFT, vectorscope, peak/RMS/correlation przeciąganie dwóch kart do zmiany kolejności oraz standardowy edytor parametrów JUCE. Zmiana wymiarów 750×550–2000×1400. Brak wymagania GPU. Rzeczywiste próbki audio z FIFO; brak sztucznych animacji.

Do implementacji: zoom 75–200%, osiem widoków, dynamiczne dodawanie i powielanie modułów, Undo/Redo, tooltips, edycja krzywej EQ, widma wejścia/wyjścia i rozszerzone testy GUI oraz HiDPI. Resizable window nie jest równoznaczne z ukończonym zoomem i testami DPI.
