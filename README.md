# Kandle (KiCAD Component Handler)

## What is it?

A CLI that handles external components in a KiCad project.

It supports files downloaded from several vendors such as:

- [x] [SnapEDA](https://www.snapeda.com/)
- [x] [Ultra Librarian](https://www.ultralibrarian.com/)
- [x] [Component Search Engine](https://componentsearchengine.com/library/kicad)

The CLI will:
- [x] Handle the unzipping of files downloaded from the above providers.
- [x] Place symbols, footprints and models in a define directory structure.
- [x] Convert KiCad v4 symbols (`.lib` files) to KiCad v6 symbols (`.kicad_sym` files).
- [x] Link a symbol and footprint automatically.
- [x] Automaticlly import the symbol **if a symbol library for that component exists* (same goes for the footprint)

## See it in action [here](https://www.youtube.com/watch?v=nRz4pBlJtNc/&t=120s)

[![Video](https://img.youtube.com/vi/nRz4pBlJtNc/mqdefault.jpg)](https://www.youtube.com/watch?v=nRz4pBlJtNc/&t=120s)

Or see an [example project](https://github.com/HarveyBates/u-nit/tree/main/electronics/u-nit/components/extern).

## Installation

Download repository

```bash
git clone https://github.com/HarveyBates/kandle
cd kandle
mkdir build
cd build
cmake ..
make
make install # (optional, if not you need to add kandle/build/bin to your path)
```

If you want to permanently add the script to your path here is
a [tutorial](https://appuals.com/how-to-make-a-program-executable-from-everywhere-in-linux/).

## Usage

### Step 1

In your terminal navigae to your KiCAD project. You want to be in the same
directory as your `.kicad_pro` file. If not you will get an error message
saying "*KiCad project not found in current working directory. Exiting.*".

### Step 2

Setup (initialise) the directory structure.

```bash
kandle -I
```

This will result in a directory structure like this:

```
your_kicad_project/
├─ components/
│  ├─ extern/
│  │  ├─ 3dmodels/
│  │  ├─ footprints/
│  │  ├─ symbols/
│  │  ├─ tmp/ <--- unzipped files will be stored here
├─ fp-info-table
├─ sym-info-table
├─ project.kicad_pcb
├─ project.kicad_pro
├─ project.kicad_sch
```

### Step 3

Download a component from the above supported vendors. No need to rename the `.zip` file. 
Kandle will remove "ul_" (from Ultra-Librarian) and "LIB_" (from CSE). It will also replace all `-` or `spaces` with `_`.

> **Example:**
> If the filename is `LIB_PESD 0402-140.zip` it will become `PESD_0402_140`.

### Step 4

Execute kandle passing the file you just downloaded and the library the part belongs to. 

> **Example:**
> If the part is an operational amplifier called LM358 stored in your `Downloads` directory. 
> Your command would look like this:
> `kandle -f Downloads/LM358.zip -l operational_amplifier`

```bash
kandle -f <your_download_file_name>.zip -l <library_name>
```
### Step 5
Open Eeschema -> Preferences -> Manage Symbol Libraries -> Project Specific Libraries -> Add existing.

Using the above example, you would import the file `components/extern/symbols/operational_amplifier.kicad_sym`.

### Step 6
Open PCBnew -> Preferences -> Manage Footprint Libraries -> Add existing. 

Using the above example, you would import the file `components/extern/footprints/operational_amplifier.pretty`.

> **Attention**
> Kandle will automatically change default text size of footprint names to 1 with a thickness of 0.15 (KiCAD defaults).

### Step 7 
Place on schematic in Eeschema. In the above example you would search for `operational_amplifier` and then select LM358. The symbol should already be linked to the footprint.

## Help

```
KiCAD 3rd Party Component Management Tool
Usage:
  kandle [OPTION...]

  -I, --init          Initialise a KiCAD project with Kandle.
  -L, --list          List existing component libraries.
  -f, --filename arg  Path to zipped (.zip) component file.
  -l, --library arg   Name of the library the component belongs to.
  -h, --help          Help information.
```

## Found a problem

Submit
an [issue](https://github.com/HarveyBates/kicad-component-handler/issues) with:

1. A link to the component (if the issue is related to a component you are
   trying to import).
2. A description of the issue that you are facing.

## License

This project is MIT licensed, as found in the LICENCE file.

