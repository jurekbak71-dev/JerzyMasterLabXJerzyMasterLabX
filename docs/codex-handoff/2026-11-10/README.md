# MasterLabX — przekazanie zadania do Codexa (START 2026-11-10)

Użytkownik zatwierdził rozpoczęcie prac słowem START. Ten PR zawiera **instrukcje**, a nie zmiany DSP, GUI ani uruchomiony proces Codexa.

## Czytaj w tej kolejności

1. [Pełne zlecenie implementacyjne](CODEX_MASTERLABX_FULL_IMPLEMENTATION_PROMPT.md)
2. [Manifest grafik i ograniczenia transferu](ASSET_HANDOFF_MANIFEST.md)
3. [Zatwierdzone Concept C i zgłoszenia użytkownika](https://app.notion.com/p/4105fed1901b810e9472e11439017087)
4. [Stem Brain LIVE: status i ograniczenia](https://app.notion.com/p/4115fed1901b81c4a099d97f56279132)
5. Aktualny kod, gałęzie i CI; najpierw audyt.

### Uwaga o materiałach binarnych

Do rozmowy załączono archiwum `MasterLabX_Codex_GUI_DSP_Package_2026-11-10.zip` zawierające referencje PNG i pełny lokalny manifest. **Archiwum nie zostało opublikowane w GitHub**, gdyż dostępne narzędzia połączenia nie umożliwiły przeniesienia binariów z lokalnego kontenera. Codex musi je otrzymać jako osobny załącznik lub przez późniejszą, rzeczywistą publikację zasobów.

### Zakaz nieuzasadnionych deklaracji

Nie nazywać gotowym interfejsem samej makiety; nie nazywać działającym miernikiem statycznej grafiki; nie twierdzić, że FL Studio, true peak albo model AI zostały przetestowane, jeśli nie zostały. Wyniki CI odnoszą się tylko do konkretnych commitów. Osobny projekt Jerzy REBALANCE pozostaje poza zakresem.

### Wytyczna organizacyjna

Prace implementacyjne prowadź w osobnej gałęzi po sprawdzeniu stanu PR #18, #19 i #20. Nie zakładaj, że gałąź z tym PR jest właściwą bazą kodu. Nie scalaj do main bez wyraźnej zgody użytkownika.
