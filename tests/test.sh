#!/bin/bash

directories=("components" \
"components/extern" \
"components/extern/symbols" \
"components/extern/footprints" \
"components/extern/3dmodels" \
"components/extern/tmp")

snap_eda_v6="test_components/snapeda/v6"

echo "Starting tests."

################################################################################
# Initialise directory structure (Test 1)
################################################################################

echo -n "1. Create directory structure..."
kandle -I 1> /dev/null

for dir in "${directories[@]}"
do
  if [ ! -d $dir ]; then
    echo "failed"
    exit 1
  fi
done

echo "success"

################################################################################
# Import v6 Parts From SnapEDA (Test 2)
################################################################################

echo -n "2. Import from SnapEDA (v6 symbol version)..."

find "$snap_eda_v6" -type f | while read file; do
  library_name=$(basename $(dirname "$file"))
 kandle -f "$file" -l "$library_name" 1> /dev/null
 if [ $? -eq 1 ]; then
   echo "failed"
   exit 1
 fi
done

echo "success"



