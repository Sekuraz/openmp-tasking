#!/bin/env bash

cd ${CMAKE_SOURCE_DIR}/deps/clang
ln -sf ../../processor/src/ tools/processor/
mkdir -p build
cd build

cmake ..