#!/bin/env bash

cd "$1"/deps/clang
ln -sf ../../processor/src/ tools/processor
mkdir -p build
cd build

cmake ..