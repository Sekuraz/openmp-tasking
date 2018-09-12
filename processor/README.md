# Compile
This project has to be built within a full clang source tree.
Please read the instructions on how to build clang on the project website.

Then copy or link this folder to the tools subfolder and add the following line to the CMakeLists.txt in the tools folder.

    add_clang_subdirectory(processor)

The result is a standalone application with all the necessary dependeencies built in.

# Run
The preprocessor accepts all clang command line options after a `--` in order to get the same code recognition and include
file handling clang would receive to compile the files.
This might also be handy if you want to integrate this preprocessor into your build script.

If there is only one parameter, this is the file to be processed, otherwise the filename is taken from the clang parameters.
If there is no file given in those parameters we default to a file called `test.cpp`