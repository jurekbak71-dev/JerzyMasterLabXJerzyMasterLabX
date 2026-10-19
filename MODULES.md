# Moduły

Phase A zawiera 16-pasmowy EQ z sześcioma typami filtrów i bypassem pasma z dedykowanym edytorem (drag, Q kółkiem, krzywa z DSP), FFT energii L/R i sample-peak limiter. Powtarzane instancje, presety per moduł i dynamiczny graf są zadaniami otwartymi.

Planowane: Spectral EQ Pro (16 pasm, M/S, dynamic, linear phase), Smart Dynamics, Ultimate Limiter, Bass Architect, Transient Sculptor, Stereo Architect, Analog Laboratory, Spectral Repair, Stem Processor, Clarity Engine, Reference Match. Master Brain będzie warstwą analityczną i propozycjami wymagającymi akceptacji użytkownika. Żaden z tych przyszłych modułów nie jest przedstawiany jako działający.


Master Brain: zaimplementowano analizę wejścia poza callback, regułowe propozycje EQ/limitera, panel konfiguracji i raportu, zatwierdzanie oraz jedną transakcję Undo. Zakres, algorytm i ograniczenia opisano w [MASTER_BRAIN.md](MASTER_BRAIN.md). Nie jest to kompletna docelowa implementacja v0.4.0.
