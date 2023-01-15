# Kandle (KiCAD Component Handler)
![tests_badge](https://github.com/HarveyBates/kicad-component-handler/actions/workflows/tests.yml/badge.svg)
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
chmod a+x src/kandle.sh # Modify permissions to make the script executable
export PATH=$PATH:$PWD/src # Add the script to your PATH (this allows you to run it from any directory)
```
If you want to permanently add the script to your path here is a [tutorial](https://appuals.com/how-to-make-a-program-executable-from-everywhere-in-linux/).

## Usage
>**Warning** 
>This project currently overwrites your cached symbol and footprint tables (only project specific), it is not recommended 
that you use it in an existing project as it requires a specific directory structure and may mess up your symbol and 
footprint links.

### Step 1
In your terminal navigae to your KiCAD project. You want to be in the same directory as your `.kicad_pro` file. If not you will get an error message saying "*No KiCad project exists in current directory.*".

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
Download a component from the above supported vendors. Place the `.zip` file in the `components/extern/tmp` directory. It's your choice, but its good idea to rename the `.zip` file with the component name and underscores instead of spaces or dashes.

>**Example:**
>If the filename is `PESD 0402-140.zip` rename it to `PESD_0402_140.zip`. This will allow you to skip renaming it later and I think its more robust to not have spaces in filenames.

### Step 4
From within the project directory run this command to unpack the .zip file download above into respective 
directories. This will use the filename as the component name. 
```bash
kandle -t <type_of_your_component> -f <your_download_file_name>.zip
```
**Or**, you can specify the part name using the command (if you didn't change the filename above):
```bash
kandle -t <type_of_your_component> -n <name_of_your_component> -f <your_download_file_name>.zip
```
>**Note:**
> The current version has a limitation where the depth of component type can only be one level. For example, you can use the component type of `op_amp` but you cant use `op_amp/rail_to_rail`.

### Step 5
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

### Step 6
To refresh your project components you need to close your KiCad project and open it again.

### Step 7
The component will now be in your symbols and footprint searches. `Extern_` will be appended to the start of each component's name so they are seperate from other components and easy to find. The component symbol and footprint will be automatically linked to each other.

## Help
```
kandle - Handle 3rd-party KiCAD components.
 
kandle [options] application [arguments]
 
options:
-h                      Show help information.
-i      (optional)      Initialise directory structure.
-l      (optional)      List installed components and their type.
-n      (optional)      Component name. Defaults to filename (without extension).
-R      (optional)      Refresh cached symbol and footprint tables.
-t      (required)      Component type. E.g. op_amp, button etc.
-f      (required)      Filename. Name of file in /tmp directory you want to extract.
```

## Found a problem 
Submit an [issue](https://github.com/HarveyBates/kicad-component-handler/issues) with:
1. A link to the component (if the issue is related to a component you are trying to import).
2. A description of the issue that you are facing.

## License
This project is MIT licensed, as found in the LICENCE file.

