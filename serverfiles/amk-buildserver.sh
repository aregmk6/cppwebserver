#!/bin/bash

if [ ! -e $(dirname ${0})/build/ ]; then 
    mkdir $(dirname ${0})/build/
fi

cmake -B $(dirname ${0})/build -S $(dirname ${0})/

if [[ "${1}" == "b" || "${1}" == "r" ]]; then 
    cmake --build $(dirname ${0})/build/
fi

if [[ "${1}" == "r" ]]; then 
    $(dirname ${0})/build/server
fi
