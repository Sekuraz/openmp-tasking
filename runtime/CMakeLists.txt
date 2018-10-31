cmake_minimum_required(VERSION 3.9)
project(tdomp)

set(CMAKE_CXX_STANDARD 17)

add_library(libtdomp SHARED Runtime.cpp RuntimeTask.cpp Worker.cpp)
