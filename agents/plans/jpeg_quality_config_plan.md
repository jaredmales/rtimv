# JPEG Quality Configuration Plan

## Goal

Make JPEG quality a configurable startup parameter for the gRPC client/server path with these behaviors:

- The setting is loaded from config files on both the client and server sides.
- `rtimvServer` can define a default JPEG quality applied to all served images.
- The client can override the configured value from the command line.
- Per-image startup configuration overrides the server default.
- The value is scoped per configured remote image stream, not as one global server setting.

## Current State

- Transport quality already exists as runtime state in [`src/server/rtimvServerThread.hpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServerThread.hpp) and [`src/server/rtimvServerThread.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServerThread.cpp):
  - `m_quality` defaults to `50`.
  - JPEG encoding uses that value in `mtxuL_render()`.
  - `SetQuality` updates it at runtime.
  - `ImagePlease` returns it to the client in the `Image` payload.
- The client already tracks and displays the active server quality in [`src/server/rtimvClientBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvClientBase.cpp) and the control panel.
- There is no config-file or CLI startup path for quality in:
  - [`src/server/rtimvClientBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvClientBase.cpp)
  - [`src/common/rtimvBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/common/rtimvBase.cpp)
  - [`src/server/rtimvServer.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServer.cpp)
- The `remote_rtimv::Config` message in [`src/proto/rtimv.proto`](/home/jrmales/Source/rtimv/rtimv/src/proto/rtimv.proto) does not currently carry a quality field for startup configuration.
- `rtimvServer` currently has no server-level default-quality setting for newly configured image threads.

## Interpretation Of "Per Image"

The existing transport architecture creates one `rtimvServerThread` per configured remote image stream/client configuration. That thread renders one JPEG payload for the displayed image state.

Because only the rendered transport image is JPEG-encoded, the clean interpretation is:

- "Per image" means per configured remote image stream / per `Configure()` session.
- It does not need separate JPEG quality values for dark, mask, flat, and sat-mask inputs, since those are not individually transported as JPEGs.

If you intended per auxiliary-image quality fields anyway, that would be a larger schema change and does not match the current transport model.

## Proposed Design

### Precedence

Use this startup precedence, highest to lowest:

1. Client command-line `quality`
2. Client config-file `quality`
3. `rtimvServer` default `quality`
4. Built-in default `50`

This preserves the current client-driven per-image model while adding a server-side fallback for every newly served image.

### 1. Add a startup config field to the gRPC `Config` message

Update [`src/proto/rtimv.proto`](/home/jrmales/Source/rtimv/rtimv/src/proto/rtimv.proto) so `Config` carries:

- `bool quality_set`
- `int32 quality`

This matches the existing `*_set` pattern already used for optional startup overrides like `update_timeout`, `autoscale`, and `mzmq_port`.

Why:

- The client needs to tell the server whether a quality value was explicitly configured.
- The server needs to preserve its default when the field is absent.

### 2. Add `quality` to client config and CLI parsing

In [`src/server/rtimvClientBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvClientBase.cpp):

- Add a config entry for `quality` in `setupConfig()`.
- In `loadConfig()`, if `quality` is set:
  - read it from the config system,
  - clamp or validate it to `[0,100]`,
  - store it into `m_configReq`,
  - set `quality_set = true`.

Behavior:

- Client config file can define the default quality for that remote image configuration.
- Client CLI can override the config file automatically via the existing `mx::app::application` precedence rules already used elsewhere.

### 3. Pass configured quality through server `Configure()`

In [`src/server/rtimvServer.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServer.cpp):

- Add a documented server member, likely `m_qualityDefault`, defaulting to `50`.
- Add a `quality` config entry in `setupConfig()` for `rtimvServer`.
- Read that value in `loadConfig()`.
- In `doConfigure()`, always seed the spawned argv with the server default:
  - `--quality`
  - `<m_qualityDefault>`
- If `cspec->m_config.quality_set()` is true, append a second:
  - `--quality`
  - `<client/image-specific value>`

to the argv vector used to configure the new [`src/server/rtimvServerThread.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServerThread.cpp) instance.

Why this fits the current structure:

- Server-side per-client image threads already bootstrap from argv plus an optional config file.
- Forwarding both values through argv keeps startup behavior aligned with how other image-specific startup options are applied.
- Because later command-line occurrences should win, the image-specific override can cleanly supersede the server default without introducing a second override path inside `rtimvServerThread`.

### 4. Add quality to `rtimvBase` startup configuration

In [`src/common/rtimvBase.hpp`](/home/jrmales/Source/rtimv/rtimv/src/common/rtimvBase.hpp) and [`src/common/rtimvBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/common/rtimvBase.cpp):

- Add a stored startup quality member in the base class, likely defaulting to `50`.
- Add `quality` to `setupConfig()`.
- Read `quality` in `loadConfig()`.
- Expose a non-inline accessor if needed, following the project rule to keep even simple accessors in `.cpp`.

Why `rtimvBase` is the right place:

- `rtimvServerThread` derives from `rtimvBase`, and its per-image startup behavior is already driven by `rtimvBase` config parsing.
- This keeps startup configuration centralized instead of duplicating parsing logic in the server thread.

### 5. Seed `rtimvServerThread` runtime quality from configured base state

In [`src/server/rtimvServerThread.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServerThread.cpp):

- After `setup()`/`loadConfig()` completes during `configure()`, initialize `m_quality` from the configured startup quality value.
- Reuse the existing `quality( int )` setter so clamping and `m_newImage` behavior stay consistent.

Expected result:

- The first JPEG served after configuration uses the configured quality.
- Runtime `SetQuality` continues to work exactly as it does now.

### 6. Keep passive state sync via `ImagePlease`

No architectural change is needed here.

- [`src/server/rtimvServer.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServer.cpp) already includes `quality` in the `Image` reply.
- [`src/server/rtimvClientBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvClientBase.cpp) already refreshes local cached quality from received `Image` state.

This already matches the standing rule in `AGENTS.md` that `Image` should remain the authoritative sync payload for frequently changing backend state.

## File-Level Implementation Plan

### Proto and generated interfaces

- Update [`src/proto/rtimv.proto`](/home/jrmales/Source/rtimv/rtimv/src/proto/rtimv.proto) `Config`.
- Regenerate protobuf/gRPC sources through the project build.

### Common base config

- Update [`src/common/rtimvBase.hpp`](/home/jrmales/Source/rtimv/rtimv/src/common/rtimvBase.hpp).
- Update [`src/common/rtimvBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/common/rtimvBase.cpp).

### Client startup config path

- Update [`src/server/rtimvClientBase.hpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvClientBase.hpp) only if a documented member is needed for startup/default quality state.
- Update [`src/server/rtimvClientBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvClientBase.cpp).

### Server configure/bootstrap path

- Update [`src/server/rtimvServer.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServer.cpp).
- Update [`src/server/rtimvServer.hpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServer.hpp).
- Update [`src/server/rtimvServerThread.hpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServerThread.hpp) only if an accessor or documented startup helper is required.
- Update [`src/server/rtimvServerThread.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvServerThread.cpp).

## Validation Plan

### Functional checks

- Start `rtimvServer` with a config file containing `quality`, connect a client with no image-specific quality, and verify the first returned `Image.quality` matches the server default.
- Start `rtimvClient` with a config file containing `quality`, connect to the server, and verify the configured value is sent during `Configure()`.
- Start `rtimvClient` with both config-file `quality` and CLI `--quality`; verify CLI wins.
- Start `rtimvServer` with one default quality and `rtimvClient` with a different per-image quality; verify the per-image value wins.
- Change quality at runtime from the control panel and verify it still propagates through `SetQuality` and `ImagePlease`.
- Verify out-of-range values are clamped or rejected consistently on both startup and runtime paths.

### Regression checks

- Confirm the existing effective default remains `50` when neither server nor client config supplies `quality`.
- Confirm reconnect/configure still works when `quality` is absent.
- Confirm the control panel still reflects server-authoritative quality after startup and after runtime changes.

### Build checks

- Build `rtimvServer`.
- Build `rtimvClient`.
- Run `clang-format -i` on touched C++/proto-adjacent source files.

## Risks And Edge Cases

- Proto change requires regenerated sources; any stale generated files will cause confusing build failures.
- Startup quality should be applied before the first rendered response, otherwise the first frame may still go out at the default `50`.
- Validation policy should be chosen once and applied consistently:
  - clamp to `[0,100]`, or
  - reject invalid startup values with a configuration error.

Clamping is more consistent with the existing runtime `rtimvServerThread::quality( int )` behavior and is likely the least disruptive choice.

## One Existing Ambiguity Worth Resolving During Implementation

While reviewing `rtimvClientBase::loadConfig()`, I noticed [`src/server/rtimvClientBase.cpp`](/home/jrmales/Source/rtimv/rtimv/src/server/rtimvClientBase.cpp) currently does:

- `flatKey` -> `m_configReq->set_mask_key( flatKey )`

That looks unrelated to JPEG quality, but it may be an existing bug or stale path in the config packing logic. I would leave it untouched for a minimal quality-only patch unless you want that cleaned up in the same change.
