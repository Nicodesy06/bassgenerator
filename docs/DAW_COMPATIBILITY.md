# Host compatibility matrix

Legend: **validated** = observed working in that host; **harness** = verified by our automated JUCE VST3 host harness / pluginval (not a DAW); **not tested**; **not supported**.

| Feature | JUCE VST3 harness (Linux) | JUCE VST3 harness (Windows CI) | pluginval (Windows CI) | REAPER | Ableton Live | FL Studio | Bitwig |
|---|---|---|---|---|---|---|---|
| Discovery as instrument / load | harness | harness (PASS) | PASS (out-of-process); in-process FAIL in Editor Automation | not tested | not tested | not tested | not tested |
| Audio follows host transport | harness | harness (PASS) | – | not tested | not tested | not tested | not tested |
| Preview while stopped | in-process UI test | in-process UI test | – | not tested | not tested | not tested | not tested |
| State save / reload | harness | harness (PASS) | PASS (state + restoration tests) | not tested | not tested | not tested | not tested |
| Export MIDI (file dialog) | unit tests (file content) | unit tests | – | not tested | not tested | not tested | not tested |
| External drag of .mid | not testable headless | not testable headless | – | not tested | not tested | not tested | not tested |
| VST3 MIDI output | harness (20/20 note on/off) | harness (PASS) | – | not tested | not tested | not tested (FL generally does not route VST3 instrument MIDI out) | not tested |
| HiDPI 125/150/200 % | snapshot scale 1.5×/2× | – | – | not tested | not tested | not tested | not tested |

Notes
- Windows install (0.3.0): the installer always targets `C:\Program Files\Common Files\VST3` (admin) and removes the 0.1/0.2 per-user copy in `%LOCALAPPDATA%\Programs\Common\VST3`, which many hosts do not scan. The plug-in is linked against the static MSVC runtime (`/MT`); CI checks with `dumpbin /dependents` that it has no VCRUNTIME/MSVCP import, so it loads on machines without the VC++ Redistributable. Both were the likely causes of the user report "the VST3 does not install". The fix is verified by CI only (silent install + host harness on the installed bundle), **not yet confirmed on the user's PC / DAW**.
- macOS: the AU passes Apple's `auval` and the VST3 passes the JUCE host harness on the GitHub macOS runner (installed from the release .pkg). Logic Pro / GarageBand themselves are **not tested**.
- No DAW was available in the development environment (Linux container). Everything in DAW columns is **not tested**.
- External drag uses `juce::DragAndDropContainer::performExternalDragDropOfFiles` with a pre-written file (`canMoveFiles=false`). If a host's window does not accept the drag, the UI shows an error and EXPORT remains available.
- MIDI output routing from an *instrument* is host-specific. The `.mid` export/drag path is the guaranteed workflow.
- Imported `.mid` tempo may be ignored by DAWs (normal behaviour); note positions are in PPQ and stay correct.
