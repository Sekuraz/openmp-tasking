project(tdomp-old)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

add_library(tdomp-old SHARED Runtime.cpp RuntimeTask.cpp Worker.cpp WorkerTask.cpp WorkerTask.h)
target_link_libraries(tdomp-old PUBLIC ${MPI_LIBRARIES})


if(MPI_COMPILE_FLAGS)
    set_target_properties(tdomp-old PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(tdomp-old PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()