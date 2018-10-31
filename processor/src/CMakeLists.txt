#cmake_minimum_required(VERSION 3.9)
#project(tdomp)

#set(CMAKE_CXX_STANDARD 17)

#add_library(libtdomp )

#add_executable(processor main.cpp ExtractorVisitor.cpp RewriterVisitor.cpp)
#
#target_link_libraries(processor stdc++ clangFrontendTool clangFrontend clangDriver clangSerialization clangCodeGen
#        clangParse clangSema clangStaticAnalyzerFrontend clangStaticAnalyzerCheckers clangStaticAnalyzerCore
#        clangAnalysis clangARCMigrate clangRewrite clangEdit clangAST clangLex clangBasic LLVM
#        stdc++fs
#)

add_clang_executable(processor main.cpp ExtractorVisitor.cpp RewriterVisitor.cpp)

#target_link_libraries(processor PRIVATE clangTooling clangRewrite clangAST clangLex stdc++fs)
