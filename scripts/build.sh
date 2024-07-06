#!/bin/bash

target_release() {
    cd release
    cmake -DCMAKE_BUILD_TYPE=Release ../..
    make
    echo "Built target in build/release/"
    cd ../..
}

target_debug() {
    cd debug
    cmake -DCMAKE_BUILD_TYPE=Debug ../..
    make
    echo "Built target in build/debug/"
    cd ../..
}

# Create folder for distribution
if [ "$1" = "release" ]
then
    if [ -d "$enet-example" ]
    then
        rm -rf -d enet-example
    fi

    mkdir -p enet-example
fi

# Creates the folder for the buildaries
mkdir -p enet-example
mkdir -p enet-example/assets
mkdir -p build
mkdir -p build/release
mkdir -p build/debug
cd build

if [ "$1" = "install" ]
then
    conan install .. -s compiler.libcxx=libstdc++11 --build=missing
fi

# Builds target
if [ "$1" = "release" ]
then
    target_release
    cp build/release/bin/enet-example enet-example /enet-example
else
    target_debug
fi

cp -R assets enet-example/
