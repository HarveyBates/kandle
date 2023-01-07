#!/bin/bash

package="kandle"

source_name=""
default_dir="components/extern"
cmp_name=""
filename=""
refresh=false

while getopts s:n:t:f:Rh-: flag; do
	case "${flag}" in
		s) source_name="${OPTARG}" ;;
		n) cmp_name="${OPTARG}" ;;
		t) cmp_type="${OPTARG}" ;;
		f) filename="${OPTARG}" ;;
		R) refresh=true ;;
		\?) echo "Invalid option: -"$OPTARG"" >&3
			exit 1 ;;
		:) echo "Option -"$OPTARG" requires an argument" >&2
			exit 1 ;;
		esac
	done
shift "$(( OPTIND - 1 ))"

# Check that component name was supplied
if [[ "$cmp_type" == "" ]]; then
	echo "No component type was supplied. Exiting."
	exit 1
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
	echo "Component name not found, using filename as component name: $cmp_name"
fi

# Assert that we are in the KiCAD project folder 
if [ ! -f *.kicad_pro ]; then
	echo "No KiCAD project exits in current directory."
	echo "Ensure you are running this program from within a KiCAD project directory"
	exit 1
fi

# Make directory structure and ignore if already exists
mkdir -p \
	"${default_dir}/3d_models" \
	"${default_dir}/footprints" \
	"${default_dir}/symbols" \
	"${default_dir}/tmp"

# Setup directory paths for later use
zip_dir="${default_dir}/tmp/${filename}" # Where the zip file is
output_dir="${default_dir}/tmp/${cmp_name}" # Where the unziped files will go

# Handles .zip files downloaded from SnapEDA.
# Unzips the .zip file and puts each containing file into their respective 
# directory. See: https://www.snapeda.com/ 
if [[ "$source_name" == "snapeda" ]]; then
	# Create an output directory and unzip file into there
	if [[ ! -d "$output_dir" ]]; then
		mkdir -p $output_dir
		tar -xvzf $zip_dir -C $output_dir
	else
		echo "Output directory: $output_dir already exists. Skipping unzip."
	fi

	for f in ./$output_dir/*.*; do
		# Handle schematic symbol
		if [[ $f == *.kicad_sym ]]; then
			if [[ ! -d "${default_dir}/symbols/${cmp_type}" ]]; then
				mkdir "${default_dir}/symbols/${cmp_type}"
			fi
			cp $f "${default_dir}/symbols/${cmp_type}/${cmp_name}.kicad_sym"
		fi

		# Handle footprint
		if [[ $f == *.kicad_mod ]]; then
			if [[ ! -d "${default_dir}/footprints/${cmp_type}.pretty" ]]; then
				mkdir "${default_dir}/footprints/${cmp_type}.pretty"
			fi
			cp $f "${default_dir}/footprints/${cmp_type}.pretty/${cmp_name}.kicad_mod"
		fi

		# Handle 3d-model
		if [[ $f == *.step ]]; then
			if [[ ! -d "${default_dir}/3d_models/${cmp_type}" ]]; then
				mkdir "${default_dir}/3d_models/${cmp_type}"
			fi
			cp $f "${default_dir}/3d_models/${cmp_type}/${cmp_name}.step"
		fi
	done
fi

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
				part_name="${part_name%.kicad_sym}"

				# Part type to add to description
				part_type="${sf%/*}"
				part_type="${part_type##*/}"
				sym_cache="(lib (name \"Extern_${part_type}_${part_name}\")(type \"KiCad\")(uri \"\${KIPRJMOD}/${sf}\")(options \"\")(descr \"${part_type}\"))"
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

if $refresh; then
	echo "Refreshing cached tables."
	refresh_tables
fi

echo "Complete."

