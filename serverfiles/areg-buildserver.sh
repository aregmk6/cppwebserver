#!/bin/bash

if [ ! -e $(dirname ${0})/build/ ]; then 
    mkdir $(dirname ${0})/build/
    cmake -B $(dirname ${0})/build -S $(dirname ${0})/
fi

cmake --build $(dirname ${0})/build/
$(dirname ${0})/build/server
