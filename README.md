# CXXPROJECT

Simple tool to configure project-template for small and medium sized C++ project. 

## Dependencies

* g++ or clang++
* make
* cmake
* git

## Compile and install

```
$ make all
$ sudo make install
```


## Command line

The program should be installed on $PATH. It works with current directory. It
is expected that current directory is the root of a project

```
$ mkdir project
$ cd project
$ cxxproject <commands>
```


### cxxproject create executable <name>

Creates project skeleton to build  executable. You just supply name of the executable

```
/build
/build/debug
/build/release
/conf
/log
/src
/src/name
/src/name/<name>.cpp
/src/name/<name>.h
/src/name/CMakeLists.txt
.gitignore
CMakeLists.txt
Makefile
```

* The main file is <name>.cpp, contains main()
* Creates configuration for debug and releae
* Creates Makefile on root - which compiles both builds
* Output is put to /build/<configuration/bin/<name>
* Automatically adds files project to git index

### cxxproject create library <name>

```
/build
/build/debug
/build/release
/conf
/log
/src
/src/name
/src/name/<name>.cpp
/src/name/<name>.h
/src/name/CMakeLists.txt
/src/tests/compile_test.cpp
/src/tests/CMakeLists.txt
.gitignore
CMakeLists.txt
Makefile
```

* Builds a library lib<name>.a as target <name>
* Headers are accessible through #include <name/header.h>
* Source file <name>.cpp and <name>.h contains namespace <name> {  }
* The directory tests should be used for unit tests
* The compile_test.cpp just tests, whether library can be compiled

### cxxproject add library <name>

* Adds empty library to existing project
* You need to manually add the library to executable target
* Headers are accessible through #include <name/header.h>
* Source file <name>.cpp and <name>.h contains namespace <name> {  }

### cxxproject add library <name> <git-url> 

* Adds submodule as library <name>
* Searches for `library.cmake` - if such file exists, it is included to root CMakeLists.txt (see Notes)
* You need to manually add the library to executable target

### cxxproject add library <name> <git-url> <branch>

* Same as above, but you can specify branch


## Notes

Purpose of `library.cmake`

* This file should be located at root of the git-library and should contain cmake
configuration about how to build and use the library

```
include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/src )
add_subdirectory (${CMAKE_CURRENT_LIST_DIR}/src/imtjson EXCLUDE_FROM_ALL)
```

* configures include directory. In this example, it expects directory path $root/src/<libname> then library header is available as #include <libname/header.h>
* add subdirectories to compile all targets need to complete the library

