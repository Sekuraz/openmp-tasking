cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 17)

include_directories(deps/clang/include)
include_directories(include)


add_subdirectory(processor)
add_subdirectory(runtime)
add_subdirectory(test)
