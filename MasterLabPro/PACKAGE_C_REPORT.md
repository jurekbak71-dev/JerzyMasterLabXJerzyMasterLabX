# Package C — IN PROGRESS

Initial VST3 target and fixed 640 normalized parameters. Temporary generic JUCE editor is a development diagnostic only, not the approved A+C GUI. No claim of FL Studio compatibility. The processor currently restores rack state but does not yet expose rack editing. Module scan uses common application data Jerzy/MasterLabPro/modules; installation pending. State callbacks currently serialize using JUCE callback lock; this is a known realtime integration limitation to replace before acceptance. No production DSP modules. CI pending.

A+C rack editor is now connected in source, replacing the generic editor. The active graph handles normalized automation without reading mutable rack topology. GUI topology operations no longer take the callback lock. State callbacks retain that lock; DAW compatibility, pluginval, installer and CI for this revision remain unverified.
