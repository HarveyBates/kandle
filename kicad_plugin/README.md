# Kandle KiCad Plugin

A KiCad 9+ plugin that imports and manages third-party components (symbols,
footprints and 3D models) from **SnapEDA**, **Ultra Librarian** and
**Component Search Engine** directly inside KiCad.

It is a pure-Python port of the Kandle CLI with a wxPython UI and an
S-expression engine, so it installs cleanly through the Plugin & Content
Manager and needs no external binaries.

## Features

- **Drag-and-drop** a vendor `.zip` into the Kandle Component Manager window
  (launched from the PCBnew toolbar).
- **Pick an existing component type** (library) from a dropdown, or **create a
  new one**.
- **Rename** the component before importing.
- Automatically sorts the symbol, footprint and 3D model into
  `components/extern/{symbols,footprints,3dmodels}`, links the footprint to the
  symbol, and converts legacy `.lib` symbols to `.kicad_sym`.
- **Registers** the libraries in the project `sym-lib-table` / `fp-lib-table`
  and asks KiCad to refresh over the IPC API (KiCad 9+), so components appear
  without a manual restart.
- Acts as a **component manager**: list, rename and delete imported components.

## Layout

```
kicad_plugin/
├─ metadata.json            # PCM package metadata (KiCad 9+)
├─ build_pcm.py             # builds the installable PCM archive
├─ resources/icon.png       # toolbar / PCM icon
├─ plugins/
│  ├─ __init__.py           # registers the action plugin with KiCad
│  ├─ action_kandle.py      # pcbnew.ActionPlugin entry point
│  ├─ kandle_core/          # pure-Python backend (no KiCad deps, fully tested)
│  │  ├─ sexpr.py           # S-expression parser / pretty-printer
│  │  ├─ legacy_converter.py# .lib -> .kicad_sym converter
│  │  ├─ symbol_library.py  # safe add/rename/delete inside .kicad_sym
│  │  ├─ lib_table.py       # sym-/fp-lib-table registration
│  │  ├─ file_handler.py    # unzip + classify vendor archives
│  │  ├─ project.py         # project discovery + directory structure
│  │  ├─ manager.py         # high-level ComponentManager API
│  │  └─ reload.py          # best-effort IPC reload (KiCad 9+)
│  └─ kandle_ui/            # wxPython UI (not unit tested)
│     └─ manager_frame.py
└─ tests/                   # pytest suite for the backend
```

## Install

Build the archive and install it in KiCad:

```bash
python3 kicad_plugin/build_pcm.py kandle-pcm.zip
```

In KiCad: **Plugin and Content Manager → Install from File…** → select
`kandle-pcm.zip`. The **Kandle - Component Manager** button appears on the
PCBnew toolbar.

## Usage

1. Open your project in KiCad and switch to PCBnew.
2. Click the **Kandle** toolbar button to open the Component Manager window.
3. Drag a vendor `.zip` into the window (or use *Browse…*).
4. Choose a component type or create a new one, optionally rename the component.
5. Click **Import component**. KiCad refreshes and the part is ready to place.

## Development / Tests

The backend has no KiCad or wxPython dependency and is covered by a pytest
suite:

```bash
cd kicad_plugin
python3 -m pytest tests/ -q
```

The UI layer (`kandle_ui`) and the action-plugin entry point require wxPython
and a running KiCad, so they are not unit tested.
