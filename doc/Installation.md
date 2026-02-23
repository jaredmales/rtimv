# Building and Installing

This document describes dependencies, standard CMake usage, CMake configuration options, and deployment of `rtimvServer` with `systemd`.

## Dependencies

### Build tools
- CMake >= 3.24
- A C++20 compiler
- `pkg-config`

### Required libraries
- Qt5 Widgets development package
- `mxlib`  [https://github.com/jaredmales/mxlib](https://github.com/jaredmales/mxlib)
  - The basic minimal-dependencies `mxlib` is the miminum requirement
  - For MILK/ImageStreamIO Image Streams (`shmims`) `mxlib` must be built with ImageStreamIO 

### Optional Protocol Support
- For the `milkzmq` protocol: 
  -`libzmq` (pkg-config module: `libzmq`)
    - Must be built with the "Draft API" enabled.  In 2026 the best way to obtain this is to build it yourself:
        ```
        wget https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
        tar -xvzf zeromq-4.3.4.tar.gz
        cd zeromq-4.3.4
        ./configure --enable-drafts
        make
        sudo make install
        ```
  - `xrif` [https://github.com/jaredmales/xrif](https://github.com/jaredmales/xrif)
- For local MILK/ImageStreamIO shared memory streams (`shmims`): `libImageStreamIO` from [MILK](https://github.com/milk-org/milk)
  - `mxlib` must be built with this support to enable this, see above.


### Optional (for remote client/server mode targets)
- Protobuf 
- gRPC C++ (pkg-config module: `grpc++`)

To install these on:
- Ubuntu 24:
    ```
    sudo apt install libgrpc++-dev
    sudo apt  install protobuf-compiler
    sudo apt install protobuf-compiler-grpc
    ```
- Fedora 42:
    ```
    sudo dnf install grpc grpc-devel
    ```

If Protobuf/gRPC are found, CMake builds:
- `rtimvServer`
- `rtimvClient`

If not found, local `rtimv` still builds.

## Standard CMake Workflow

From the repository root:

```bash
cmake -S . -B _build
cmake --build _build -j
```

Install (default prefix is typically `/usr/local`):

```bash
cmake --install _build
```

To install to a custom prefix:

```bash
cmake -S . -B _build -DCMAKE_INSTALL_PREFIX=/opt/rtimv
cmake --build _build -j
cmake --install _build
```

## CMake Options

### Core install/layout
- `CMAKE_INSTALL_PREFIX`  
  Install prefix (for example `/usr/local`, `/opt/rtimv`).

- `CMAKE_BUILD_TYPE`  
  Typical values: `Release`, `Debug`, `RelWithDebInfo`.

### `rtimvServer` systemd unit install options

These options are available when Protobuf + gRPC are found (the same condition used to build `rtimvServer`):

- `RTIMV_INSTALL_SYSTEMD_UNIT` (default: `ON`)  
  Install the `rtimvServer` systemd unit file.

- `RTIMV_SERVER_SYSTEMD_USER` (default: `rtimv`)  
  User account for running `rtimvServer` under systemd.

- `RTIMV_SERVER_SYSTEMD_GROUP` (default: `rtimv`)  
  Group account for running `rtimvServer` under systemd.

- `RTIMV_SERVER_SYSTEMD_ENVIRONMENT_FILE` (default: `/etc/default/rtimvServer`)  
  Environment file path referenced by the unit via `EnvironmentFile=-...`.

- `RTIMV_SERVER_SYSTEMD_ARGS` (default: empty)  
  Extra arguments appended to `ExecStart` for `rtimvServer`.

- `RTIMV_SERVER_SYSTEMD_RESTART` (default: `on-failure`)  
  Value used for `Restart=`.

- `RTIMV_SERVER_SYSTEMD_RESTART_SEC` (default: `2s`)  
  Value used for `RestartSec=`.

- `RTIMV_SERVER_SYSTEMD_UNIT_DIR` (default: `${CMAKE_INSTALL_LIBDIR}/systemd/system`)  
  Destination for the installed unit file.

## Deploying `rtimvServer` with systemd

### 1. Build and install
A typical build setup for `MagAO-X` is

```bash
cmake -S . -B _build \
  -DRTIMV_SERVER_SYSTEMD_USER=xsup \
  -DRTIMV_SERVER_SYSTEMD_GROUP=xsup \
  -DRTIMV_SERVER_SYSTEMD_ARGS="-c rtimvServer.conf"
cmake --build _build -j
sudo cmake --install _build
```

### 2. Provide environment variables (optional but common)

By default the installed unit references:

```ini
EnvironmentFile=-/etc/default/rtimvServer
```

Create that file and set variables as needed:

```bash
sudo tee /etc/default/rtimvServer >/dev/null <<'EOF'
RTIMV_CONFIG_PATH=/opt/MagAOX/config
EOF
```

`RTIMV_CONFIG_PATH`, here set to the MagAO-X standard, is used as the base path for `-c/--config` filenames.

### 3. Enable and start the service

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now rtimvServer
```

### 4. Check status and logs

```bash
systemctl status rtimvServer
journalctl -u rtimvServer -f
```

## Notes

- `rtimvServer` is intended to run as an unprivileged user.
- The installed unit is configured to restart on failure by default.
- Shell startup files (for example `~/.bashrc`, `~/.bash_aliases`) are not sourced by systemd services; use `EnvironmentFile=` or explicit `Environment=` in the unit.
