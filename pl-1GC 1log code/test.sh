#!/bin/bash

executable="./ssd.exe"
# params=("16k2m80g819m.txt" "16k2m80g1638m.txt" "16k2m80g4096m.txt" "16k2m80g5734m.txt" "16k2m80g8192m.txt" "16k2m80g81920m.txt")  # 80G 
params=("4k64m128g1310m.txt" "4k64m128g6553m.txt" "4k64m128g9175m.txt" "4k64m128g13107m.txt" "4k64m128g65536m.txt" "4k64m128g131072m.txt")  # 128G 
# params=("16k2m160g1638m.txt" "16k2m160g3276m.txt" "16k2m160g8192m.txt" "16k2m160g11468m.txt" "16k2m160g16384m.txt")  # 160G 


# check files exist
for param in "${params[@]}"; do
    if [ ! -f "../Config File/my_config/$param" ]; then
        echo "File $param does not exist."
        exit 1
    fi
done

# execute!
make
for param in "${params[@]}"; do
    echo "Executing $executable with parameter: $param"
    "$executable" "$param"
done
make clean