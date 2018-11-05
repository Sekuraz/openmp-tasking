
cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 17)

project(test)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

#add_library(libtdomp SHARED Runtime.cpp RuntimeTask.cpp Worker.cpp)
#target_link_libraries(libtdomp PUBLIC ${MPI_LIBRARIES})

#add_custom_command()

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/simple.pre.cpp
        COMMAND ../processor/processor ${CMAKE_CURRENT_SOURCE_DIR}/simple.cpp --outfile ${CMAKE_BINARY_DIR}/simple.pre.cpp
        DEPENDS ../processor/processor ${CMAKE_CURRENT_SOURCE_DIR}/simple.cpp
)

add_executable(simple ${CMAKE_BINARY_DIR}/simple.pre.cpp)
target_link_libraries(simple PRIVATE tdomp-old ${MPI_LIBRARIES})


if(MPI_COMPILE_FLAGS)
    set_target_properties(simple PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(simple PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()