# Kandle (KiCAD Component Handler)
## What is it?

A CLI that automatically imports external components into a KiCad project.

It supports files downloaded from several vendors such as:
- [x] [SnapEDA](https://www.snapeda.com/)
- [x] [Ultra Librarian](https://www.ultralibrarian.com/)
- [x] [Component Search Engine](https://componentsearchengine.com/library/kicad)

The CLI will automatically extract, rename, and link these components to your project. So all you have to do is place on your schematic or PCB.

## See it in action [here](https://www.youtube.com/watch?v=N1Pj9GHJTcU/&t=610s)
[![Video](https://img.youtube.com/vi/N1Pj9GHJTcU/mqdefault.jpg)](https://www.youtube.com/watch?v=N1Pj9GHJTcU/&t=610s)

## Installation
Download repository
```bash
git clone https://github.com/HarveyBates/kicad-component-handler
cd kicad-component-handler
```
Add kandle.sh to you path, in you shell scripts somewhere.

## Usage
>**Warning** 
>This project currently overwrites your cached symbol and footprint tables (only project specific), it is not reccomended 
that you use it in an existing project as it requires a specific directory structure and may mess up your symbol and 
footprint links.

### Step 1
In your terminal naviage to your KiCAD project. You want to be in the same directory as your `.kicad_pro` file. If not you will 
get an error message.

### Step 2
Setup (initialise) the directory structure.
```bash
kandle -i
```
This will result in a directory structure like this:
```
your_kicad_project/
├─ components/
│  ├─ extern/
│  │  ├─ 3d_models/
│  │  ├─ footprints/
│  │  ├─ symbols/
│  │  ├─ tmp/ <--- downloaded .zip files go here
├─ fp-info-table
├─ sym-info-table
├─ project.kicad_pcb
├─ project.kicad_pro
├─ project.kicad_sch
```
### Step 3
Download a component from the above supported vendors. Place the `.zip` file in the `components/extern/tmp` directory.
**Ensure the filename of the `.zip` file matches that of the component.**

### Step 4
From your project directory run.

From within the project directory run this command to unpack the .zip file download above into respectivie 
directories.
```bash
kandle -t <type_of_your_component> -f <your_download_file_name>.zip
```
Then, refresh cached symbol and footprint tables (sym-info-table & fp-info-table)
```bash
kandle -R
```
You can check the allocation of your new component by:
```bash
kandle -l
```
Which will output something like this depending on your installed 3rd-party components:
```
|- battery_holder/
  |- 1043
  |- 1048P
|- op_amp/
  |- LM358
  |- LM359M
|- regulator/
  |- LM350T
```

### Step 5
To refresh your project components you need to close your kicad project and open it again.

### Step 6
The component will now be in your symbols and footprint searches. `Extern_` will be appended to the start of each component's name so they are seperate from other components and easy to find. The component symbol and footprint will be automatically linked to eachother.

## Help
```
kandle - Handle 3rd-party KiCAD components.
 
kandle [options] application [arguements]
 
options:
-h                      Show help information.
-i      (optional)      Initialise directory structure.
-l      (optional)      List installed components and their type.
-n      (optional)      Component name. Defaults to filename (without extension).
-R      (optional)      Refresh cached symbol and footprint tables.
-t      (required)      Component type. E.g. op_amp, button etc.
-f      (required)      Filename. Name of file in /tmp directory you want to extract.
```
