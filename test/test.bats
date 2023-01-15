setup() {
	load 'test_helper/bats-support/load'
    load 'test_helper/bats-assert/load'

    DIR="$( cd "$( dirname "$BATS_TEST_FILENAME" )" >/dev/null 2>&1 && pwd )"
    PATH="$DIR/../src:$PATH"
}

@test "Create directory structure" {
	kandle.sh -i

	[ -d components ]
	[ -d components/extern ]
	[ -d components/extern/3d_models ]
	[ -d components/extern/footprints ]
	[ -d components/extern/symbols ]
	[ -d components/extern/tmp ] # Should always exist anyways for tests
}

# Ensure that the CLI works within a KiCad project directory
@test "Check commands can be run inside KiCad working directory" {
	run kandle.sh -i
	assert_success
}

# Ensure that the CLI doesn't work outside KiCad project directory
@test "Check commands cannot be run outside of working directory" {
	cd ../
	run kandle.sh -i
	assert_output --partial 'No KiCad project exists in current directory.'
	assert_failure
}

# Test extractions from SnapEDA for KiCad v6.0
# All components have all symbols, footprints and 3D-models
# https://www.snapeda.com/ 
@test "Extract and rename from SnapEDA (KiCad v6.0)" {

	run kandle.sh -i

	# Operational amplifier LM358MX 
	kandle.sh -t op_amp -n LM358MX -f SnapEDA/kicad_v6/LM358MX.zip
	[ -f components/extern/3d_models/op_amp/LM358MX.step ]
	[ -f components/extern/footprints/op_amp.pretty/LM358MX.kicad_mod ]
	[ -f components/extern/symbols/op_amp/LM358MX.kicad_sym ]

	# Operational amplifier OPA355 
	kandle.sh -t op_amp -n OPA355NA -f SnapEDA/kicad_v6/OPA355NA.zip
	[ -f components/extern/3d_models/op_amp/OPA355NA.step ]
	[ -f components/extern/footprints/op_amp.pretty/OPA355NA.kicad_mod ]
	[ -f components/extern/symbols/op_amp/OPA355NA.kicad_sym ]

	# Operational amplifier OPA2209AID 
	kandle.sh -t op_amp -n OPA2209AID -f SnapEDA/kicad_v6/OPA2209AID.zip
	[ -f components/extern/3d_models/op_amp/OPA2209AID.step ]
	[ -f components/extern/footprints/op_amp.pretty/OPA2209AID.kicad_mod ]
	[ -f components/extern/symbols/op_amp/OPA2209AID.kicad_sym ]

	# Photodiode SFH_2400_Z 
	kandle.sh -t photodiode -n SFH2400_Z -f SnapEDA/kicad_v6/SFH_2400_Z.zip
	[ -f components/extern/3d_models/photodiode/SFH2400_Z.step ]
	[ -f components/extern/footprints/photodiode.pretty/SFH2400_Z.kicad_mod ]
	[ -f components/extern/symbols/photodiode/SFH2400_Z.kicad_sym ]

	# LoRa Transceiver RFM95W_915
	kandle.sh -t lora_transceiver -n RFM95W_915 -f SnapEDA/kicad_v6/RFM95W_915.zip
	[ -f components/extern/3d_models/lora_transceiver/RFM95W_915.step ]
	[ -f components/extern/footprints/lora_transceiver.pretty/RFM95W_915.kicad_mod ]
	[ -f components/extern/symbols/lora_transceiver/RFM95W_915.kicad_sym ]
}


# Test extractions from SnapEDA for KiCad v4.0
# All components have all symbols, footprints and 3D-models
# The difference between v6 and v4 is the use of .kicad_sym and .lib respectively
# https://www.snapeda.com/ 
@test "Extract and rename from SnapEDA (KiCad v4.0)" {

	run kandle.sh -i

	# Operational amplifier HX711 
	kandle.sh -t op_amp -n HX711 -f SnapEDA/kicad_v4/HX711.zip
	[ -f components/extern/3d_models/op_amp/HX711.step ]
	[ -f components/extern/footprints/op_amp.pretty/HX711.kicad_mod ]
	[ -f components/extern/symbols/op_amp/HX711.lib ]

	# Linear Regulator LT3015MPMSE-5_PBF 
	kandle.sh -t linear_regulator -n LT3015MPMSE_5_PBF -f SnapEDA/kicad_v4/LT3015MPMSE-5_PBF.zip
	[ -f components/extern/3d_models/linear_regulator/LT3015MPMSE_5_PBF.step ]
	[ -f components/extern/footprints/linear_regulator.pretty/LT3015MPMSE_5_PBF.kicad_mod ]
	[ -f components/extern/symbols/linear_regulator/LT3015MPMSE_5_PBF.lib ]

	# Schottky Diode SS34 
	kandle.sh -t diode_schottky -n SS34 -f SnapEDA/kicad_v4/SS34.zip
	[ -f components/extern/3d_models/diode_schottky/SS34.step ]
	[ -f components/extern/footprints/diode_schottky.pretty/SS34.kicad_mod ]
	[ -f components/extern/symbols/diode_schottky/SS34.lib ]
}


