<p align="center">
  <img src="icons/kandle_master_1024.png" alt="Kandle logo" width="200">
</p>

<h1 align="center">Kandle (KiCad Component Handler)</h1>

A KiCad 9+ plugin that imports and manages third-party components (symbols,
footprints and 3D models) from **SnapEDA**, **Ultra Librarian** and
**Component Search Engine** directly inside KiCad.

It installs through the **Plugin & Content Manager**, is pure Python (wxPython
UI + an S-expression engine) and needs no external binaries.

Supported vendors:

- [x] [SnapEDA](https://www.snapeda.com/)
- [x] [Ultra Librarian](https://www.ultralibrarian.com/)
- [x] [Component Search Engine](https://componentsearchengine.com/library/kicad)

## Features

- **Drag-and-drop** a vendor `.zip` into the Kandle Component Manager window
  (launched from the PCBnew toolbar).
- **Pick an existing component type** (library) from a dropdown, or **create a
  new one**.
- **Rename** the component before importing.
- Automatically sorts the symbol, footprint and 3D model into
  `components/{symbols,footprints,models}`, links the footprint to the
  symbol, and converts legacy `.lib` symbols to `.kicad_sym`.
- **Registers** the libraries in the project `sym-lib-table` / `fp-lib-table`
  and asks KiCad to refresh over the IPC API (KiCad 9+), so components appear
  without a manual restart.
- Acts as a **component manager**: list, rename and delete imported components.

## Install

Kandle requires **KiCad 9 or newer**.

Build the PCM archive from a checkout of this repository:

```bash
python3 build_pcm.py kandle-pcm.zip
```

Then in KiCad:

1. Open the **Plugin and Content Manager** from the KiCad project window.
2. Click **Install from File…** and select the `kandle-pcm.zip` you built.
3. Click **Apply Pending Changes** to finish the install.

After restarting KiCad, the **Kandle - Component Manager** button appears on
the PCBnew toolbar.

### Enable the KiCad API (required)

Kandle talks to KiCad over the IPC API (via the bundled `kipy` package) to
register the project library tables and refresh components. The API is
disabled by default, so you must enable it before using the plugin:

1. In KiCad, open **Preferences → Preferences… → Plugins**.
2. Tick **Enable the KiCad API / IPC server**.
3. Click **OK** and restart KiCad.

Without the API enabled, Kandle cannot refresh KiCad automatically — you would
have to reload the project or restart KiCad to see newly imported components.

## Usage

1. Open your project in KiCad and switch to PCBnew.
2. Click the **Kandle** toolbar button to open the Component Manager window.
3. Drag a vendor `.zip` into the window (or use *Browse…*).
4. Choose a component type or create a new one, optionally rename the component.
5. Click **Import component**. KiCad refreshes and the part is ready to place.

Imported components land in a tidy directory structure in your project:

```
your_kicad_project/
├─ components/
│  ├─ models/
│  ├─ footprints/
│  ├─ symbols/
│  ├─ .tmp/
├─ project.kicad_pcb
├─ project.kicad_pro
├─ project.kicad_sch
```

All future symbols and footprints of an existing component type appear
automatically, with the symbol already linked to its footprint.

## Development / Tests

The backend has no KiCad or wxPython dependency and is covered by a pytest
suite:

```bash
python3 -m pytest tests/ -q
```

The UI layer (`kandle_ui`) and the action-plugin entry point require wxPython
and a running KiCad, so they are not unit tested.

## Found a problem

Submit an [issue](https://github.com/HarveyBates/kandle/issues) with:

1. A link to the component (if the issue is related to a component you are
   trying to import).
2. A description of the issue that you are facing.

## License

This project is MIT licensed, as found in the LICENCE file.
