
cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 17)

project(test)


add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/simple.pre.cpp
        COMMAND ../processor/processor ${CMAKE_CURRENT_SOURCE_DIR}/simple.cpp --outfile ${CMAKE_BINARY_DIR}/simple.pre.cpp
        DEPENDS ../processor/processor ${CMAKE_CURRENT_SOURCE_DIR}/simple.cpp
)

add_executable(simple ${CMAKE_BINARY_DIR}/simple.pre.cpp)
target_link_libraries(simple PRIVATE tdomp-old)

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/main_task.pre.cpp
        COMMAND ../processor/processor ${CMAKE_CURRENT_SOURCE_DIR}/main_task.cpp --outfile ${CMAKE_BINARY_DIR}/main_task.pre.cpp
        DEPENDS ../processor/processor ${CMAKE_CURRENT_SOURCE_DIR}/main_task.cpp
)

add_executable(main_task ${CMAKE_BINARY_DIR}/main_task.pre.cpp)
target_link_libraries(main_task PRIVATE tdomp)