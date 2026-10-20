# Plugin System Guide

This document is for programmers writing `rtimv` plugins. It describes how plugins are discovered, which interfaces are available, how the data dictionary works, and how to add plugin-specific help overlays.

## Overview

`rtimv` plugins are Qt plugins built around the interfaces declared in [rtimvInterfaces.hpp](../src/librtimv/rtimvInterfaces.hpp).

There are two primary plugin roles:

- `rtimvDictionaryInterface`
  Exposes a shared key/value dictionary to a plugin so it can publish or consume auxiliary state.
- `rtimvOverlayInterface`
  Gives a plugin access to selected GUI objects so it can draw overlays, react to keypresses, and update display state.

A single plugin class may implement one or both interfaces.

All plugin interfaces derive from `rtimvInterface`, which provides:

- `info()` for user-visible plugin information shown in the `i` info overlay
- standardized plugin log formatting helpers
- optional plugin text-overlay help hooks

As of interface version `1.4`, `rtimvInterface` also provides optional default methods for plugin text overlays:

- `hasTextOverlay()`
- `textOverlayKey()`
- `textOverlayTitle()`
- `textOverlayText()`

These default to the no-overlay case, so existing plugins do not need to implement them unless they want a dedicated help/info panel.

## Plugin Discovery and Loading

`rtimvMainWindow` loads plugins in three ways:

- static Qt plugins returned by `QPluginLoader::staticInstances()`
- dynamic plugins found in directories listed in `RTIMV_PLUGIN_PATH`
- dynamic plugins found in the application `plugins` directory next to the installed executable

Each candidate plugin object is passed through `rtimvMainWindow::loadPlugin()`. That function:

- detects which supported interfaces the plugin implements
- sets plugin log context with `setLogContext()`
- sets a plugin label with `setPluginName()`
- calls `attachDictionary()` when the plugin implements `rtimvDictionaryInterface`
- calls `attachOverlay()` when the plugin implements `rtimvOverlayInterface`
- registers optional plugin text overlays when `hasTextOverlay()` returns `true`

Plugin attachment return values follow the established convention:

- `0`: configured and attached successfully
- `> 0`: not configured or intentionally unused, but not an error
- `< 0`: hard error

If a dynamically loaded plugin is unused, `rtimv` attempts to unload it.

## Base Interface

Every supported plugin derives from `rtimvInterface`.

The required method is:

```cpp
virtual std::vector<std::string> info() = 0;
```

This should return programmer- or operator-facing summary lines for the `i` info overlay. The first line should identify the plugin and its type. Later lines should align visually with that first line.

Useful inherited helpers include:

- `setLogContext()`
- `setPluginName()`
- `formatPluginLogMessage()`
- `pluginLogInfo()`
- `pluginLogError()`

These keep plugin log output consistent with the rest of `rtimv`.

## Dictionary Plugins

Dictionary plugins implement `rtimvDictionaryInterface`:

```cpp
virtual int attachDictionary( dictionaryT *, mx::app::appConfigurator & ) = 0;
```

The dictionary itself is a shared `std::map<std::string, rtimvDictBlob>`, typedef’d as `dictionaryT`.

The storage types are declared in [rtimvDictionary.hpp](../src/librtimv/rtimvDictionary.hpp). Plugins can continue
including `<rtimv/rtimvInterfaces.hpp>`, which includes that header and declares the plugin interfaces. Code that
only needs the storage types can include `<rtimv/rtimvDictionary.hpp>` without Qt or mxlib headers.

Blob methods are implemented in `librtimv`; consumers must link the library, for example with `LIBS += -lrtimv`
in a qmake project. Install matching headers and libraries before rebuilding plugins. New headers cannot be
paired with an older library that lacks the blob method definitions. The dictionary and overlay interface IIDs
remain unchanged, but existing plugin binaries should be validated separately if they must be retained.
The existing shared library includes Qt components, so dictionary-only consumers linking it must still supply
its Qt link dependencies.

The intent is that plugins can exchange lightweight state through stable string keys without introducing direct coupling between unrelated plugin classes and GUI code.

### Blob Storage

Each dictionary entry is an `rtimvDictBlob`, which stores:

- a byte buffer
- the current payload size
- the allocated size
- ownership state
- a `CLOCK_REALTIME` modification timestamp
- an internal mutex protecting access

The main accessors are:

- `setBlob(const void *blob, size_t sz)`
- `getBlob(char *blob, size_t mxsz, timespec *ts = nullptr)`
- `getBlobStr(char *blob, size_t mxsz, timespec *ts = nullptr)`
- `getBlobSize()`

These accessors synchronize each blob independently. They do not protect the dictionary's structure or an
entry's lifetime: coordinate insertion and erasure with other map users, and keep entries alive while accessing
them. A size query followed by a read is not one snapshot; use the read's returned byte count. Request the
optional timestamp with the read to obtain the timestamp corresponding to those bytes. `CLOCK_REALTIME` is not
a monotonic clock.

`getBlobStr()` replaces the last copied byte with a null terminator. A return equal to the destination capacity
can indicate truncation. An empty payload produces one null byte when capacity is positive; zero capacity
returns zero without writing.

The legacy constructor taking `rtimvDictBlob &` transfers buffer ownership but leaves the source aliasing the
same storage with a separate mutex. It does not transfer the timestamp or synchronize the source. Avoid moving
entries this way while they are shared, and do not access an alias after the owner frees or reallocates its
storage. This constructor is not a deep copy; construction from a const reference remains deleted.

### Dictionary Usage Pattern

Writers typically:

1. choose a stable namespaced key such as `rtimv.pluginName.state`
2. fill a POD struct or C string with the desired content
3. call `setBlob()` on the target dictionary entry

Readers typically:

1. look up the key in `dictionaryT`
2. allocate a destination buffer of suitable size
3. call `getBlob()` or `getBlobStr()`
4. optionally compare or inspect the returned timestamp

### Recommended Dictionary Practices

- Use namespaced keys such as `rtimv.<plugin>.<field>` to avoid collisions.
- Prefer fixed-layout POD structs or null-terminated strings for simple interoperability.
- Document the payload format in the plugin source when the blob is not self-describing.
- Treat missing keys as a normal startup condition.
- Do not assume another plugin is already loaded unless that dependency is explicit and documented.
- Keep payloads small and cheap to copy.
- Prefer replacing the whole blob with `setBlob()` rather than inventing partial-update semantics.

### Dictionary Example

For a simple string status:

```cpp
std::string status = "camera connected";
auto &blob = ( *m_dictionary )["rtimv.example.status"];
blob.setBlob( status.c_str(), status.size() + 1 );
```

And the corresponding reader:

```cpp
char buffer[256];
timespec ts;

auto it = m_dictionary->find( "rtimv.example.status" );
if( it != m_dictionary->end() )
{
    it->second.getBlobStr( buffer, sizeof( buffer ), &ts );
}
```

## Overlay Plugins

Overlay plugins implement `rtimvOverlayInterface`:

```cpp
virtual int attachOverlay( rtimvOverlayAccess &, mx::app::appConfigurator & ) = 0;
virtual int updateOverlay() = 0;
virtual void keyPressEvent( QKeyEvent *ke ) = 0;
virtual bool overlayEnabled() = 0;
virtual void enableOverlay() = 0;
virtual void disableOverlay() = 0;
```

The `rtimvOverlayAccess` struct exposes selected GUI internals:

- `m_mainWindowObject`
- `m_colorBox`
- `m_statsBox`
- `m_userBoxes`
- `m_userCircles`
- `m_userLines`
- `m_graphicsView`
- `m_dictionary`

Plugins should copy the passed `rtimvOverlayAccess` into internal state during `attachOverlay()`.

### Overlay Responsibilities

An overlay plugin usually does one or more of the following:

- add or update `QGraphicsItem` overlays in the main scene
- react to keyboard events not handled centrally by `rtimvMainWindow`
- read shared state from the dictionary
- publish plugin state back into the dictionary

### Key Handling

Overlay plugins still receive raw `keyPressEvent()` callbacks after `rtimvMainWindow` has handled built-in shortcuts and any registered text-overlay shortcut.

That means:

- built-in `rtimv` shortcuts remain centralized in `rtimvMainWindow`
- plugin text overlays should use the `rtimvInterface` text-overlay API, not ad hoc overlay key handling
- other plugin-specific hotkeys can continue to be handled in `rtimvOverlayInterface::keyPressEvent()`

Avoid conflicting with built-in keys unless the collision is intentional and coordinated.

## Plugin Text Overlays

If a plugin wants the same full-screen text panel used by `h` help and `i` info, implement these optional `rtimvInterface` methods:

```cpp
bool hasTextOverlay() override;
char textOverlayKey() override;
std::string textOverlayTitle() override;
std::string textOverlayText() override;
```

Recommended behavior:

- return `true` from `hasTextOverlay()`
- use a lowercase printable key such as `w`
- return a short label such as `warnings` from `textOverlayTitle()`
- generate current text on demand in `textOverlayText()`

`rtimvMainWindow` owns registration and toggling of these overlays. The behavior is:

- pressing the registered key shows the plugin text overlay
- pressing the same key again hides it
- pressing a different registered text-overlay key switches the shared panel contents

If a plugin registers a shortcut that is already in use, `rtimv` logs the collision and keeps the first registration.

## Minimal Skeleton

The exact Qt boilerplate depends on how you package the plugin, but the conceptual shape is:

```cpp
class examplePlugin : public QObject, public rtimvOverlayInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID rtimvOverlayInterface_iid )
    Q_INTERFACES( rtimvOverlayInterface )

  public:
    std::vector<std::string> info() override
    {
        return { "example overlay", "                demo plugin" };
    }

    int attachOverlay( rtimvOverlayAccess &roa, mx::app::appConfigurator &config ) override
    {
        static_cast<void>( config );
        m_roa = roa;
        return 0;
    }

    int updateOverlay() override
    {
        return 0;
    }

    void keyPressEvent( QKeyEvent *ke ) override
    {
        static_cast<void>( ke );
    }

    bool overlayEnabled() override
    {
        return true;
    }

    void enableOverlay() override
    {
    }

    void disableOverlay() override
    {
    }

    bool hasTextOverlay() override
    {
        return true;
    }

    char textOverlayKey() override
    {
        return 'w';
    }

    std::string textOverlayTitle() override
    {
        return "warnings";
    }

    std::string textOverlayText() override
    {
        return "Current warnings go here.\n";
    }

  protected:
    rtimvOverlayAccess m_roa;
};
```

If the plugin also needs dictionary access, implement `rtimvDictionaryInterface` as well and add the corresponding `Q_INTERFACES(...)` entry.

## Compatibility Notes

- `rtimvDictionaryInterface_iid` is currently `rtimv.dictionaryInterface/1.4`.
- `rtimvOverlayInterface_iid` is currently `rtimv.overlayInterface/1.4`.
- If the interface ID changes, out-of-tree plugins must be rebuilt and updated to advertise the new IID.
- The optional text-overlay methods were added with defaults in `rtimvInterface`, so existing plugin source code can remain unchanged unless it wants to participate in the new help system.

## Practical Advice

- Keep plugin `info()` output concise. It is displayed directly to users.
- Prefer using the shared dictionary for cross-plugin state instead of passing raw object pointers around.
- Generate plugin text-overlay help dynamically if it reflects live state.
- Log plugin failures with the standardized helpers so load and runtime problems are easy to diagnose.
- Return `> 0` from `attachDictionary()` or `attachOverlay()` when the plugin is simply not configured for the current environment, and reserve negative returns for actual errors.

## Optional commands and property snapshots

A dictionary plugin can also implement `rtimvCommandInterface`, advertised through
`Q_INTERFACES(rtimvDictionaryInterface rtimvCommandInterface)`. Its separate IID is
`rtimv.commandInterface/1.0`; the existing 1.4 interfaces and `rtimvOverlayAccess`
layout are unchanged. Build command-aware plugins against the matching installed
rtimv headers and shared library.

The main window owns an `rtimvCommandRouter` QObject child named
`rtimv.commandRouter`. Overlays find it through `m_roa.m_mainWindowObject` when
needed, after plugin loading. This lookup works regardless of provider/overlay load
order. The router keeps weak provider references; providers remain owned by plugin
loading. Exactly one provider must advertise the requested protocol. Missing or
ambiguous providers reject commands and return unavailable snapshots.

A `rtimvCommand` identifies a device, property, Number/Switch type, and a map of
numeric or boolean elements. Pass an ordered vector of commands to
`router->send(protocol, commands, error)`. The provider validates the entire batch
before submitting vectors in order, stops on failure, and never replays a batch on
reconnect. An accepted send reports local transport submission only. Camera
acknowledgement must come from incoming properties; an ordered batch is not an
atomic transaction with respect to other clients.

`router->snapshot(protocol, device, property)` returns a value copy containing the
property type, permission, state, connection generation, receive revision, and
all cached elements. `m_valid` requires a current definition and connection.
`m_complete` means the latest received message supplied every defined element;
a cache assembled from partial messages must not claim a coherent completion.
Consumers requiring an atomic property read must check both flags, expected
state/type, generation, and required elements. Definitions, deletion, reconnect,
and unavailable state must invalidate pending authorizations. All router calls
and provider snapshots run on the GUI thread; providers queue incoming work there
before modifying the shared dictionary or their snapshot cache.

For MagAO-X, `indiDictionary` provides protocol `indi` using its existing INDI
client and property subscriptions. Add normal incoming dictionary keys for each
property the overlay needs. Do not use dictionary blobs as command queues or
create a second connection. The bridge currently sends Number and Switch vectors;
Text can be observed but is not an outgoing command type.

The main-window QObject also exposes these optional slots without changing
`rtimvOverlayAccess`:

- `newUserBoxRect()` returns the standard new-box placement at the current zoom
  and viewport center.
- `overlayImageSize()` returns the displayed image dimensions.
- `addInteractiveBox(StretchBox*)` adds a scene-owned box with normal selection,
  cross/readout, maintain-center resizing, and screen-scaled line/tolerance values.
  Its cross and readout use the box's pen color.
  The overlay must retain a weak pointer, own command authorization, and clear
  selection before deleting the box. The host also keeps only a weak reference.
- `showStatusMessage(QString)` uses the normal transient zoom/status box and font
  luminance treatment. Messages retain the configured font size: the box expands
  to fit the text and wraps within a narrow view. Resizing the view reflows the
  message without restarting its timeout; short messages use a compact box.

`StretchBox::editStarted` and `editFinished` mark user mouse editing. Existing
`moved`/`resized` signals can fire repeatedly during a drag. Stage camera commands
on completion, block signals for programmatic redraws, and use `cancelEdit()` to
abort unsent mouse work. After unblocking a selected box's programmatic geometry
update, `emitSelected()` refreshes its cross/readout without staging a command.
Keyboard handlers should use exact modifiers, reject
auto-repeat for command actions, and never assume `QKeyEvent::text()` is nonempty.
The host routes Alt+R ahead of plain `r` and leaves text editors' input alone.
