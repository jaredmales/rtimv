# Mask Statistics Plan — Issue #42

## Goal and scope

Implement [issue #42, “support stats in the mask”](https://github.com/jaredmales/rtimv/issues/42): calculate statistics
inside the selected pixels of the configured image mask. The
[clarifying comment](https://github.com/jaredmales/rtimv/issues/42#issuecomment-6251954496) requires a selectable mask-stats
mode independent of both the stats box and whether the mask is applied, with a readout styled like the stats-box overlay,
normally in the upper-left corner.

This proposal covers local `rtimv` and remote `rtimvClient`/`rtimvServer`. It adds a separate, initially disabled mask-stats
toggle; mask stats and box stats can be enabled simultaneously. It does not change box-statistics behavior, autoscaling,
mask multiplication, or saturation masking.

## Implementation status

Implemented for local and gRPC operation. The control panel now has a separate `Mask stats` checkbox and an independent
overlay. Backend results use the existing processed values at finite samples selected by mask values exactly equal to
`1`; excluded locations never contribute zeros. The additive protocol carries authoritative results in `Image`, with
asynchronous mode requests, pending-state display, reconnect restoration, and graceful older-server handling.

The full build and all nine CTest tests pass, including new backend, client lifecycle, and local/remote GUI coverage.
An offscreen rendered view was inspected with both readouts visible; a pointer-text collision found during inspection
was fixed and added to the GUI regression checks. Touched C++ files were formatted and `git diff --check` passes.
Physical-display checks and sustained acquisition with deployment-specific image sources remain follow-up validation.

A synthetic 1024×1024 image with a half-selected binary mask, mask application enabled, and a 3-pixel mean low-pass
filter measured 367 ms/frame with mask stats off and 399 ms/frame with them on (median of 12 refreshes after warmup,
using the current unoptimized development build). The additional work was about 32 ms/frame, or 9%, without another
filter pass. This selection needs 1 MiB of membership bytes and 2 MiB of median samples, excluding allocator/capacity
overhead. These are development measurements, not deployment throughput estimates.

## Implementation before this change

Reviewed against local commit `3779595`.

- [`rtimvBase`](../../src/common/rtimvBase.cpp) owns box-statistics calculation in `mtxL_calcStatsBox()`. It computes
  minimum, maximum, mean, and median from finite `calPixel()` values, using a reusable median workspace. Enabled box
  statistics are refreshed in `mtxUL_changeImdata()`.
- The ordinary mask is `m_images[2]`, configured by `mask.key`; `m_images[3]` is the separate saturation mask.
  `rawPixel()` selects calibration functions, including mask multiplication when `m_applyMask` is enabled.
  `m_calDataRaw` therefore can already contain multiplied pixels. HP/LP filters and MTF then produce `m_calData`.
- `updateImages()` already rebuilds calibrated data for updates to support images, including the mask. The comment
  “copy out mask to m_mask if valid” in `mtxUL_changeImdata()` is a placeholder, not an existing mask snapshot.
- [`rtimvMainWindow`](../../src/gui/rtimvMainWindow.cpp) formats the four statistics and places the existing overlay
  relative to a `StretchBox`. Its visibility depends on the box. The alternate
  [`rtimvStats`](../../src/gui/rtimvStats.cpp) dialog only polls backend values.
- [`rtimvServerThread::populateImageReply()`](../../src/server/rtimvServerThread.cpp) includes box-statistics state in
  `Image`; [`rtimvClientBase::ImageReceived()`](../../src/server/rtimvClientBase.cpp) applies it and emits a foundation
  update signal. This is the appropriate passive synchronization pattern for mask stats too.
- `markResponseDirty()` schedules an image response after state changes. A mask-stats toggle must use this path even
  for a static FITS image; waiting for the next camera frame would leave the UI stale.

## Proposed behavior and clarified mask semantics

The follow-up clarification is that the mask identifies pixels that should not be used, and statistics should preserve
that interpretation. Use the existing processed image, honoring the normal mask/filter pipeline, and exclude masked-out
locations from the statistical sample. A separate unmasked processing branch is not needed. The issue specifies a
binary mask; the implemented exact-1 rule below also defines the policy for nonbinary inputs.

1. **Selection:** use the entire image footprint, selecting pixels whose mask value is exactly `1`. Zero, non-finite,
   and other mask values are excluded. This follows the issue's “1 pixels” literally and does not turn a calibration
   mask into statistical weights. Supporting all finite nonzero values could be a later explicit policy change.
2. **Values:** report min, max, mean, and median from `calPixel()`/`m_calData`, using the same processed values and median
   convention as the stats box, with a double accumulator for the mean. Exclude non-finite sampled image values;
   zero-valued selected pixels remain valid samples. Mask-zero locations are excluded even if filtering makes their
   processed values nonzero; they do not contribute zeros to the mean or median.
3. **Independence:** enabling mask stats and selecting its sample do not depend on the stats box or `m_applyMask`.
   Mask stats work with mask application either on or off and always exclude mask-zero locations. They honor the
   existing dark-subtraction, mask-multiplication, HP/LP, and MTF pipeline. Toggling `M` may therefore change the statistics
   when it changes the processed values at selected locations. Do not force or temporarily change `m_applyMask`, rerun
   filters on unmasked data, or multiply the processed values by the mask again to calculate statistics.
4. **Validity:** distinguish enabled state from a valid result. Missing/disconnected masks, size mismatches, unavailable
   science images, and zero finite selected samples produce an unavailable result with a reason and sample count zero.
   Never silently fall back to whole-image statistics or present stale/zero results as a measurement. Retain the user's
   enabled choice so the readout recovers when valid inputs return.
5. **Presentation:** use a separate overlay headed `Mask stats`, followed by the existing four value labels and numeric
   formatting. Place it at the upper left with viewport padding and avoidance of other visible status/readout text.
   Show a concise unavailable reason instead of numbers when invalid. The existing Overlay/Dialog setting continues to
   control box stats; a second mask-stats dialog is outside this change.

Independence means independent controls and sample selection, not numerical invariance under processing changes. This
keeps mask stats consistent with box stats for the same selected processed pixels and preserves current filter behavior.
It does not introduce new mask-aware filter algorithms. With MTF enabled, this proposal applies selection at the
resulting image's pixel coordinates, consistent with box-statistics sampling of the displayed transform; a source-plane
mask does not become a transformed mask automatically.

## Implementation sequence

### 1. Add independent backend state and calculation

In `rtimvBase.hpp/.cpp`, add `Mask Statistics - Data` and `Mask Statistics` sections, with an enabled flag, result status,
finite sample count, four values, a mask-membership snapshot, and a backend-owned median workspace. Provide documented accessors and
`mtxUL_*`/`mtxL_*` entry points; keep definitions in the `.cpp`. Use a coherent result snapshot so consumers cannot read
values, count, and validity from different calculations.

- Snapshot mask membership while the existing raw-data lock protects mask dimensions and pixels, alongside the normal
  calibrated-data refresh. Follow the existing calibrated-data-then-raw-data lock order. Protect snapshot lifetime and
  result publication explicitly; a shared lock alone must not permit concurrent writes to workspaces or results.
- Reduce the existing `m_calData` after normal processing, using the corresponding membership snapshot. No extra image
  processing buffers, filter/FFT workspaces, or processing-pipeline refactor are required. Skip excluded locations
  entirely instead of multiplying by the mask and calculating statistics over the whole image.
- Calculate over all selected pixels, including the final row/column and one-pixel dimensions. Do not reuse stats-box
  coordinates or `normalizeStatsBox()`; this region is not a rectangular box.
- Reuse median storage between frames. Only perform mask-statistics work while enabled; avoid new per-frame allocations
  after dimensions settle. Keep existing box result semantics unchanged, including its current empty-region behavior.
- Recompute on enable, science-frame/cube changes, mask/support-image updates, mask-application toggles, and other
  processing changes. Enabling on a static image must compute immediately. Disabling clears the result and stops
  additional calculation.
- Detect input invalidation as well as new frames. `updateImages()` currently reacts primarily to image-update return
  codes, so verify mask loss/resize/reconnect paths explicitly and invalidate results even when no replacement frame
  arrives. Reuse the application's existing freeze/update cadence.
- Publish a `maskStatsUpdated` foundation notification after releasing calculation locks. The GUI supplies intent and
  renders results; it neither scans pixels nor starts a statistics worker.

### 2. Synchronize through the existing gRPC architecture

Extend `rtimv.proto` additively, without changing existing field numbers or the `StatsBox` RPC:

- Add a dedicated `SetMaskStats` enable/disable request and response, grouped with image modification RPCs after
  `ImagePlease` in the proto and corresponding server/client declarations.
- Add a mask-statistics state message to `Image`, containing enabled state, result status, finite sample count, and the
  four values. Message presence distinguishes an older server from a supported-but-disabled feature. Give the status
  enum an unavailable/unspecified zero value so absent data cannot look valid.
- In the server reactor, use the standard connected-image guard and `static_cast<void>(...)` for unused parameters.
  Call the backend's locking wrapper and mark the response dirty for enable, disable, and result-validity changes.
- Populate the new state in `rtimvServerThread::populateImageReply()` from one coherent backend snapshot. Ensure the
  normal dirty-response path delivers changes without requiring a new JPEG or camera frame.
- Mirror the public backend interface in `rtimvClientBase`, with cached state but no mask-membership or median workspace;
  document that those belong to the local/server backend. Update the cache exclusively from accepted `Image` state,
  then emit the matching foundation notification, including for accepted replies without JPEG payloads.
- Send the setter asynchronously with a deadline and tracked cancellation. Serialize/coalesce rapid toggles so the last
  requested state wins; keep pending user intent separate from authoritative cached state. Apply queued results through
  owner-checked foundation slots, release RPC bookkeeping locks before connection-error reporting, and drain callbacks
  during shutdown without waiting for Qt result delivery.
- Clear valid cached results on disconnect. Restore the current requested enable state after successful reconfiguration
  and wait for a fresh authoritative result. Older servers should leave the feature unavailable, not show zero stats or
  repeatedly issue an unsupported RPC; newer servers should continue serving older clients normally.

### 3. Add the independent UI toggle and readout

- Add a `Mask stats` checkbox near the stats controls in `rtimvControlPanel.ui`, with polling/update code and a slot in
  `rtimvControlPanel.hpp/.cpp`. Route changes through `rtimvMainWindow` and the backend interface. Reflect confirmed
  backend state and pending/unavailable status without emitting recursive toggle requests during refresh.
- Add an independent text widget in `rtimvGraphicsView.hpp/.cpp`, using the existing transparent, read-only overlay
  setup and font styling. Share value formatting with the box overlay without coupling mask visibility to a
  `StretchBox`, `doHideStatsBox()`, or dialog-close callbacks.
- Connect the foundation signal in `rtimvMainWindow`. Refresh placement on resize/zoom and relevant overlay changes.
  Keep both readouts visible when both modes are enabled; include the mask readout in collision checks so the box
  readout does not cover it. Keep all widget changes on the Qt GUI thread.
- Use the control-panel toggle as the initial selection mechanism. Preserve `s` for the stats box, `M` for mask
  application, and `Ctrl+M` for MTF. A new shortcut is optional and should be checked against plugin overlay keys before
  being assigned.
- Update `doc/UserGuide.md` with the toggle, mask source, inclusion rule, processing semantics, four reported values,
  unavailable states, and local/remote parity. Add mask-stats state to the info overlay if useful; update help if a
  shortcut is added.

### 4. Validate and document the change

Add focused backend and client tests to the existing CTest setup. Use synthetic inputs with known answers, and exercise
the actual backend integration as well as numerical reduction:

| Case | Expected result |
| --- | --- |
| Image `[1, 100; 3, 200]`, mask `[1, 0; 1, 0]` | Count 2; min 1, max 3, mean 2, median 2 |
| Select zeros, negative values, and a single pixel | Correct values/count; no rejection of valid zero measurements |
| Odd/even selected counts, full mask, last row/column, 1x1 and nonsquare images | Correct bounds, mean, and established median convention |
| Empty mask, missing mask, dimension mismatch, all selected values non-finite | Unavailable with count zero; no stale numbers |
| Nonbinary or non-finite mask values | Excluded according to the documented exact-1 rule |
| Toggle box stats, move/delete its box, change its display mode | Mask selection, values, and visibility remain independent |
| Toggle mask application with a binary mask and filters/MTF off | Same selected values and stats; excluded zeros never dilute the result |
| Toggle mask application with HP/LP or MTF on | Stats match the selected pixels of the current processed image in each mode; values may change |
| Filtering produces nonzero values at mask-zero locations | Those locations remain excluded from all four statistics and the sample count |
| Change dark subtraction, filter settings, science frame, or mask only | Recalculation follows the documented input/processing policy |
| Enable/disable on static FITS data | Immediate local update and a new authoritative remote response |
| Mask loss, resize, recovery, or client reconnect | Validity clears promptly and recovers without stale cross-session results |
| Delayed RPCs, rapid toggles, out-of-order image replies, shutdown | Final requested state wins; stale replies cannot restore old values; no deadlock/use-after-free |
| Older server/newer client and newer server/older client | Graceful unavailable feature or ignored additive fields |

Manually check both readouts together in local and gRPC modes, including small windows, zoom, panning, existing status
text, dark/bright backgrounds, and closing the control panel or stats dialog. Measure additional calculation time and
memory for a representative large live image with filters enabled; membership handling and median selection must not
stall the normal display unacceptably. Enabling mask stats must not introduce a second filter/MTF calculation per frame.

Build `rtimv`, `rtimvServer`, and `rtimvClient` with tests enabled, regenerate protobuf/gRPC code through CMake, and run the
new tests plus existing filter, async-race, lock-order, and reconnect coverage. Run `clang-format` on touched C++ sources
and headers. Apply `AGENTS.md` documentation conventions, remove `\author` from touched source/header file blocks, and
keep base/client declarations and section ordering aligned. Record the clarified mask-statistics semantics in
`AGENTS.md` so future changes preserve the same interpretation.

## Expected implementation files

| Files | Responsibility |
| --- | --- |
| `src/common/rtimvMaskStats.hpp` | Shared result and availability types |
| `src/common/rtimvBase.hpp/.cpp` | State, reduction of processed values, mask snapshot, locking, update/invalidation hooks |
| `src/common/rtimvBaseObject.hpp/.cpp` | Mask-statistics notification and any owner-checked forwarding slots |
| `src/proto/rtimv.proto` | Enable RPC and authoritative `Image` state |
| `src/server/rtimvServer.hpp/.cpp` | Reactor and mutation dispatch |
| `src/server/rtimvServerThread.cpp` | Snapshot publication and state-only response scheduling |
| `src/server/rtimvClientBase.hpp/.cpp` | Async request lifecycle, cached interface, reconnect behavior |
| `src/gui/rtimvMainWindow.hpp/.cpp` | Toggle routing, formatting, overlay refresh and positioning |
| `src/gui/rtimvControlPanel.hpp/.cpp/.ui` | Independent mask-stats control and backend state display |
| `src/librtimv/rtimvGraphicsView.hpp/.cpp` | Dedicated mask-statistics text widget |
| `tests/common/rtimvBase_maskStats_test.cpp`, `tests/client/rtimvClientBase_maskStats_test.cpp`, `tests/gui/rtimvMaskStats_gui_test.cpp`, `tests/CMakeLists.txt` | Numerical, lifecycle, protocol, and local/remote UI checks |
| `doc/UserGuide.md` | User-facing behavior and usage |
| `AGENTS.md` | Standing mask-statistics semantics |

The filter implementation does not need to change. Leave `rtimvStats` unchanged unless shared display formatting warrants
a narrowly scoped extraction.

## Risks and follow-up choices

- Exact-1 selection is implemented and documented for nonbinary masks. Use of the existing processed image is settled
  by the follow-up clarification; it does not require a second processing branch.
- Document that processing changes can change mask statistics, and that MTF selection uses output pixel coordinates.
  Do not imply that a spatial validity mask has been transformed into a frequency-domain validity mask.
- A live mask can change independently of the science image. Snapshot membership coherently and handle validity
  transitions; dimensions matching once is not sufficient protection against later resize/disconnection.
- Preserve existing stats-box behavior and protocol fields. Avoid combining this work with unrelated cleanup of its
  normalization, locking, or invalid-result semantics.
- Weighted masks, separate mask-stats dialogs, startup configuration, additional statistics, and a dedicated keyboard
  shortcut can be follow-ups; none is required for the requested selectable overlay.
