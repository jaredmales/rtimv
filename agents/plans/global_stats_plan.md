# Global Statistics Plan — Issue #42

## Scope and clarified behavior

This implements [issue #42](https://github.com/jaredmales/rtimv/issues/42), revised by the user's follow-up: the separate
full-image readout is **Global stats**, with an independent **Use mask** option for statistical selection. It works in
local `rtimv` and remote `rtimvClient`/`rtimvServer` modes, independently of the statistics box.

- Global statistics are initially disabled. When enabled, they respect the mask by default, as requested by the user.
- **Use mask** is initially on. When on, sample only finite processed image values at locations whose ordinary mask
  value is exactly `1`. Excluded locations never contribute zeros. The saturation mask does not select samples.
- With **Use mask** off, sample all finite processed image values without requiring a mask.
- Statistical selection and image masking remain independent. `--mask=false --stats.global=true --stats.mask=true`
  leaves the image unmasked while reporting statistics only within the selected mask locations.
- Values come from the existing `m_calData` processing pipeline, honoring dark subtraction, ordinary masking, filters,
  and MTF. Unrestricted statistics include zeros produced by image masking. No alternate processing branch is added.
- With MTF enabled, optional mask selection uses output-image coordinates, as box statistics do; the source mask is
  not transformed into a frequency-domain validity mask.
- Report min, max, mean, median, and finite sample count. Use a double mean accumulator and the existing median convention.
- Missing science data or no finite samples invalidate results. Missing/mismatched masks invalidate results only when
  mask selection is enabled. Preserve both requested settings through invalid input and recovery.
- Preserve the blank heading line and the existing formatting and placement of the separate overlay. Keep it clear of
  the pointer readout, status text, and box-statistics overlay.

## Implementation

### Backend

Rename the shared state, accessors, foundation notifications, and workspaces to global-statistics terminology. Add an
explicit `m_useMask` field to the coherent result. Apply the enabled and selection settings together under the existing
image-change serialization lock; keep result publication protected by its own mutex.

When mask selection is enabled, snapshot exact-1 membership alongside calibration input under the calibrated-data and
raw-data locks. Otherwise, skip mask validation and membership work. Reduce all finite processed pixels or the selected
subset after normal processing. Reuse median workspace; recompute immediately on either setting change, including on
static FITS data. Handle missing inputs and recovery without requiring a new science frame. Notify GUI/transport readers
after releasing processing locks.

### Configuration and transport

- `stats.global=true/false` controls the readout and calculation.
- `stats.mask=true/false` controls mask selection independently, including while the readout is off.
- Both settings use `[stats]` in config files and explicit boolean values on the command line.
- `mask=true/false` continues to control ordinary image masking; supplying a mask enables that by default if unset.
- Preserve server startup defaults. A remote client can override either global-statistics setting without overriding
  the other. Preserve current requested settings through reconnects.
- Replace the previous `SetMaskStats` feature RPC with `SetGlobalStats`, carrying both settings atomically. Keep requests
  asynchronous, serialized/coalesced, deadline bounded, and cancellable through shutdown. Apply responses through the
  existing owner-checked foundation slots, and publish measurements only from accepted `Image` replies.
- Reserve the former `Image.mask_stats` field number/name and use a new `global_stats` field with enabled state,
  selection policy, availability, count, and values. Earlier feature builds must not mistake unrestricted global results
  for mask-selected results. Older servers are detected as unsupported by message absence.
- Replace the earlier startup option `mask.stats` with `stats.global` plus `stats.mask`; document the migration.

### UI and documentation

Use a **Global stats** checkbox and a separate **Use mask** checkbox on the **Setup** tab. Read backend state and
pending intent, blocking signals during refresh. Either control may be changed without toggling ordinary image masking
or the statistics box. Keep the view display-only. Update the user guide, plan, and standing stats semantics in `AGENTS.md`.

## Validation

Extend the existing backend, client, and local/remote GUI tests:

| Case | Expected behavior |
| --- | --- |
| Whole image `[1,100;3,200]` | Count 4, min 1, max 200, mean 76, median 51.5 |
| Same image with mask `[1,0;1,0]`, mask selection on | Count 2, min 1, max 3, mean/median 2 |
| Image masking on, statistical mask selection off | Include processed zeros in the whole-image result |
| No mask or mismatched mask, statistical mask selection off | Valid whole-image result |
| No mask or mismatched mask, statistical mask selection on | Unavailable reason, no stale result |
| Zeros, negatives, non-finite values, empty sample, one-pixel/nonsquare images | Correct finite sample and bounds |
| HP/LP and MTF, either selection mode | Statistics match the appropriate current processed pixels |
| Static FITS image; either setting toggled | Immediate local and authoritative remote result |
| Config file, CLI override, server defaults, one-setting client override | Each explicitly configured setting wins independently |
| Rapid toggles, stale replies, reconnect, deadline, shutdown | Last requested pair wins; no stale results or callback lifetime errors |
| Both GUI controls, box stats, pointer text, zoom/resize | Independent behavior, preserved blank-line height, readable placement |

Build all three applications and run CTest with loopback networking available for gRPC fixtures. Format touched C++
files, preserve protobuf field numbers, and run `git diff --check`. Inspect offscreen screenshots of the image and control
panel. Physical-display checks and sustained acquisition with deployment-specific image sources remain follow-up validation.

### Validation results

- All application and test targets built successfully with `cmake --build _build -j4`.
- All 11 CTests passed in a complete normal run, covering local/remote controls, startup settings, processing, and
  asynchronous client behavior (36.50 seconds).
- The remote startup GUI test passed two sets of three consecutive runs with a temporary crash tracer.
- Offscreen image/control-panel screenshots confirmed the separate controls, preserved blank heading line, and
  non-overlapping global and box readouts. Touched C++ files were formatted and `git diff --check` passed.
- Three runs of the remote startup GUI test aborted in gRPC with `ref_counted.h:183: prior > 0`. Two debugger
  runs and the traced runs did not reproduce it. The root cause remains unconfirmed; retain this as an
  open startup/shutdown validation issue rather than treating successful retries as a fix.
- The reported deployment issue where `mask=false` was ignored was reproduced against the running remote server:
  startup masking stayed on while the runtime toggle worked. The user confirmed using service `start` instead of
  `restart`, leaving the older server process running. Updated client/server deployments must restart the server.

## Affected files

- `src/common/rtimvGlobalStats.hpp`, `rtimvBase.hpp/.cpp`, and `rtimvBaseObject.hpp/.cpp`.
- `src/server/rtimvClientBase.hpp/.cpp`, `rtimvServer.hpp/.cpp`, and `rtimvServerThread.cpp`.
- `src/proto/rtimv.proto`.
- `src/gui/rtimvControlPanel.hpp/.cpp/.ui` and `rtimvMainWindow.hpp/.cpp`.
- `src/librtimv/rtimvGraphicsView.hpp/.cpp`.
- `tests/CMakeLists.txt`, `tests/common/rtimvBase_globalStats_test.cpp`,
  `tests/client/rtimvClientBase_globalStats_test.cpp`, and `tests/gui/rtimvGlobalStats_gui_test.cpp`.
- `doc/UserGuide.md`, `AGENTS.md`, and this plan.

## Follow-ups

Weighted masks, a separate global-statistics dialog, additional statistics, and a dedicated shortcut remain outside this
change. Prior synthetic development-build measurements for mask-selected statistics on a 1024×1024 image with a half
mask and a 3-pixel mean low-pass filter were about 367 ms/frame disabled versus 399 ms/frame enabled, with approximately
3 MiB of sample/membership storage before allocator overhead. These are not deployment throughput estimates; unrestricted
global statistics may need median storage for the whole image, but do not need membership storage.
