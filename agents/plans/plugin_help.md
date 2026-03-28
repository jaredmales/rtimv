Problem statement: review AGENTS.md.  I would like to implement overlayed help text for plugins.  This should would work like the existing 'h' help and 'i' info shortcuts.  For example, the "warnings" plugin would display the active warnings when 'w' is pressed.  The same dialog has h and i should exist.  So w shows, w then hides.  Hitting w after h switches from help to warnings, etc.  I am thinking this requires a mechanism for plugins to register their help shortcut so it can be handled properly, which then requires some sort of list of plugin shortcuts to handle and dispatchers to the existing or plugin help text generators.  Please analyze and suggest a plan in this doc below.

Plan:
Current behavior analysis

- `rtimvMainWindow` already has a single shared overlay text widget via `ui.graphicsView->helpText()`.
- The built-in `h` and `i` paths are hardcoded in `keyPressEvent`, and each uses its own visibility boolean (`m_helpVisible`, `m_infoVisible`).
- Those booleans are really acting as a crude "which text mode is active" state for the one shared widget.
- Overlay plugins currently receive raw `keyPressEvent` callbacks, but there is no central key registration or conflict resolution.  `rtimvOverlayInterface` even documents this limitation.
- Plugins already expose `info()`, but there is no analogous API for a plugin-provided overlay text panel.

Recommendation

- Move from separate `m_helpVisible` / `m_infoVisible` booleans to a single centralized "active text overlay" mode in `rtimvMainWindow`.
- Keep ownership of shortcut dispatch and widget visibility in `rtimvMainWindow`.
- Let plugins register optional text-overlay entries consisting of:
  - a shortcut key
  - a short label/title
  - a text generator callback or virtual interface method
- Treat built-in help/info as entries in that same registry so all toggling rules are identical.

Why this is the cleanest fit

- The existing widget is singular, so the state machine should also be singular.
- Central dispatch lets us define conflict behavior once and avoids every plugin independently trying to show/hide the same widget.
- It preserves existing plugin keyboard handling for non-help actions while adding a dedicated path for text overlays.
- It scales naturally from `h` and `i` to plugin keys like `w`.

Suggested design

1. Add a text-overlay registration concept

- Introduce a small struct in `rtimvMainWindow` (or a nearby shared header if needed) such as:
  - key
  - name
  - text provider
- Store these in a map keyed by `QChar`, `int`, or `Qt::Key`.
- Register built-ins during main window setup:
  - `h` -> `generateHelp()`
  - `i` -> `generateInfo()`

2. Track one active overlay mode

- Replace:
  - `bool m_helpVisible`
  - `bool m_infoVisible`
- With something like:
  - active overlay key / id
  - or an enum for built-ins plus plugin entries
  - plus a helper that answers whether any text overlay is visible
- Add one helper to apply the overlay text to `helpText()` and one helper to hide it.

3. Add a single toggle path

- Add a helper along the lines of:
  - `toggleTextOverlay( key )`
  - `showTextOverlay( key )`
  - `hideTextOverlay()`
- Behavior:
  - if requested key is already active: hide
  - if a different text overlay is active: switch contents and stay visible
  - if none is active: show requested contents

4. Extend plugin API minimally

- Chosen approach: add optional virtual methods to `rtimvInterface` for text-overlay registration.
- These methods should have default implementations in `rtimvInterface` representing the no-help case so existing plugins do not need to be modified.
- Suggested API shape:
  - `virtual bool hasTextOverlay()`
  - `virtual int textOverlayKey()`
  - `virtual std::string textOverlayTitle()`
  - `virtual std::string textOverlayText()`
- Suggested defaults in `rtimvInterface`:
  - `hasTextOverlay()` returns `false`
  - `textOverlayKey()` returns `0` or another invalid key sentinel
  - `textOverlayTitle()` returns `""`
  - `textOverlayText()` returns `""`
- I would avoid requiring plugins to connect signals just to register this, because plugin loading is already synchronous and registration data is static.

5. Register plugin text overlays during `loadPlugin`

- After a plugin is successfully attached, ask whether it provides a text overlay entry.
- Validate the key before registering:
  - reject duplicates with built-ins
  - reject duplicates between plugins
  - reject empty/invalid/non-printable keys
- Log collisions and skip the conflicting plugin text overlay registration rather than failing plugin load entirely.

6. Integrate with key handling

- In `rtimvMainWindow::keyPressEvent`, check the centralized text-overlay registry before forwarding to plugins for general key handling.
- For a registered overlay key, consume the key and call the unified toggle helper.
- This avoids double-processing where a plugin both toggles itself and also wants the main window to own the shared text widget.

7. Include plugin shortcuts in built-in help

- Update `generateHelp()` so the plugin-registered shortcuts appear in the shortcuts list.
- This keeps discoverability aligned with the existing `h` help panel.
- The formatting can remain simple at first, e.g. one line per plugin shortcut appended after built-ins.

Implementation notes

- The "warnings" plugin is a good first adopter:
  - register key `w`
  - return current warning text from its overlay text generator
  - let the main window handle all show/hide/switch behavior
- If plugin text needs to reflect live state, generate it on demand when the key is pressed rather than caching the text at registration time.
- Reuse the existing font-luminance behavior already applied to `helpText()` after setting text.

Potential edge cases

- Shortcut collisions with built-ins or other plugins.
- Uppercase vs lowercase semantics; recommend matching current built-in behavior and treating lowercase and uppercase distinctly only when intentionally registered.
- A plugin whose overlay text is empty; recommend showing an empty panel only if that is meaningful, otherwise skip display and log once.
- Plugin unload behavior is currently not dynamic; if runtime unload is ever added later, unregistering the text entry will need to be paired with it.

Concrete implementation order

1. Refactor built-in help/info into the unified text-overlay state machine in `rtimvMainWindow`.
2. Add registration storage and helper methods in `rtimvMainWindow`.
3. Extend the plugin interface with optional text-overlay metadata/text generation.
4. Register plugin entries during `loadPlugin`.
5. Update `keyPressEvent` to consult the registry first.
6. Append plugin shortcuts to `generateHelp()`.
7. Implement the first plugin consumer, likely warnings.
8. Verify:
  - `h` toggles help on/off
  - `i` toggles info on/off
  - `w` toggles plugin text on/off
  - `w` after `h` switches help -> warnings
  - `h` after `w` switches warnings -> help
  - duplicate plugin shortcut registration is rejected cleanly

Design decisions captured

- Keep this as a main-window-owned registry rather than inventing a signal/slot dispatcher for plugin help text.
- Put the optional text-overlay API on `rtimvInterface`, not `rtimvOverlayInterface`.
- Provide no-help default implementations in `rtimvInterface` so existing plugins remain compatible without source changes.
- If later you want plugins to update already-visible text asynchronously, we can add an optional refresh signal after the initial registration model is in place.
