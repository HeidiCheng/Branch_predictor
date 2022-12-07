#!/bin/sh

make

tracesFolder="../traces/"
traces=("fp_1.bz2" "fp_2.bz2" "int_1.bz2" "int_2.bz2" "mm_1.bz2" "mm_2.bz2")

for file in ${traces[@]}; do
    tracePath="$tracesFolder$file"
    echo "Test on $file"
    echo -e "\nStatic"
    bunzip2 -kc $tracePath | ./predictor --static
    echo -e "\nGshare"
    bunzip2 -kc $tracePath | ./predictor --gshare:13
    echo -e "\nTournament"
    bunzip2 -kc $tracePath | ./predictor --tournament:9:10:10
    # echo -e "\nCustom"
    # bunzip2 -kc $tracePath | ./predictor --custom:9:10:10
done