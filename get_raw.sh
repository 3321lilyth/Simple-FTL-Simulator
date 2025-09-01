#!/bin//bash

mkdir rawfile
for i in {0,1,2,3,4,6}
do
    echo "$i"
    cat ./all/*-LUN$i.csv.gz | gzip -d > ./rawfile/LUN$i.csv;
done