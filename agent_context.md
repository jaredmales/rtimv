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

12) Doxygen Named Section Ordering
- For classes that expose configuration via member data + accessors, keep named sections split into:
  - `... - Data` for protected/private member state
  - `...` (without `- Data`) for public access functions
- Place the `... - Data` section before the corresponding public accessor section.

13) Base/Client Interface Parity
- Keep `rtimvBase` and `rtimvClientBase` documentation and section structure highly aligned when they implement the same conceptual interface.
- If one side intentionally lacks working-memory members (e.g., server-only processing buffers), add an explicit note documenting why.

14) Header Declaration Parameter Docs
- In headers, prefer inline parameter documentation on declarations (`type name /**< ... */`) rather than separate `\param` lists, unless there is a specific reason to deviate.

15) PR Prompt Attribution
- At the top of PR descriptions, include an explicit attribution line when work was performed with Codex.
- Preferred format:
  - `This work was performed by GPT-5.3-Codex in response to the prompt: "...".`
- Include the primary user prompt verbatim (or a faithful condensed version if it is extremely long).

16) Stats Architecture Rule
- Keep statistics calculations in `rtimvBase`/`rtimvClientBase` rather than GUI classes when the behavior should match between local and gRPC clients.
- GUI classes should provide region coordinates and render values, but not own the calculation pipeline.

17) Display-Only Widget Pattern
- When backend state is authoritative, GUI dialogs/widgets should be display-only and poll/read backend values instead of running duplicate worker threads.
- Prefer a single source of truth for computed values and synchronize via `Image` state where appropriate.

18) Scoped Block Comments
  - For any `{}` block used only to control lock/mutex lifetime, annotate the opening brace as:
    - `{ //mutex scope`

19) Keep This File Current
  - Add new standing style/documentation instructions and coding conventions to `agent_context.md` as they are introduced.

When you finish:
- Summarize what changed.
- List affected files.
- Note any follow-up items or potential edge cases.
