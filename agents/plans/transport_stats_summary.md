# Transport Stats And Control Panel Summary

## Overview

This summary captures the recent `rtimvClient` and control-panel updates for rolling transport statistics and related UI changes.

## Backend Changes

- Added rolling transport statistics to `rtimvClientBase`.
- Added configurable rolling window length with default `10` frames.
- Added client-side rolling average of:
  - compression ratio
  - frame arrival rate
- Kept these calculations in `rtimvClientBase` rather than GUI code.

## Compression Ratio Definition

- Compression ratio is now based on the native source image size, not the decoded Qt image size.
- Added `bytesPerPixel()` to the `rtimvImage` interface and implemented it for:
  - `shmimImage`
  - `mzmqImage`
  - `fitsImage`
  - `fitsDirectory`
- Added `rtimvBase::bytesPerPixel(size_t)` so server-side code can query native source pixel size.
- Added `source_bytes_per_pixel` to the gRPC `Image` message.
- `rtimvServer` now includes source bytes-per-pixel in each `ImagePlease` reply.
- `rtimvClient` computes compression ratio as:

```text
(nx * ny * source_bytes_per_pixel) / compressed_jpeg_bytes
```

## Arrival Rate Definition

- The rolling FPS-like metric now measures local frame arrival rate on the client.
- It no longer uses remote `fpsEst`, which reflects source acquisition cadence.
- The arrival rate is computed from monotonic time deltas between consecutive `ImageReceived()` calls.

## Control Panel Changes

- Added three display-only fields on the Color tab:
  - last compression ratio
  - rolling average compression ratio
  - rolling average arrival rate
- Positioned these to the right of the JPEG quality slider.
- Updated displayed precision to one decimal place.
- Compression values are shown like `99.1:1`.
- Arrival rate is shown with one decimal place.

## Tab Behavior Changes

- The Color tab is now moved to the first position in the control panel.
- The Color tab is the default selected tab when the control panel opens.

## Configuration

- Added client config key:

```text
update.rollingStatsFrames
```

- Meaning: number of frames used in the rolling averages.
- Default: `10`

## Verification Performed

- Ran `clang-format` on touched source/header files.
- Built:
  - `rtimvClient`
  - `rtimvServer`

## Current Assumptions

- The reported compression ratio is for the single 2D frame that is JPEG-encoded for transport.
- The calculation uses the source image's native bytes-per-pixel for that frame.
