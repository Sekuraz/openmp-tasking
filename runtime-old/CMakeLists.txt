project(tdomp)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

add_library(libtdomp SHARED Runtime.cpp RuntimeTask.cpp Worker.cpp WorkerTask.cpp WorkerTask.h)
target_link_libraries(libtdomp PUBLIC ${MPI_LIBRARIES})

add_executable(tdomp main.cpp)
target_link_libraries(tdomp PRIVATE libtdomp ${MPI_LIBRARIES})

if(MPI_COMPILE_FLAGS)
    set_target_properties(libtdomp PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
    set_target_properties(tdomp PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(libtdomp PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
    set_target_properties(tdomp PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()