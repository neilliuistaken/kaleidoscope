#!/usr/bin/env bash

VERBOSE_MODE=0
ACTION="build"
PROJECT_BUILD_TYPE="Debug"
CMAKE_ARGUMENTS=()

Build () {
    echo "Build type: ${PROJECT_BUILD_TYPE}"
    SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
    cd "$SCRIPT_DIR"
    mkdir -p build
    cd build
    cmake ../ -GNinja -DCMAKE_BUILD_TYPE=Debug
    ninja all
}

Clean () {
    SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
    cd "$SCRIPT_DIR"
    rm -rf build
    echo "Build files are cleaned."
}

CURRENT_POSITIONAL_ARGUMENT_INDEX=0

DOUBLE_DASH_IS_PROCESSED=0

# $1 is argument value
HandlePositionalArgument () {
    if [[ "$DOUBLE_DASH_IS_PROCESSED" == "1" ]]; then
        CMAKE_ARGUMENTS+=("$1")
        return 0
    fi
    if [[ "$CURRENT_POSITIONAL_ARGUMENT_INDEX" == "0" ]]; then
        ACTION="$1"
    else
        echo "Too many positional arguments! argument '$1' is ignored."
    fi
    CURRENT_POSITIONAL_ARGUMENT_INDEX=$((CURRENT_POSITIONAL_ARGUMENT_INDEX+1))
}

# $1 is argument name
# $2 is argument value, if applicable
HandleOptionArgument () {
    case "$1" in
        "--help")
            echo "No help yet."
            exit 0
            ;;
        "--")
            DOUBLE_DASH_IS_PROCESSED=1
            ;;
        "--debug")
            PROJECT_BUILD_TYPE="Debug"
            ;;
        "--release")
            PROJECT_BUILD_TYPE="Release"
            ;;
        "--verbose" | "-v")
            VERBOSE_MODE=1
            ;;
        *)
            echo "Unrecognized option: $1"
            exit 1
            ;;
    esac
}

for var in "$@"
do
    ARG_NAME=${var%=*}
    ARG_VALUE=${var##*=}
    if [[ "$DOUBLE_DASH_IS_PROCESSED" == "0" && "$ARG_NAME" == -* ]]; then
        HandleOptionArgument "$ARG_NAME" "$ARG_VALUE"
    else
        HandlePositionalArgument "$var"
    fi
done

case "${ACTION}" in
    "build")
        echo "Build type: ${PROJECT_BUILD_TYPE}"
        cd "$SCRIPT_DIR"
        mkdir -p build
        cd build
        if [[ "$VERBOSE_MODE" == "1" ]]; then
            echo "cmake ../ -GNinja -DCMAKE_BUILD_TYPE=$PROJECT_BUILD_TYPE ${CMAKE_ARGUMENTS[*]}"
        fi
        cmake ../ -GNinja -DCMAKE_BUILD_TYPE=$PROJECT_BUILD_TYPE ${CMAKE_ARGUMENTS[*]}
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
