#!/bin/bash

# ./build.sh Release to override
BUILD_TYPE=${1:-Debug}

clean_build() {
    rm -rf build/
    echo "Cleaned."
}

configure_build() {
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
    cd ..
}

compile_project() {
    echo "Compiling..."
    cd build
    make
    cd ..
    echo "Done."
}

if [ "$2" == "clean" ]; then
    clean_build
else
    configure_build
    compile_project
fi
