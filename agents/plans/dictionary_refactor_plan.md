# Dictionary refactor plan

## Goal and scope

Extract the dictionary storage types from `rtimvInterfaces.hpp` into
`src/librtimv/rtimvDictionary.hpp` and `src/librtimv/rtimvDictionary.cpp`. This addresses
[issue #37](https://github.com/jaredmales/rtimv/issues/37), which requests a separate home for dictionary support to
improve organization. Preserve the existing types, API, storage behavior, and plugin interface identifiers.

The extraction is implemented on `jrmales/dictionary-refactor`, with the companion MagAOX changes on
`jrmales/rtimv-dictionary-link`. Both branches start from their respective `dev` branches. Ownership,
allocation failure handling, and dictionary synchronization improvements remain separate follow-ups.

## Original implementation

- [`rtimvInterfaces.hpp`](../../src/librtimv/rtimvInterfaces.hpp) defines `rtimvDictBlob`, all its methods inline,
  `dictionaryT`, and `dictionaryIteratorT`. The dictionary is exactly `std::map<std::string, rtimvDictBlob>`.
  The same header contains the Qt plugin interfaces, overlay access types, and unrelated enums.
- Each blob contains a buffer pointer, payload size, capacity, ownership flag, modification timestamp, and mutex.
  `setBlob()`, `getBlob()`, `getBlobSize()`, and the destructor lock the blob. `getBlobStr()` reads through
  `getBlob()`. The map itself has no synchronization wrapper.
- The constructor taking `rtimvDictBlob &` transfers the ownership flag while leaving the source pointing at the
  same storage. It does not copy the timestamp or lock the source. Construction from a const reference is deleted.
- [`rtimvMainWindow`](../../src/gui/rtimvMainWindow.hpp) owns `m_dictionary`. Its
  [`implementation`](../../src/gui/rtimvMainWindow.cpp) passes that map to dictionary and overlay plugins and
  reads/writes `rtimv.north.angle` through the blob API.
- [`CMakeLists.txt`](../../CMakeLists.txt) compiles library sources into `objlib`, then uses those objects in
  `rtimv-shared` and `rtimv-static`. Public headers are installed explicitly under `include/rtimv`.
  The viewer, client, and server already link `rtimv-shared`.
- [`PluginGuide.md`](../../doc/PluginGuide.md) documents these public types and their usage. Existing tests have
  no dedicated dictionary coverage.

## File boundaries

| File | Responsibility after extraction |
| --- | --- |
| `src/librtimv/rtimvDictionary.hpp` | Complete `rtimvDictBlob` declaration, member state, method declarations, and the two existing dictionary typedefs. |
| `src/librtimv/rtimvDictionary.cpp` | Every non-deleted blob method definition, including constructors, destructor, `clearBlob()`, and `getBlobSize()`. |
| `src/librtimv/rtimvInterfaces.hpp` | Plugin and overlay contracts; includes `rtimvDictionary.hpp` so existing consumers retain access to the dictionary types. |

Keep `rtimvDictionaryInterface`, its IID macro, and `Q_DECLARE_INTERFACE` in `rtimvInterfaces.hpp`. They depend on
the common Qt plugin base declared there. Moving that interface into the storage header would introduce Qt and
mxlib dependencies or require a wider split of the plugin interfaces. `rtimvOverlayAccess` also stays in place.

The new dictionary header must be self-contained and usable without including Qt, mxlib, graphics classes, or
`rtimvInterfaces.hpp`. Include the standard/POSIX declarations it uses directly, including `<map>`, `<string>`,
`<mutex>`, `size_t`, and `timespec`; the current map declaration relies on transitive includes. Put allocation,
byte-copy, and clock implementation dependencies in the `.cpp` where possible. Include the matching header first
in the `.cpp`, followed by its implementation dependencies, following neighboring library sources.

This separates header dependencies; it does not create a new standalone library or remove Qt from the existing
project configuration and `librtimv` build.

## API and behavior preservation

Retain the global names `rtimvDictBlob`, `dictionaryT`, and `dictionaryIteratorT`. Keep the typedefs as the same
map and mutable iterator types. Do not introduce a namespace migration, map wrapper, PIMPL, virtual functions,
new overloads, or new copy/move operations. Preserve the member order, types, initializers, access levels, and
existing public/protected signatures.

Move the following bodies to the `.cpp` without changing their logic:

| Method | Contract or behavior to retain |
| --- | --- |
| Default constructor | Empty state with a null buffer, zero sizes and timestamp, and no ownership. |
| Constructor taking `rtimvDictBlob &` | Existing ownership transfer and source aliasing, including the destination's initially zero timestamp. |
| `clearBlob()` | Frees and resets storage only when the object owns a non-null buffer. |
| `setBlob()` | Reuses owned capacity, reallocates when needed or when not the owner, copies supplied bytes, and updates `CLOCK_REALTIME` timestamp. |
| `getBlob()` | Copies at most the requested capacity, returns the copied size, and optionally returns the timestamp from the same locked read. |
| `getBlobStr()` | Uses `getBlob()`, then terminates the returned bytes; preserves truncation and empty-buffer behavior. |
| `getBlobSize()` | Locks internally and returns zero if the buffer pointer is null. |
| Destructor | Locks and releases owned storage through `clearBlob()`. |

Leave `rtimvDictBlob(const rtimvDictBlob &)` deleted in the header. Keep optional `timespec *` default arguments
only on the declarations. Even the empty constructor and simple size accessor belong in the `.cpp` under the
repository's placement rules.

The string helper's details need precise documentation: a nonempty read replaces the last copied byte with
`'\0'`; an empty read writes a terminator and returns `1` only when destination capacity is positive. A
zero-capacity call returns `0` and writes nothing. Preserve these results rather than introducing a different
string-copy convention during extraction.

Keep the current `clearBlob()` name to avoid changing the protected API. Document that callers must hold the
blob mutex when the object can be accessed concurrently; construction is the existing unshared exception.
Retain the existing internally locking public API. Any future `mtxL_*`/`mtxUL_*` additions must follow
`AGENTS.md`, but no new locking API is necessary for this file split.

Document that per-blob locking does not protect map insertion, lookup during structural mutation, erasure, or
object lifetime. Callers must coordinate map changes and keep entries alive while they are in use. A separate
`getBlobSize()` followed by `getBlob()` is not a single snapshot; readers must use the actual returned byte count.

## Source and library compatibility

Continue including the new header from `rtimvInterfaces.hpp`. Existing plugins can retain their current include
and use `dictionaryT *` in `attachDictionary()` and `rtimvOverlayAccess` without source changes. Consumers that
only need dictionary storage can include `<rtimv/rtimvDictionary.hpp>` from the installed tree.

Keep `rtimv.dictionaryInterface/1.4` and all other plugin IIDs unchanged. There is no intentional change to class
layout or plugin virtual interfaces. Moving definitions out of a header does, however, change linkage:

- New or rebuilt consumers must link the matching `librtimv.so` or `librtimv.a`; a consumer that previously used
  only the inline blob implementation must add that dependency.
- New headers paired with an old library will lack the required out-of-line symbols. Install the header and
  library together and rebuild dependent plugins for deployment.
- Preserving layout and IIDs is not a blanket guarantee for existing plugin binaries. If supporting plugins
  built with the old inline definitions is required, exercise that combination explicitly with the supported
  compiler and standard library before claiming binary compatibility.

No functional edits should be needed in `rtimvMainWindow.hpp/.cpp`, the backend classes, or `rtimv.proto`.
The main window still owns the dictionary and exposes the same pointers to plugins. This refactor introduces
no transport state, RPC, worker thread, or new ownership boundary.

## MagAOX plugin build changes

The sibling checkout stores these plugins under `../MagAOX/gui/rtimv/plugins` (singular `gui`). All six include
`<rtimv/rtimvInterfaces.hpp>` and use dictionary blob methods, so the compatibility include preserves their
source usage. Two qmake projects now declare the additional library dependency for the out-of-line methods:

| Plugin | Explicit `-lrtimv` before extraction | Implemented build-file change |
| --- | --- | --- |
| `indiDictionary` | Absent | Added `LIBS += -lrtimv` to `indiDictionary/indiDictionary.pro`, alongside its existing INDI library entries. |
| `pwfsAlignment` | Absent | Added `LIBS += -lrtimv` to `pwfsAlignment/pwfsAlignment.pro`, alongside `SOURCES`/`TARGET`. |
| `acquisition` | Present | None. |
| `cameraStatus` | Present | None. |
| `dmStatus` | Present | None. |
| `warnings` | Present | None. |

`indiDictionary` publishes string and north-angle payloads through `setBlob()`. `pwfsAlignment` initializes keys
through `setBlob()` and reads values through `getBlobStr()`. Before the changes, their generated makefiles also
omitted `-lrtimv`, and their existing plugin binaries had no direct `DT_NEEDED` entry for `librtimv.so`. The other
four generated makefiles and existing binaries already contained the dependency. All six rebuilt plugins now
declare it and reference the extracted blob methods.

`../MagAOX/Make/rtimvPlugin.mk` invokes qmake for each plugin and then its generated makefile; it supplies no
additional library flags. No shared build-rule change is needed. Update the two `.pro` files and let the normal
build regenerate `makefile.<plugin>` rather than editing generated files. Do not compile the dictionary source
into the plugins. An unresolved symbol may be satisfied by the viewer's already-loaded library on Linux, but
that should not substitute for declaring each plugin's direct dependency.

Install the refactored rtimv headers and library together, then cleanly rebuild all six plugins and restart the
viewer/client before checking plugin loading. The validation below used staged installation files and isolated
plugin build directories; it did not replace the system installation or the existing MagAOX build artifacts.

## Build and documentation changes

1. Add `src/librtimv/rtimvDictionary.cpp` to `objlib` so both shared and static libraries contain the definitions.
   Do not compile another copy into the viewer, client, server, or each test executable.
2. Add `src/librtimv/rtimvDictionary.hpp` to the existing public-header installation list. Preserve target names
   and installation destinations.
3. Replace the blob and typedef definitions in `rtimvInterfaces.hpp` with an include of the new header, and remove
   the completed dictionary-file TODO. Audit includes after the move; retain dependencies used by the remaining
   plugin declarations and avoid unrelated include cleanup.
4. Update the dictionary section of `doc/PluginGuide.md` with the dedicated header, the continued umbrella include,
   library-link requirement, and synchronization/lifetime limits. Keep its existing usage examples valid.
5. Apply `AGENTS.md` documentation rules to the new files and touched declarations: `\file` and `\brief` blocks,
   no `\author` in touched source/header files, `///` summaries for every function declaration and non-trivial
   member, inline parameter documentation, and return-value documentation. Preserve declaration grouping and
   put multiline closing parentheses on their own line. Annotate any added lock-lifetime-only scope with
   `{ // mutex scope`.

Include-guard style varies within the repository (`*_hpp`, `*_hpp_inc`, and prefixed forms). Use
`rtimvDictionary_hpp`, matching the adjacent `rtimvInterfaces.hpp` and `rtimvCommandInterface.hpp`; leave existing
guards unchanged. Preserve local include ordering because `.clang-format` disables include sorting. These are
local choices for the extraction, not new repository-wide conventions, so no `AGENTS.md` change is required.

## Implementation sequence and validation

1. Add focused characterization coverage for valid blob operations before moving their definitions. Use the
   existing small executable/CTest approach in `tests/librtimv/rtimvDictionary_test.cpp`, with checks that remain
   active in release builds. Keep the test registration before the gRPC availability guard in
   `tests/CMakeLists.txt`.
2. Extract declarations and definitions, add the compatibility include, and update the library source and
   installation lists in one buildable change.
3. Add the two explicit MagAOX plugin link dependencies identified above as companion build changes.
4. Run the same dictionary tests against both `rtimv-shared` and `rtimv-static`. Tests must link the library
   rather than compile `rtimvDictionary.cpp` directly, so missing packaged definitions cause a failure.
5. Update the plugin guide, run `clang-format` on touched C++ files, and run `git diff --check`. Review the moved
   bodies against the originals to confirm that only placement and documentation changed.
6. Build the normal viewer and both library variants; build the client and server when gRPC is available. Run
   CTest, including existing plugin/GUI coverage, to catch integration regressions.
7. Stage an installation in a temporary directory. Compile and run a small consumer against the staged header
   and shared library, and another against the static library. Include the dictionary header first, without
   Qt/mxlib include paths, to check header independence. Separately compile a consumer of the installed
   `rtimvInterfaces.hpp` with its normal dependencies to check the compatibility include. Exercise more than one
   translation unit to catch missing or duplicate definitions.
8. Load rebuilt MagAOX dictionary/overlay plugins in the viewer and, when available, the client. Check dictionary
   attachment, a plugin-published value, and the `rtimv.north.angle` display path. External plugin validation
   requires a suitable plugin build; record any unavailable integration check rather than implying it passed.

The focused dictionary coverage should include:

- Default construction, empty reads, and zero initial size/timestamp.
- Binary round trips with embedded zero bytes; shorter, equal-size, and larger writes; truncated reads; and
  payload sizes returned after each write.
- Optional timestamps returned with reads, without assuming that `CLOCK_REALTIME` is monotonic or that two
  rapid writes have different timestamps.
- Null-terminated strings, truncated strings, destination capacity one, empty strings, and zero destination
  capacity using valid destination buffers.
- A zero-length write to an owned blob followed by another nonempty write.
- Construction from a non-const source, reading the destination, and destruction in safe orders. Keep the new
  owner alive while inspecting the source alias; do not turn use-after-free behavior into a supported contract.
  Confirm that const copying remains unavailable and the destination timestamp retains its existing behavior.
- Dictionary insertion, lookup, iteration through `dictionaryIteratorT`, and destruction through the linked API.

Acceptance requires a single definition location for the dictionary types, no executable blob bodies remaining
in `rtimvInterfaces.hpp`, successful direct and umbrella-header consumption, both packaged library variants
providing the methods, and unchanged characterized results. The dictionary header must not pull in the GUI
interfaces.

## Affected files

- Added `src/librtimv/rtimvDictionary.hpp` and `src/librtimv/rtimvDictionary.cpp`.
- Updated `src/librtimv/rtimvInterfaces.hpp` and root `CMakeLists.txt`.
- Added `tests/librtimv/rtimvDictionary_test.cpp` and updated `tests/CMakeLists.txt`.
- Updated `doc/PluginGuide.md`.
- Companion MagAOX changes: updated `../MagAOX/gui/rtimv/plugins/indiDictionary/indiDictionary.pro` and
  `../MagAOX/gui/rtimv/plugins/pwfsAlignment/pwfsAlignment.pro` to add `LIBS += -lrtimv`.
- Keep this plan in `agents/plans/dictionary_refactor_plan.md` current as implementation decisions change.

## Validation results

Validation completed on 2026-10-19 with GCC 15 and Qt 6:

- The new characterization test passed against the original inline implementation before extraction, with
  optimization and `NDEBUG` enabled. Both shared and static CTest variants passed after extraction.
- All eight extracted method bodies match the originals apart from whitespace. The public type names, member
  layout declarations, signatures, typedefs, and plugin IIDs are preserved.
- The full CMake build passed for the libraries, viewer, server, client, and tests. The dictionary test targets
  explicitly link Qt Widgets, matching existing consumers of the shared library's GUI components.
- All 13 CTest tests passed across the validation runs. The sandbox initially blocked four gRPC tests from
  opening loopback sockets. With loopback access, three passed; `rtimvGlobalStats_clientStartupGui` encountered
  a gRPC `ref_counted.h` assertion in the concurrent run and then passed in an isolated run. This intermittent
  startup failure remains a follow-up; no transport implementation was changed as part of this extraction.
- CMake installed the headers and both libraries into an isolated staging directory. Two-translation-unit
  consumers compiled the installed dictionary header without Qt/mxlib include flags and passed against both
  library variants. The shared consumer supplied the existing Qt link dependencies; the static consumer did
  not need them. Installation used the `Unspecified` component to avoid an existing unwritable default manifest.
- All six MagAOX plugins rebuilt using their qmake projects against the staged headers and library, validating
  the installed umbrella include. ELF inspection confirmed all six declare `librtimv.so` and reference the new
  external blob symbols.
- Integration harnesses using the production main window with local and gRPC backends loaded and attached all
  six rebuilt plugins. A synthetic INDI service bound to `127.0.0.1` supplied a north angle and warning state.
  The checks verified INDI publication, overlay reads, warning text, north-arrow rotation, and a subsequent
  `northAngleRaw()` write. The remote backend used a local server and a generated FITS image.
- `clang-format` and whitespace checks passed. No application, backend, or protocol implementation changes
  were required. Validation artifacts and logs are under the ignored `_build/dictionary-validation` directory.

## Separate follow-ups and edge cases

The ownership-transferring constructor leaves aliases with independent mutexes and can leave the source with a
dangling pointer after the new owner frees or reallocates storage. Fixing this requires an explicit copy/move and
lifetime design. The extraction should document the limitation without silently replacing that constructor.

Allocation failures are not checked today. `setBlob(nullptr, positive_size)` does not initialize newly allocated
bytes, and a zero-length write on a non-owner can retain the old pointer and size because `clearBlob()` only
resets owned storage. Treat improved failure handling, null-input rules, and non-owner reset semantics as
separate behavioral changes with their own tests.

A synchronized map wrapper, typed values, timestamp redesign, namespace cleanup, and further separation of the
Qt plugin interfaces are also outside issue #37's organizational scope. Keeping these separate makes it possible
to review and validate the file extraction on its own.
