# MasterLabX — instalacja Windows x64 VST3 (wersja testowa)

## Co zawiera paczka
- `JerzyMasterLabX.vst3` — rzeczywista wtyczka Windows x64.
- `INSTALL_MASTERLABX.cmd` — uruchomienie instalatora dwuklikiem.
- `install_masterlabx.ps1` — walidacja SHA-256, kopia poprzedniej wersji i instalacja z wycofaniem zmian po błędzie.
- `MASTERLABX_SHA256.txt` — SHA-256 binarium VST3 z tej samej kompilacji CI.
- `OFL-Manrope.txt` — licencja osadzonej czcionki.

**UWAGA:** Paczka została przygotowana dla gałęzi `fix/masterlabx-brain-on-current-modules-20261123`, czyli bieżącego projektu masteringu **bez STEMS**. Obejmuje 20-sekundowy Master Brain, analogowy moduł EXCITE Tube/FET/Tape oraz moduły z gałęzi 10 listopada 2026 r. Nie zastępuje testu w FL Studio na komputerze użytkownika. Nie instaluj paczki pochodzącej ze starszego PR #22 jako nowszej wersji.

## Instalacja
1. Zamknij FL Studio i inne programy hostujące MasterLabX.
2. Rozpakuj cały ZIP do dowolnego folderu. Nie uruchamiaj instalatora wewnątrz ZIP-a.
3. Uruchom dwuklikiem `INSTALL_MASTERLABX.cmd` i zaakceptuj uprawnienia administratora.
4. Instalator sprawdzi SHA-256 binarium **przed zmianą czegokolwiek**. Jeżeli masz starszą wersję, przeniesie ją do katalogu z datą `.backup-YYYYMMDD-HHMMSS` obok nowej wtyczki. Następnie skopiuje nową wersję do katalogu tymczasowego, ponownie sprawdzi SHA-256 i dopiero po poprawnej walidacji nada jej docelową nazwę.
5. Otwórz FL Studio i przeprowadź ponowne wyszukanie wtyczek VST3.

Domyślna ścieżka: `C:\\Program Files\\Common Files\\VST3\\JerzyMasterLabX.vst3`.

## Weryfikacja bez instalowania
W PowerShell uruchom z rozpakowanego katalogu:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\\install_masterlabx.ps1 -VerifyOnly
```

Ta komenda sprawdza zawartość i SHA-256, **niczego nie instaluje ani nie wymaga administratora**.

## Wycofanie aktualizacji
Przy zamkniętym FL Studio przenieś aktualny katalog `JerzyMasterLabX.vst3` poza `Common Files\\VST3`, a zachowany katalog `JerzyMasterLabX.vst3.backup-...` z wybranej daty zmień na `JerzyMasterLabX.vst3`. Do zmian w tym katalogu potrzebne są uprawnienia administratora. Instalator sam przywraca poprzednią wersję po błędzie kopiowania. Stare projekty DAW i presety **nie są przez instalator kasowane**.

## Zakres potwierdzenia
CI sprawdza: kompilację ClangCL Windows x64, CTest DSP, smoke GUI/zmiany rozmiaru, pluginval strictness 5, integralność ZIP oraz identyczność SHA-256 binarium przed i po rozpakowaniu paczki. Rzeczywiste uruchomienie na komputerze użytkownika, jakość muzyczna i niezawodność przez długi czas wymagają osobnego sprawdzenia. Nie należy utożsamiać zielonego CI z finalną akceptacją produktu.

Źródło zmian: https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/pull/23
