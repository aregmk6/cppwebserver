#!/bin/bash

if [ ! -e $(dirname ${0})/build/ ]; then 
    mkdir $(dirname ${0})/build/
fi

cmake -B $(dirname ${0})/build -S $(dirname ${0})/

cmake --build $(dirname ${0})/build/

if [[ "${1}" == "run" ]]; then 
    $(dirname ${0})/build/server
fi
