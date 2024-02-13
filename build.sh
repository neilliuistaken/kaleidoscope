#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

ACTION="build"
if [[ ! -z "$1" ]]; then
    ACTION="$1"
fi

PROJECT_BUILD_TYPE="Debug"
if [[ ! -z "$2" ]]; then
    PROJECT_BUILD_TYPE="$2"
fi

case "${ACTION}" in
    "build")
        echo "Build type: ${PROJECT_BUILD_TYPE}"
        cd "$SCRIPT_DIR"
        mkdir -p build
        cd build
        cmake ../ -GNinja -DCMAKE_BUILD_TYPE=Debug
        ninja all
        ;;
    "clean")
        cd "$SCRIPT_DIR"
        rm -rf build
        echo "Build files are cleaned."
        ;;
    *)
        echo "Unrecognized action: ${ACTION}"
        ;;
esac
