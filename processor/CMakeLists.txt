cmake_minimum_required(VERSION 3.9)
project(processor)

set(CMAKE_CXX_STANDARD 17)

add_executable(processor main.cpp tasking.h)

target_link_libraries(processor stdc++ clangFrontendTool clangFrontend clangDriver clangSerialization clangCodeGen
        clangParse clangSema clangStaticAnalyzerFrontend clangStaticAnalyzerCheckers clangStaticAnalyzerCore
        clangAnalysis clangARCMigrate clangRewrite clangEdit clangAST clangLex clangBasic LLVM-5.0
        stdc++fs
)