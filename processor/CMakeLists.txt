project(processor)

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/processor/prepare_build.sh ${CMAKE_SOURCE_DIR})
add_custom_target(processor ALL make -j 5
        COMMAND cp ${CMAKE_SOURCE_DIR}/deps/clang/build/tools/processor ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/clang/build
        )
