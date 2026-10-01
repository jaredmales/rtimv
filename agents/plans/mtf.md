We want to implement a modulation transfer function (MTF) calculation and display.  The MTF is the modulas of the fourier transform of the image (assuming it's the image of a point source).  Please develop a plan to implement this.  Initial guidance include:
- use mxlib's FFTW interface.
- we can use the real-to-complex form, and fill in the other half of the output to give a full image
- add toggle to turn MTF on in the Filter's tab
- add a shortcut, probably 'M' if it's not in use

Review AGENTS.md and then develop the plan in this document below.  Do not edit this prompt, and do not begin modifications until I approve the plan.

Plan:

## Scope

Implement an optional display transform that replaces the currently displayed calibrated image with its modulation
transfer function (MTF), defined here as the modulus of the 2-D Fourier transform of the image, treating the image as a
point-source image. Keep the calculation in the shared backend path so local `rtimv` and gRPC `rtimvClient` behavior stay
aligned with the existing filter/state-sync architecture.

## Current Relevant Plumbing

- Image filtering already lives in `src/common/rtimvBase.*` and `src/common/rtimvFilters.*`.
- `rtimvBase::mtxUL_changeImdata()` fills `m_calDataRaw`, calls `mtxL_applyFilter()`, then recolors and publishes display
  state.
- Existing HP/LP filter state is synchronized through `src/proto/rtimv.proto`, `src/server/rtimvServer.*`,
  `src/server/rtimvServerThread.cpp`, `src/server/rtimvClientBase.*`, and `src/server/rtimvFilterGRPC.hpp`.
- The Filters tab is `src/gui/rtimvControlPanel.ui`, with update/slot logic in `src/gui/rtimvControlPanel.cpp`.
- `F` toggles HP/LP filters in `rtimvMainWindow::keyPressEvent()` and `rtimvMainWindow::toggleFilter()`.
- Uppercase `M` is already used by `toggleApplyMask()`, so the MTF shortcut needs either a different key or an explicit
  reassignment.

## Proposed Data Model

Add a separate MTF enable flag rather than modeling MTF as another HP/LP filter type:

- `bool m_applyMTF{ false }` in `rtimvBase`, documented under a new `Image MTF - Data` section placed near filtering.
- `void applyMTF( bool apply )` and `bool applyMTF()` accessors in `rtimvBase`, with definitions in
  `src/common/rtimvBase.cpp`.
- Matching cached state and accessors in `rtimvClientBase`, documented in the same section shape as `rtimvBase`.
- A display working buffer in `rtimvBase`, e.g. `mx::improc::eigenImage<float> m_mtfImage`, plus FFT work/planning state.
  `rtimvClientBase` should explicitly note that MTF working buffers are server/local-backend only.

This keeps HP/LP filtering and MTF independently toggleable, and lets MTF be applied after the existing calibrated/filter
pipeline.

## MTF Calculation

Create a small backend helper in `rtimvFilters` or a new focused pair such as `rtimvMTF.hpp/.cpp`. Prefer a new helper only
if the implementation makes `rtimvFilters` too crowded; otherwise keep the image-display transform near existing filters.

Implementation details:

- Use `mx::math::ft::fftT<float, std::complex<float>, 2, 0>` from `<mx/math/ft/fftT.hpp>` for the real-to-complex forward
  FFT.
- Reuse the FFT plan when image dimensions are unchanged, because mxlib/FFTW planning uses `FFTW_MEASURE` and can be
  expensive.
- Populate FFT input from the calibrated image currently selected by `m_calData`, after dark/mask and HP/LP filters.
- Generate an output image with the same dimensions as the input.
- Fill the directly produced real-to-complex half from the FFT output, then synthesize the conjugate-symmetric half so the
  display is a full-size modulus image.
- Apply an `fftshift`-style quadrant shift so zero spatial frequency appears at image center. This should be documented
  because it affects what users expect visually.
- Normalize by the DC modulus when it is finite and nonzero, so the peak MTF is 1.0. If DC is zero/non-finite, leave
  unnormalized finite moduli and allow existing stretch/min-max handling to display them.
- Treat NaN/Inf input pixels conservatively by converting non-finite values to 0 before the FFT, matching display stability
  expectations.

Potential edge cases to handle in the helper:

- Empty images.
- Odd/even dimensions when mirroring the real-to-complex half.
- 1-pixel dimensions.
- Non-finite DC or all-zero images.

## Backend Integration

In `rtimvBase`:

- Add MTF state/accessors and call `mtxUL_changeImdata( false )` when the flag changes.
- Extend `mtxL_applyFilter()` so the order is:
  1. Start from `m_calDataRaw`.
  2. Apply HP/LP filters exactly as today.
  3. If `m_applyMTF` is true, calculate MTF into `m_mtfImage` and set `m_calData = m_mtfImage.data()`.
  4. Otherwise leave `m_calData` pointing at the HP/LP result or raw calibrated buffer as today.
- Ensure statistics, color scaling, pixel readout, and stats-box values operate on the displayed MTF image when MTF is
  enabled, because those consumers already read `m_calData`.

In the client/server path:

- Add `SetApplyMTF(ApplyMTFRequest) returns (ApplyMTFResponse)` after `ImagePlease` or near the image-modification RPCs,
  following the project's gRPC grouping rule.
- Add `bool apply_mtf` to `Image` so clients passively refresh from `ImagePlease`.
- Add server unary reactor `SetApplyMTF` with the usual `static_cast<void>(...)` handling and connected-image guard.
- Add async client setter `rtimvClientBase::applyMTF(bool)` and cached getter, mirroring the existing HP/LP apply methods.
- Update `rtimvServerThread::fillImage()` and `rtimvClientBase::mtxL_updateImage()`/callback image-state assignment so the
  client cache is authoritative from received `Image` fields.

## GUI Integration

Filters tab:

- Add an "MTF" checkbox/toggle in `rtimvControlPanel.ui`, visually grouped with the Filters tab but not tied to HP/LP width
  controls.
- Add `update_mtf()` and `on_mtfApplyCheck_stateChanged(int)` in `rtimvControlPanel.cpp`.
- Include MTF in `update_panel()` alongside `update_hpFilter()` and `update_lpFilter()`.

Main window:

- Add `setApplyMTF(bool)` and `toggleApplyMTF()` methods in `rtimvMainWindow`, with transient messages like `MTF on` and
  `MTF off`.
- Add MTF state to `filterStatusString()` or a sibling status string so `Get Info` reports when MTF display is enabled.
- Add shortcut help text.
- Since uppercase `M` is occupied by mask toggle, proposed shortcut is lowercase `m` if it is unused after checking plugin
  text-overlay key handling, otherwise `Ctrl+M`. Do not steal existing uppercase `M` without explicit approval.

## Documentation and Tests

- Update `doc/UserGuide.md` with the Filters tab MTF toggle, shortcut, and a short note that MTF displays normalized Fourier
  modulus with zero frequency centered.
- Add focused unit tests for the MTF helper if the project test harness can link `commonlib` without launching Qt UI:
  - delta-like point source gives flat normalized modulus.
  - constant image gives only centered DC after shift.
  - odd and even dimensions produce full-size finite output.
- If adding a standalone helper test is too heavy, add a small executable test under `tests/` and wire it through
  `tests/CMakeLists.txt`.
- Run `clang-format` on touched C++/header/proto-generated-adjacent files and build at least `commonlib`, `rtimv`, and, when
  gRPC dependencies are enabled, `rtimvServer`/`rtimvClient`.

## Implementation Order After Approval

1. Add and test the MTF helper using mxlib FFT.
2. Add `rtimvBase` state/accessors and display-pipeline integration.
3. Add gRPC proto, server, and client state synchronization.
4. Add Filters-tab UI and shortcut/status/help updates.
5. Add docs/tests, run formatting, and build/verify.
