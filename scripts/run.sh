#!/bin/bash

if [ "$1" = "release" ]
then
    ./build/release/bin/enet-example
else
    ./build/debug/bin/enet-example
fi