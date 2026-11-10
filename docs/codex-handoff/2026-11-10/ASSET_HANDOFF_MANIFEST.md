# MasterLabX — manifest przekazania grafik 2026-11-10

**Status: PLIKI BINARNE NIE SĄ W TYM PR.** Zestaw 11 PNG oraz pełny manifest JSON znajdują się w lokalnym archiwum załączonym do rozmowy: `MasterLabX_Codex_GUI_DSP_Package_2026-11-10.zip`. Codex nie uzyska ich przez sam checkout gałęzi; należy dołączyć to archiwum do zadania Codexa lub przenieść zasoby za pomocą obsługiwanej metody. Samo umieszczenie listy plików nie oznacza uploadu.

W chwili przekazania: lokalne `unzip -t` całego ZIP — OK. Poniższe sumy były zapisane w manifeście paczki. Przed integracją Codex powinien zweryfikować SHA-256 faktycznie dostarczonych plików.

| Ścieżka wewnątrz ZIP | Rozmiar obrazu / typ | SHA-256 |
|---|---|---|
| `assets/analog_exciter_main_excite_knob.png` | 1254×1254 | `ba694e220bd9feffb2988758569f27bae952981d1560e12b23aa48cf4f798366` |
| `assets/analog_exciter_secondary_knob.png` | 1254×1254 | `95b77a5d9a8b8c7e685cf51e989b0078a30a4d9c65caf8e063522048bd985579` |
| `assets/control_switch_active_reference.png` | 1448×1086 | `0bf4816170840834f4680a12da777a0187a7a46f9ce6b2888d41a00e8e26c913` |
| `assets/led_states_sprite_reference.png` | 2172×724 | `7fe7ed0ed8d450558dc6c74a87ddece1af2df19aed259ddceecd036d9cf49a7c` |
| `assets/oscilloscope_appearance_reference.png` | 1448×1086 | `a98fc8fa291279d5037f07997b13d9aadcd45ce0c58ae4eba7c6f8e430ec44d0` |
| `assets/harmonic_analyzer_appearance_reference.png` | 1448×1086 | `6e204c4e3665005ced3e6dfbdbf9d5c749f874d3c7b56a81365b4f5b6d13a17e` |
| `assets/gain_reduction_meter_appearance_reference.png` | 1448×1086 | `2656f9dde140cbffe26bfb6624ff05b82244f80b70ae88f2ef9451778c8fb9ff` |
| `references/analog_exciter_approved_layout_reference.png` | user screenshot | `8a06f90ecbbd7a1defb5dfc75816ea06c4ab400c0b899a98cb1bf9c229e510ab` |
| `references/analog_exciter_current_problem_reference.png` | user screenshot | `8a24be7191819e798d9678dfb504c4a69a74c15c1675c2a0ccbe3d3e29cf8412` |
| `references/oscilloscope_stability_reference_maximus.png` | user screenshot | `13c2994840ee1cbdd9011a760f051f6ad17747b342e934616f0027bd59921547` |
| `references/limiter_current_problem_reference.png` | user screenshot | `1045bf72ddb3167b9566bac591ba671d6fdfbb0882b5f6ea14c49dc3890e9475` |

## Niedozwolone zastosowania

W obrazach oscyloskopu, harmonik i GR są przykładowe wizualizacje, **nie dane DSP**. Nie podłączaj ich jako „działających” mierników. Pokrętła mają tylko jedną pozycję, nie stanowią atlasów animacji. LED-y są jednym arkuszem i wymagają separacji. Każdy obraz należy zweryfikować wizualnie i technicznie.

Źródła wymaganych makiet i decyzji: https://app.notion.com/p/4105fed1901b810e9472e11439017087.
