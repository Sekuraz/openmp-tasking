#!/bin/env bash

cd "$1"/deps/clang
grep -q processor tools/CMakeLists.txt || echo "add_clang_subdirectory(processor)" >> tools/CMakeLists.txt
test ! -d tools/processor && ln -s "$1"/processor/src/ tools/processor
rm -rf build
mkdir -p build
cd build

#rm CMakeCache.txt

cmake -DCMAKE_BUILD_TYPE="$2" ..