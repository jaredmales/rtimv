Follow these code style and documentation rules exactly.

1) File-Level Documentation
- Each header/source should have a top Doxygen file block:
  - \file
  - \brief
  - \author (if project uses it)

2) Include Guards and Includes
- Match existing project include-guard naming convention.
- Keep include ordering consistent with project style.
- Do not introduce new include style unless project already uses it.

3) Function Declaration Documentation
- Every function declaration must have a brief `///` summary.
- Add a short `/** ... */` details block only when needed.
- Document every parameter inline at declaration site using:
  - `type name /**< [in] description */`
- Apply to normal methods, constructors, slots, and signals.
- Keep return-value docs where project uses them.

4) Member Variable Documentation
- Document non-trivial class members with `///`.
- Describe role/ownership/state, not just type.

5) Naming and Structure
- Use project member naming convention (e.g. `m_` prefix).
- Keep declaration ordering/grouping stable:
  - public/protected/private
  - slots/signals grouped consistently.
- Leave a blank line between declarations for readability.

6) Mutex/Locking API Convention
- `mtxL_*` functions require lock already held by caller.
- `mtxUL_*` functions acquire lock internally then call `mtxL_*`.
- Prefer calling `mtxUL_*` from UI/controller code.
- Avoid passing mutex pointers through UI classes unless unavoidable.

7) Header vs Source Placement
- Keep non-trivial definitions out of headers.
- Move implementations to `.cpp` unless intentionally inline.

8) Editing Discipline
- Preserve existing behavior unless explicitly requested.
- When renaming members/APIs, update all dependent call sites.
- Keep changes minimal and scoped.

9) Formatting and Verification
- Run `clang-format` on touched files.
- Ensure docs and naming are consistent after formatting.
- Report any places where project style is ambiguous before making assumptions.

10) gRPC Reactor Conventions
- For unary reactors, use `static_cast<void>(...)` for intentionally unused parameters.
- For image-dependent modification RPCs, guard with `if( !imageTh->connected() )` and return `Status::OK` early.
- Keep image modification RPC declarations grouped after `ImagePlease` in `rtimv.proto` and corresponding server/client files.

11) Client/Server State Synchronization
- Treat `Image` as the authoritative state sync payload for frequently changing backend state.
- When practical, include configuration/state fields in `Image` so clients can refresh passively on each `ImagePlease` response.
- Prefer updating local cached client state from received `Image` fields instead of immediately mirroring local writes after set-RPC calls.

When you finish:
- Summarize what changed.
- List affected files.
- Note any follow-up items or potential edge cases.
