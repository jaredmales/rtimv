# Quality Shortcuts And User Guide Plan

## Goal

Add keyboard shortcuts to adjust remote JPEG transport quality in `rtimvClient`, update the in-app help overlay, and bring `doc/UserGuide.md` back into sync with the currently implemented shortcuts and configuration options.

## Current State

- `src/gui/rtimvMainWindow.cpp` owns the keyboard shortcut dispatch in `rtimvMainWindow::keyPressEvent()`.
- The online help text is generated in `rtimvMainWindow::generateHelp()`.
- JPEG quality is already a supported runtime setting for the gRPC client/server path:
  - `src/server/rtimvClientBase.cpp` exposes `quality()` and `quality( int )`.
  - `src/server/rtimvServer.cpp` and `src/server/rtimvServerThread.cpp` already support quality state and RPC updates.
- Local `rtimv` does not expose a JPEG quality API, so the new shortcuts must remain gRPC-client-only.
- `doc/UserGuide.md` is currently missing or inconsistent for:
  - the remote JPEG quality option
  - the rolling transport stats option
  - some existing keyboard shortcuts such as warning-border cycling
  - several descriptions/typos in the shortcut and configuration tables

## Implementation Plan

1. Add `Q` and `q` handling in `rtimvMainWindow::keyPressEvent()`.
   - Gate the behavior with `#ifdef RTIMV_GRPC` so local `rtimv` builds are unchanged.
   - Use the existing `quality()` getter/setter.
   - Adjust in steps of `5`.
   - Clamp to `[0, 100]`.
   - Reuse the existing transient on-screen text for user feedback.

2. Update `rtimvMainWindow::generateHelp()`.
   - Add `q` / `Q` entries for JPEG quality.
   - Add any existing shortcut currently implemented but omitted from the help overlay, especially `w`.
   - Keep the help text conditional so local builds do not advertise unsupported shortcuts.

3. Audit and update `doc/UserGuide.md`.
   - Refresh the keyboard shortcut table to match the implementation.
   - Mark `q` / `Q` as `rtimvClient`-only.
   - Add missing configuration rows from the current code:
     - client `quality`
     - client `update.rollingStatsFrames`
     - `log.appname`
     - `no-log-appname`
     - server default `quality`
   - Correct obvious wording/typos while keeping the content scoped and minimal.

4. Format and verify.
   - Run `clang-format` on touched C++ sources.
   - Build the affected target if practical.
   - Sanity-check that docs and help text agree on shortcut names and scope.

## Verification Checklist

- Pressing `Q` in `rtimvClient` requests quality `+5`, capped at `100`.
- Pressing `q` in `rtimvClient` requests quality `-5`, floored at `0`.
- The online help overlay lists the new quality shortcuts and existing `w` shortcut consistently.
- `doc/UserGuide.md` matches the current shortcut set and config option set called out above.

## Risks / Edge Cases

- `rtimvClientBase::quality( int )` is asynchronous and the authoritative value still comes from `ImagePlease`; shortcut feedback should not bypass that state flow.
- Advertising `Q` / `q` in local `rtimv` would be misleading, so documentation needs to clearly note the remote-only scope where relevant.
