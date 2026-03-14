# rtimv Logging Prefix + Formatter Implementation Plan

## Goal
Standardize all log/status messages to:

`called-name (client: client-id image: image0-name): message`

with these rules:
- `called-name` is `argv[0]` (or equivalent executable name).
- `called-name` output is explicitly configurable (`log.appname` / `--no-log-appname`), with default `on`.
- `client: client-id` appears only in `rtimvClient` and `rtimvServer` outputs.
- `image: image0-name` is the primary image name/key.
- Message body follows the prefix consistently for both normal status and errors.

## Scope
- `src/common`: common formatter and helpers.
- `src/server`: server/client adoption.
- `src/proto/rtimv.proto`: add `client_id` to `ConfigResult`.
- Build/proto generated outputs as needed.

## Proposed Design

### 1. Add common formatter utility
Create a shared utility in `src/common` (e.g. `rtimvLog.hpp/.cpp`) providing:
- `struct logContext`
  - `std::string calledName`
  - `std::string image0`
  - `std::string clientId` (optional)
  - `bool includeClient` (default false)
- `std::string formatPrefix(const logContext&)`
- `std::string formatMessage(const logContext&, std::string_view message)`

Behavior:
- Base (default): `<called-name> (`
- If `log.appname=false`: `(`
- Include `client: ... ` only if `includeClient == true` and `clientId` non-empty.
- Always include `image: ...` (fallback `unknown` if empty).
- Close with `): ` and append message.

Example outputs:
- `rtimvServer (client: ipv4:10.1.2.3:50482 image: cam0): configured`
- `rtimvClient (client: ipv4:10.1.2.3:50482 image: cam0): connected`
- `rtimv (image: cam0): loaded plugin foo`
- `(client: ipv4:10.1.2.3:50482 image: cam0): configured` (with `--no-log-appname`)

### 2. Normalize called-name extraction
Implement helper in app base/utility layer:
- Prefer configured argv/program name (already available through mx app setup paths).
- Fallback: executable basename from argv[0].
- Apply explicit enable/disable:
  - Config key: `log.appname` (bool, default `true`)
  - CLI convenience flag: `--no-log-appname` (sets `log.appname=false`)
- Do not auto-detect systemd by default (keep behavior explicit and unit-file controlled).

Ensure this is available in:
- `rtimvBase`
- `rtimvClientBase`
- `rtimvServer`

### 3. Add `client_id` to proto
Update `src/proto/rtimv.proto`:
- Add field in `ConfigResult`, e.g.:
  - `string client_id = <next available tag>;`

Server behavior:
- In `Configure`, set `client_id` to the canonical server-side client id (currently `context->peer()`).

Client behavior:
- Store returned `client_id` in `rtimvClientBase` member (new field `m_clientId`).
- Use that id in all client log prefixes.

Compatibility notes:
- Adding a new optional scalar field is wire-compatible in protobuf.
- Old clients ignore unknown `client_id`.
- New clients should tolerate empty `client_id` if connected to old servers.

### 4. Server-side logging adoption
In `src/server/rtimvServer.cpp`:
- Replace ad-hoc `std::cerr`/`std::cout` strings with formatter-backed helpers:
  - Connection/configure messages
  - Reconfigure attempts
  - Thread sleep/disconnect notices
  - Error branches (`null thread`, `missing thread`, terminate path)

Context mapping:
- `called-name`: server program name.
- `client`: peer/client id.
- `image`: `image0OrUnknown(imageTh)` or configured `image_key` fallback.

### 5. Client-side logging adoption
In `src/server/rtimvClientBase.cpp`:
- Route existing status/error outputs through formatter:
  - Configure success/failure
  - Server disconnection/deadline messages
  - Reconnect transitions
  - Key async RPC failure logs

Context mapping:
- `called-name`: client program name.
- `client`: `m_clientId` (from `ConfigResult.client_id`; fallback empty before configure).
- `image`: local image0 key/name currently configured.

### 6. Common GUI/non-client/server logging adoption (optional first pass)
For `rtimv` GUI and common modules:
- Use same formatter with `includeClient=false`.
- Keep scope limited initially to high-value log points to reduce patch size.

## Implementation Steps (Order)
1. Add formatter utility files and unit-test-style coverage (if test infra available).
2. Add explicit app-name controls (`log.appname`, `--no-log-appname`) in config parsing.
3. Add `client_id` field to proto and regenerate gRPC/protobuf outputs.
4. Update server `Configure` to populate `ConfigResult.client_id`.
5. Update client `Configure` handling to persist `client_id`.
6. Migrate server logs to formatter helper.
7. Migrate client logs to formatter helper.
8. Optional: migrate selected GUI/common logs.
9. Build + smoke tests with mixed-version client/server.

## Validation Plan

### Functional checks
- Start server/client normally; verify prefix format on connect/disconnect.
- Start server with `--no-log-appname`; verify prefix omits app name cleanly.
- Missing image path: verify `image: unknown` or configured key fallback.
- High-load multi-client run: verify every server/client log has consistent prefix.
- Confirm client receives and uses `client_id` from `ConfigResult`.

### Compatibility checks
- New server + old client: no breakage.
- Old server + new client: client handles missing `client_id` gracefully.

### Regression checks
- Build targets:
  - `rtimvServer`
  - `rtimvClient`
  - `rtimv`
- Existing load scenario still stable with updated log paths.

## Suggested Follow-on (after baseline)
- Add lightweight log level enum (`ERROR/WARN/INFO/DEBUG`) to formatter output.
- Add runtime switch for terse vs verbose formatting.
- Add compile-time option to disable expensive context fetches in hot paths.
- Optional (future, not baseline): add systemd environment auto-detect (`INVOCATION_ID`) only as an override layer if explicitly requested.

## Estimated Change Set
- Proto: 1 file (`src/proto/rtimv.proto`)
- Shared formatter: 2 new files (`src/common/rtimvLog.hpp/.cpp`)
- Client/server adoption: 3-5 existing files
- Generated protobuf/grpc files: build-generated artifacts

## Review Notes
- Keep first patch focused on infrastructure + server/client paths only.
- Avoid refactoring unrelated logging in same commit.
- Preserve existing message semantics while normalizing prefix.
