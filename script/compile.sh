#!/bin/bash

function Pause() {
    read -s -n 1 -p "Press any key to continue..."
    echo ""
}

function Compile() {
    if [ -z "$1" ] || [ -z "$2" ] ; then
        echo "Compile function requires arguments. Example usage: Compile <source file> <output file>"
        Pause
        exit 0
    fi

    local src="${commandLineArguments[0]}"/"$1"
    local bin="${commandLineArguments[1]}"/"$2"

    # shellcheck disable=SC2086
    ../bin/glslc.exe "${src}" -o "${bin}"

    local message="Compiled ${src} into ${bin}"
    echo "${message}"
}

# Run the script with two arguments.
commandLineArguments=("$@")

# Check argument values
if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Example usage: compile.sh <path to source directory> <path to output directory>"
    Pause
    exit 0
fi

# Clear/create shader binary directory.
[ -d "$2" ] && ( echo "Clearing shader binary directory." && rm "$2"/* ) || ( echo "Making shader binary directory." && mkdir -p "$2" )

# Compile GLSL shader code into SPIR-V binary.
Compile triangle.vert triangle_vert.spv
Compile triangle.frag triangle_frag.spv

Pause