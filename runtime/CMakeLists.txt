project(tdomp)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

add_library(schedule SHARED src/Scheduler.cpp)

add_library(tdomp SHARED src/Task.cpp src/Worker.cpp src/Runtime.cpp src/Receiver.cpp src/globaldef.cpp)
target_link_libraries(tdomp PUBLIC schedule ${MPI_LIBRARIES})



if(MPI_COMPILE_FLAGS)
    set_target_properties(tdomp PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(tdomp PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

add_executable(create_task test/create_task.cpp)
target_link_libraries(create_task tdomp)
add_executable(setup test/setup.cpp)
target_link_libraries(setup tdomp)
add_executable(one_task test/one_task.cpp)
target_link_libraries(one_task tdomp)
add_executable(run_main test/run_main.cpp)
target_link_libraries(run_main tdomp)