Problem: right now the help/info overlays are static, and only update if they've been toggled. we need to implement a method for the help overlays to update dynamically.  A good working case is the image age in the info overlay: it should update in the overlay whenever it is updated by the image itself.  Plugin overlays should likewise be able to signal an update is needed. Ideally the signal will indicate which line to update. Please review AGENTS.md and fill in this document with a plan below.

Plan:
1. Document the current shared overlay flow and keep the change centered in `rtimvMainWindow`.
   The current text overlay path is `registerTextOverlay(...) -> showTextOverlay(...) -> ui.graphicsView->helpTextText(...)`.
   Today the provider is evaluated only when the overlay is first shown, so `generateInfo()` and plugin `textOverlayText()` results go stale while the overlay remains visible.
   The least invasive fix is to keep the existing shared `helpText()` widget and add a refresh path in `rtimvMainWindow` rather than introducing a second overlay system.

2. Extend the text overlay registry so an active overlay can be refreshed in place.
   Add per-overlay refresh metadata to the existing `textOverlayEntry` in `src/gui/rtimvMainWindow.hpp`.
   Keep the existing full-text provider for backward compatibility.
   Add optional support for line-oriented refreshes so the active overlay can either:
   refresh the entire text block, or
   refresh a single line identified by index.
   Add helpers in `rtimvMainWindow` along the lines of:
   `refreshActiveTextOverlay()`
   `refreshTextOverlay(char key)`
   `refreshTextOverlayLine(char key, size_t line)`
   These helpers should no-op when the requested overlay is not active or not visible.

3. Hook overlay refresh into the existing image update path.
   The natural place is in `rtimvMainWindow.cpp` wherever image updates already drive UI refresh, especially the path that already updates the FPS/age gauge and calls `m_overlays[n]->updateOverlay()`.
   When the image reports `RTIMVIMAGE_AGEUPDATE` or a full image update, call the new overlay refresh helper for the active text overlay.
   This will make the built-in info overlay update its age text continuously without requiring a toggle.
   Keep the refresh conditional on an overlay actually being visible so normal image update cost stays minimal.

4. Keep built-in help and info overlays compatible with the new API.
   Leave `generateHelp()` and `generateInfo()` as the canonical full-text builders.
   For the first pass, allow the built-in overlays to refresh by regenerating the full text block; this is simple and low risk.
   If needed afterward, add an internal helper that returns the image info lines separately so the info overlay can update just the age line without rebuilding the whole string.

5. Add plugin-facing refresh signaling without forcing every plugin to change immediately.
   Use Qt signals/slots through `rtimvOverlayAccess::m_mainWindowObject`, which already exposes the main window `QObject *` to overlay plugins.
   Add `rtimvMainWindow` slots that accept a plugin overlay shortcut and request either:
   a full overlay refresh, or
   a single-line refresh.
   Plugins can then connect their own signals directly to these slots during `attachOverlay(...)`.
   This keeps visibility and active-overlay state in `rtimvMainWindow` and avoids making plugins track whether their overlay is currently visible.
   Preserve the current `hasTextOverlay() / textOverlayKey() / textOverlayText()` interface, and add only optional line-provider support for plugins that want targeted line replacement.

6. Keep line-level updates practical but not over-engineered.
   Store the currently displayed overlay text as split lines when an overlay is shown or refreshed.
   For a line refresh request, replace only that line, then push the rebuilt text back through `helpTextText(...)`.
   This still redraws the shared `QTextEdit`, but it gives plugins a stable way to target one line and avoids designing a brand new widget layout.
   If a plugin requests an out-of-range line, fall back to a full refresh and log once if useful.

7. Verify behavior with one built-in case and one plugin case.
   Built-in case:
   show the `i` overlay and confirm the image age changes while the overlay remains visible.
   Plugin case:
   use or create a plugin overlay that changes one reported line and emits a signal connected to the new main-window refresh slot.
   Confirm that:
   the visible overlay updates without retoggling,
   hidden overlays are not unnecessarily regenerated,
   help overlay behavior is unchanged,
   existing plugins without the new callback API still work.

8. Files likely to change.
   `src/gui/rtimvMainWindow.hpp`
   `src/gui/rtimvMainWindow.cpp`
   `src/librtimv/rtimvInterfaces.hpp`
   Possibly one plugin implementation or test fixture to exercise the callback-driven refresh path.

9. Main risks to watch.
   Refreshes must stay on the Qt GUI thread.
   Plugin callbacks must not outlive `rtimvMainWindow` shutdown.
   Full-text regeneration on every image age tick is acceptable as a first pass, but if a remote client drives very frequent updates we should watch for unnecessary churn and then tighten to line-only refresh for the info overlay.
