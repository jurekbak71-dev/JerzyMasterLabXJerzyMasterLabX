# Jerzy MasterLabPro

Independent new project inside this repository, isolated from legacy MasterLabX. Legacy main and root build remain untouched. Do not reuse legacy DSP without audit and listening tests.

Authoritative requirements: https://app.notion.com/p/4235fed1901b81b08df0e02b4422a493

Approved A+C photorealistic GUI; ten universal reorderable slots; global stereo input/output meters on right; real spectral and heat-map before/after/delta; delta per module; coloured frequency band buttons in all modules; right-click neutral reset; stable FL Studio automation independent of routing position. No Master Brain or STEMS.

Next: import and verify G3.02 source/assets, create independent CMake target with distinct VST3 identity, integrate RackEditor, build and test. Legacy repository uses JUCE 8.0.6 and C++20; inspect existing build and CI as reference only. Existing G3.02 JUCE compilation is unverified. No production release or merge authorized by this document.
