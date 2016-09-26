#!/bin/bash

count=$1

# default to generate 3 numbers randomly
if [[ $# -eq 0 ]]; then
    count=3
fi

while [[ count -gt 0 ]]; do

    count=$((count-1))
    echo $RANDOM
done

