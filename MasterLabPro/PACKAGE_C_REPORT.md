# Package C — IN PROGRESS

Initial VST3 target and fixed 640 normalized parameters. Temporary generic JUCE editor is a development diagnostic only, not the approved A+C GUI. No claim of FL Studio compatibility. The processor currently restores rack state but does not yet expose rack editing. Module scan uses common application data Jerzy/MasterLabPro/modules; installation pending. State callbacks currently serialize using JUCE callback lock; this is a known realtime integration limitation to replace before acceptance. No production DSP modules. CI pending.

A+C rack editor is now connected in source, replacing the generic editor. The active graph handles normalized automation without reading mutable rack topology. GUI topology operations no longer take the callback lock. State callbacks retain that lock; DAW compatibility, pluginval, installer and CI for this revision remain unverified.

Intermediate state access: removed explicit getCallbackLock from state callbacks. Concurrent read opt-in for technical modules; legacy save and restore use a nonblocking audio exclusion gate that emits silence during the control operation. This is not seamless live restore and must not be described as accepted. Bypass PDC correction and state access regression pending CI.

Live host replacement implemented with a 256-sample crossfade and deferred reclamation. Equal-latency continuity unit test added. Actual VST3/DAW tests, unequal latency transitions and legacy save remain open.

Added warm-up, dual-path delay compensation and effective-latency reporting. A regression verifies equal latency amplitude continuity and faster replacement with retained 7-sample delay. Slower replacement time-reference change and DAW PDC remain acceptance blockers.

Added two-tap delay transition and warm-up restart on reported latency changes; regression includes slower replacement, impulse delay and mid-fade latency change. CI and actual DAW behavior remain unverified.

Validation: fixed GCC misleading-indentation error in LiveHostExchange. Added eight independent exchange instances with 100 replacement cycles each; measured elapsed wall time and maximum sample jump. This tests the exchange algorithm, not eight loaded VST3 instances. Added unsigned development Inno Setup package and CI artifact; installer build and installation remain unverified. Pluginval not yet integrated.

Added Windows runner silent install smoke, a native JUCE VST3 host loading eight real binary instances with independent Gain states, and pinned pluginval v1.0.4 strictness 5. All new checks pending CI. FL Studio acceptance remains manual. No production release.

Run 45798314749: Windows build, 31 CTests, sanitizers, installer build and silent install passed; native instance test failed because raw RackState bytes were passed to JUCE's hosted VST3 XML state API. Corrected test envelope per JUCE 8.0.6 VST3PluginFormat.cpp. Pluginval now runs even if the instance test fails (provided installation succeeded). New CI pending.

Run 45812761707: Windows 31/31, ASan/UBSan and TSan pass, installer build/install pass, pluginval v1.0.4 strictness 5 SUCCESS. Instance test measured 0.111686 vs 0.112202 for +1 dB. JUCE 8.0.6 AudioParameterFloat convenience constructor uses a normalized step of 0.01 (48 dB range => 0.48 dB steps); replaced with continuous explicit NormalisableRange. Module-local quantization remains authoritative. Test tolerance unchanged. Measurements from runner: 10 mixed modules median 79.4 us, p99 84.9 us, max 660.3 us per 256 samples; process RSS 20078592 bytes; graph PDC 128 samples; 800 exchange cycles 5.1755 ms, max sample jump 0.00390625, verified PDC 13 samples. These are technical-module benchmarks, not production DSP or FL Studio. Retest pending.

26 Nov 2026: user reports FL Studio crashes when touching module. Acceptance blocked despite run 45814911544 passing all automated checks. Confirmed code defect: find(0) returned an unused instance with null Type; empty slot selection calls descriptor(0), dereferences null. Fixed both find overloads, guarded empty selection, copied panel metadata under control mutex rather than exposing native descriptor pointers. Added invalid ID regression and actual plugin editor selection/reset/empty slot test after installation. Exact FL Studio crash stack unavailable; retest required.

User confirmed module selection works in FL Studio after 02fd721, but rejected tiny lower-panel controls. Increased module panel height, moved remove button into header, added horizontally scrollable control strip, descriptor-based real names/units and normalized-to-physical readout; stable host parameter IDs and right-click reset retained. Regression now checks actual dial size and Gain label/readout. Visual acceptance and CI pending.
