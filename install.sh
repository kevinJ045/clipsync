#!/bin/sh

path=$1/bin

echo "Install at $path?" 

read -r s

if [ "$s" == "y" ]; then
    cp ./clipsync $path
fi
