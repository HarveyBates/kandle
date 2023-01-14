#!/bin/bash

package="kandle"

default_dir="components/extern"
cmp_name=""
filename=""
refresh=false

bold=$(tput bold)
normal=$(tput sgr0)

# Ensure we are in the right directory before continuing
# Should be in a directory containing a "*.kicad_pro" file
project_parent_dir_check() {
	if [ ! -f *.kicad_pro ]; then
		echo "No KiCad project exists in current directory."
		exit 1
	fi
}

# Make directory structure and ignore if already exists
init_project() {
	mkdir -p \
		"${default_dir}/3d_models" \
		"${default_dir}/footprints" \
		"${default_dir}/symbols" \
		"${default_dir}/tmp"
}

# List part types so you don't acciendtly use two slightly different names
# for the same part type
list_part_types(){
	for f in ${default_dir}/symbols/*; do
		if [[ -d $f ]]; then
			part_type="${f##*/}"
			echo "|- ${bold}$part_type${normal}/"
			for sf in $f/*; do
				part_name=${sf##*/}
				part_name="${part_name%.*}"
				echo "  |- $part_name"
			done
		fi
	done
}

while getopts iln:t:f:Rh-: flag; do
	case "${flag}" in
		i) 
			project_parent_dir_check
			echo -n "Building component directories..." 
			init_project
			echo "done."
			exit 0 ;;
		l) 
			project_parent_dir_check
			list_part_types
			exit 0 ;;
		n) cmp_name="${OPTARG}" ;;
		t) cmp_type="${OPTARG}" ;;
		f) filename="${OPTARG}" ;;
		R) refresh=true ;;
		h) 
			echo "$package - Handle 3rd-party KiCAD components."
			echo " "
			echo "$package [options] application [arguements]"
			echo " "
			echo "options:"
			echo "-h			Show help information."
			echo "-i	(optional)	Initialise directory structure."
			echo "-l	(optional)	List installed components and their type."
			echo "-n	(optional)	Component name. Defaults to filename (without extension)."
			echo "-R	(optional)	Refresh cached symbol and footprint tables." 
			echo "-t	(required)	Component type. E.g. op_amp, button etc."
			echo "-f	(required)	Filename. Name of file in /tmp directory you want to extract."
			exit 0
			;;
		\?) echo "Invalid option: -"$OPTARG"" >&3
			exit 1 ;;
		:) echo "Option -"$OPTARG" requires an argument" >&2
			exit 1 ;;
		esac
	done
shift "$(( OPTIND - 1 ))"

# Assert that we are in the KiCAD project folder 
project_parent_dir_check

# Initialise directory structure (may not be needed)
init_project

# Refresh tables (sym-lib-table and fp-lib-table) with components that are found
# within the footprint and symbol directories
refresh_tables(){
	# This section adds each of the symbols in the symbols dir into the symbol 
	# cache of KiCAD (sym-lib-table file)
	symbol_cache_file="sym-lib-table"
	echo "Writing component symbols to: ${symbol_cache_file}"
	touch $symbol_cache_file
	echo "(sym_lib_table" >| $symbol_cache_file
	for f in ${default_dir}/symbols/*; do
		if [ -d $f ]; then
			for sf in $f/*; do
				# Component (part) name without extension
				part_name=${sf##*/}
				extension="${part_name#*.}"
				part_name="${part_name%.*}"

				# Part type to add to description
				part_type="${sf%/*}"
				part_type="${part_type##*/}"

				# Type can either be KiCad or Legacy depending on file 
				# extension (ul is mostly .lib as far as I can see)
				lib_type="KiCad"
				if [[ $extension == "lib" ]]; then
					lib_type="Legacy"
				fi

				sym_cache="(lib (name \"Extern_${part_type}\")(type \"${lib_type}\")(uri \"\${KIPRJMOD}/${sf}\")(options \"\")(descr \"${part_type}\"))"

				echo $sym_cache >> $symbol_cache_file
				echo "Processing ${part_name} (${part_type})...done."
			done
		fi
	done
	echo ")" >> $symbol_cache_file

	# This section adds each of the footprints in the footprints dir into the 
	# footprint cache of KiCAD (fp-lib-table file)
	fp_cache_file="fp-lib-table"
	echo "Writing component symbols to: ${fp_cache_file}"
	touch $fp_cache_file
	echo "(fp_lib_table" >| $fp_cache_file
	for f in ${default_dir}/footprints/*; do
		if [ -d $f ]; then
			# Part type to add to description
			part_type="${f##*/}"
			part_type="${part_type%.*}"
			sym_cache="(lib (name \"Extern_${part_type}\")(type \"KiCad\")(uri \"\${KIPRJMOD}/${f}\")(options \"\")(descr \"${part_type}\"))"

			echo $sym_cache >> $fp_cache_file

			echo "Processing ${part_type}...done."
		fi
	done
	echo ")" >> $fp_cache_file
}

# Refresh only, no filename means we can exit straight away
if $refresh && [[ $filename == "" ]]; then
	echo "Refreshing cached tables..."
	refresh_tables
	exit 0
fi

# Check that component name was supplied
if [[ "$cmp_type" == "" ]]; then
	read -p "No component type was supplied." \ 
		"Do you what to use the default type of Unknown? (y/n)." answer
	if [ "$answer" == "y" ]; then
		$cmp_type="Unknown"
	else
		echo "Exiting..."
		exit 1
	fi
fi

# Check that filename was supplied
if [[ "$filename" == "" ]]; then
	echo "No filename was supplied. Exiting."
	exit 1
fi

# Check that supplied filename exists
if [ ! -f "${default_dir}/tmp/${filename}" ]; then
	echo "File: $filename not found in $default_dir/tmp"
	echo "Exiting"
	exit 1
fi

# Check that component type was supplied, use filename otherwise
if [[ "$cmp_name" == "" ]]; then
	cmp_name="${filename%*.*}"
	echo "Component name not provided, using filename as component name: $cmp_name"
fi

# Setup directory paths for later use
zip_dir="${default_dir}/tmp/${filename}" # Where the zip file is
output_dir="${default_dir}/tmp/${cmp_name}" # Where the unziped files will go

handle_symbol() {
	if [[ ! -d "${default_dir}/symbols/${cmp_type}" ]]; then
		mkdir "${default_dir}/symbols/${cmp_type}"
	fi

	footprint_name="";
	for f in $(find $output_dir -type f -print); do
		if [[ $f == *.kicad_mod ]]; then
			footprint_name=${f##*/}
			footprint_name="${footprint_name%.*}"
			# TODO at the moment replacing the footprint name when the cmp_name 
			# is the same results in bad filenames. Current work around is just 
			# to rename if they are different.
			if [[ $footprint_name != $cmp_name ]]; then
				sed -i '' "s/${footprint_name}/Extern_${cmp_type}:${cmp_name}/g" "$1"
				echo "Footprint for \"${cmp_name}\" automatically linked to symbol."
				break
			else
				echo "Footprint for \"${cmp_name}\" could not be linked to symbol."
				break
			fi
		fi
	done

	if [[ $footprint_name == "" ]]; then
		echo "Footprint for \"${cmp_name}\" could not be automatically assigned."
	fi

	cp $1 "${default_dir}/symbols/${cmp_type}/${cmp_name}.${2}"
}

handle_footprint(){
	if [[ ! -d "${default_dir}/footprints/${cmp_type}.pretty" ]]; then
		mkdir "${default_dir}/footprints/${cmp_type}.pretty"
	fi
	cp $1 "${default_dir}/footprints/${cmp_type}.pretty/${cmp_name}.kicad_mod"
}

handle_3d_model(){
	if [[ ! -d "${default_dir}/3d_models/${cmp_type}" ]]; then
		mkdir "${default_dir}/3d_models/${cmp_type}"
	fi
	cp $1 "${default_dir}/3d_models/${cmp_type}/${cmp_name}.${2}"
}

symbol=false
footprint=false
model=false

# Handles .zip files downloaded from SnapEDA, UltraLibrarian and 
# ComponentSearchEngine
# Unzips the .zip file and puts each containing file into their respective 
# directory. Will handle nested files and directories.
# See: https://www.snapeda.com/ 
# See: https://www.ultralibrarian.com/ 
# See: https://componentsearchengine.com/ 
recursive_extract() {
	if [[ ! -d "$output_dir" ]]; then
		mkdir -p "$output_dir"
		tar -xvzf "$zip_dir" -C "$output_dir" > /dev/null 2>&1
	else
		echo "Output directory: $output_dir already exists. Skipping unzip."
	fi

	# For CSE only, just needs to look in the KiCad folder first
	# as it provides directories from all different providers with
	# conflicting filetypes
	OIFS="$IFS"
	IFS=$'\n'
	for f in $(find "$output_dir" -type "d" -print); do
		if [[ $f == $output_dir/*/KiCad ]]; then
			echo "Detected ComponentSearchEngine part."
			search_in_dir "$f"
			break
		fi
	done
	IFS="$OIFS"

	search_in_dir "$output_dir"
}

search_in_dir(){
	# Handle spaces in filenamename
	OIFS="$IFS"
	IFS=$'\n'
	for f in $(find $1 -type "f" -print); do

		# Handle schematic symbol (both .kicad_sym and .lib (legacy))
		if [[ $f == *.kicad_sym && "$symbol" == false ]]; then
			symbol=true
			handle_symbol "$f" "kicad_sym"
		fi

		# Old symbol version
		if [[ $f == *.lib && "$symbol" == false ]]; then
			symbol=true
			handle_symbol "$f" "lib"
		fi

		# Handle footprint
		if [[ "$f" == *.kicad_mod && "$footprint" == false ]]; then
			footprint=true
			handle_footprint "$f"
		fi

		# Handle 3d-model (some models are .stp)
		if [[ $f == *.step && "$model" == false ]]; then
			model=true
			handle_3d_model "$f" "step"
		elif [[ $f == *.stp && "$model" == false ]]; then
			model=true
			handle_3d_model "$f" "stp"
		fi
	done
	IFS="$OIFS"
}

recursive_extract

if [[ "$symbol" == false ]]; then
	echo "${bold}Error${normal}, no schematic symbol found for component: $cmp_name"
else
	echo "Symbol -> found."
fi

if [[ "$footprint" == false ]]; then
	echo "${bold}Error${normal}, no PCB footprint found for component: $cmp_name"
else
	echo "Footprint -> found."
fi

if [[ "$model" == false ]]; then
	echo "${bold}Error${normal}, no 3D model found for component: $cmp_name"
else
	echo "3D-model -> found."
fi

if $refresh; then
	echo "Refreshing cached tables..."
	refresh_tables
	exit 0
fi

