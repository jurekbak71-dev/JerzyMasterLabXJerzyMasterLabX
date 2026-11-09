# MasterLabX LIVE Preview — Windows x64

**Experimental noncommercial build.** Neural separation is connected to actual incoming VST3/FL Studio audio via an asynchronous ONNX worker. Stable Windows real-time performance and FL Studio compatibility remain unqualified.

## Installation

1. Close FL Studio and extract the entire preview ZIP.
2. Read the supplied licence notice at licenses/StemgenRT/WEIGHTS-LICENSE.md and model card.
3. Double-click INSTALL_MASTERLABX_LIVE.cmd and type ACCEPT only if you agree to noncommercial model evaluation.
4. The installer downloads the three pinned partition models and verifies their SHA-256 before installing them to %ProgramData%/JerzyMasterLabX/StemgenRT.
5. A backup of the previous VST3 is preserved. Existing offline HTDemucs engine and model are not removed if previously installed.
6. Set FL Studio to **44.1 kHz**, rescan the plug-in and enable **LIVE NEURAL** in Stem Brain. The existing DRUMS/BASS/OTHER/VOCALS gain/tone/width/mute/solo controls act on the live-separated signal.
7. At 48/96 kHz or without models the live path is unavailable; the earlier DSP graph continues.
8. The neural path buffers 4096 samples (92.88 ms at 44.1 kHz) plus the pre-existing mastering DSP latency, reported to the host as PDC. Actual round-trip latency also depends on audio interface and host.
9. If underruns or artifacts occur, turn off LIVE NEURAL and compare against the existing offline mode.

## Limits and licensing

This is not the completed full MasterLabX feature set. Independent per-stem EQ/dynamics, unlimited EQ bands, Master Brain module orchestration and 15% DSP authority remain unfinished. Real-world Windows/FL Studio endurance and quality tests must be performed.

The preview package deliberately contains no AI model weights. The end-user downloads models for individual noncommercial evaluation after accepting the terms. Original StemgenRT code is MIT; weights v0.7/v0.8 are CC BY-NC-SA 4.0, and upstream flags unresolved rights regarding training datasets. Redistributable commercial clearance is not established.
