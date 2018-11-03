
cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 17)

project(test)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

#add_library(libtdomp SHARED Runtime.cpp RuntimeTask.cpp Worker.cpp)
#target_link_libraries(libtdomp PUBLIC ${MPI_LIBRARIES})

#add_custom_command()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/simple.pre.cpp
  COMMAND ../processor/processor ${CMAKE_CURRENT_SOURCE_DIR}/simple.pre.cpp --outfile ${CMAKE_CURRENT_BINARY_DIR}/simple.pre.cpp -- ${CMAKE_CXX_FLAGS}
  )


add_executable(simple ${CMAKE_CURRENT_BINARY_DIR}/simple.pre.cpp)
target_link_libraries(simple PRIVATE libtdomp ${MPI_LIBRARIES})
target_compile_options(simple PUBLIC --compile)
set(CMAKE_COMPILER ${CMAKE_SOURCE_DIR}/processor/processor)

if(MPI_COMPILE_FLAGS)
    set_target_properties(simple PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(simple PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()