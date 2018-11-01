project(processor)

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/processor/prepare_build.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BUILD_TYPE})
add_custom_target(processor ALL $(MAKE)
        COMMAND cp ${CMAKE_SOURCE_DIR}/deps/clang/build/bin/processor ${CMAKE_BINARY_DIR}/processor/
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/deps/clang/build
        )
