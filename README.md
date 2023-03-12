# Kandle (KiCAD Component Handler)

![tests_badge](https://github.com/HarveyBates/kicad-component-handler/actions/workflows/tests.yml/badge.svg)

## What is it?

A CLI that automatically imports external components into a KiCad project.

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

## See it in action [here](https://www.youtube.com/watch?v=N1Pj9GHJTcU/&t=610s)

[![Video](https://img.youtube.com/vi/N1Pj9GHJTcU/mqdefault.jpg)](https://www.youtube.com/watch?v=N1Pj9GHJTcU/&t=610s)

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

Download a component from the above supported vendors. Rename the `.zip` file with the component name.

> **Example:**
> If the filename is `PESD 0402-140.zip` rename it to `PESD_0402-140.zip`.

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

### Step 7 
Place on schematic in Eeschema. In the above example you would search for `operational_amplifier` and then select LM358. The symbol should already be linked to the footprint.

## Help

```
KiCAD 3rd Party Component Management Tool
Usage:
  kandle [OPTION...]

  -I, --init          Initialise a KiCAD project with Kandle
  -f, --filename arg  Path to zipped (.zip) component file
  -l, --library arg   Name of the library the component belongs to.
  -h, --help          Print usage
```

## Found a problem

Submit
an [issue](https://github.com/HarveyBates/kicad-component-handler/issues) with:

1. A link to the component (if the issue is related to a component you are
   trying to import).
2. A description of the issue that you are facing.

## License

This project is MIT licensed, as found in the LICENCE file.

