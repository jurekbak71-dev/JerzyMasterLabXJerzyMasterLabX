# Architektura

`Source/Dsp.h` nie zależy od JUCE. `Engine::prepare` alokuje dwa bufory opóźnienia; `process` pracuje bez alokacji i mutexów. Opóźnienie pozostaje aktywne przy bypassie. Kolejność dwóch stałych procesorów wybiera atomowy parametr; nie istnieje jeszcze dynamiczny graf i przełączenie kolejności nie jest crossfadowane.

Wrapper obsługuje tylko stereo. APVTS przechowuje parametry hosta i stan niezależnie od GUI. FFT wykonuje timer edytora; SPSC FIFO przekazuje próbki z audio. Zamknięcie GUI może przepełnić FIFO (próbki analizy są pomijane), ale nie zatrzymuje DSP. Otwarta jest najwyżej jedna instancja edytora na procesor.

Dalszy rozwój: stabilne ID instancji modułów, stała pula parametrów automatyzacji, niezmienne snapshoty grafu przekazywane przez kolejkę z odzyskiwaniem pamięci poza audio, crossfade wyrównanych latencją torów, wersjonowana serializacja i walidacja stanów. MASTER BRAIN pozostanie poza grafem audio.
