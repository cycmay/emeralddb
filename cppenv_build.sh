#!/bin/sh

echo 'Lets build a C plus plus envirment for programming!\n'
read -p 'please input the name of this project:\n' name

echo 'defult CMake version:3.10, project version:1.0, build model:Debug'

mkdir build
mkdir bin
mkdir lib
mkdir src
mkdir include

touch src/hello.cxx
echo  "
#include <iostream>
int main()
{
    std::cout<<\"hello!\"<<std::endl;
    return 0;
} 
" > src/hello.cxx

touch CMakeLists.txt

echo "
cmake_minimum_required(VERSION 3.10)
project(${name})
#the version number
set(hello_VERSION_MAJOR 1)
set(hello_VERSION_MINOR 0)

#set build model
SET(CMAKE_BUILD_TYPE Debug)

#configure a header file to pass some of the CMake settings
#to the source code
#configure_file(
#    \${PROJECT_SOURCE_DIR}/src/helloConfig.h.in
#    \${PROJECT_SOURCE_DIR}/src/helloConfig.h
#)

#set headfile path
include_directories(
    \${PROJECT_SOURCE_DIR}/include
)

#set execute path
set(EXECUTABLE_OUTPUT_PATH \${PROJECT_SOURCE_DIR}/bin)
#set library path
set(LIBRARY_OUTPUT_PATH \${PROJECT_SOURCE_DIR}/lib)

add_executable(
    ${name}
    src/hello.cxx
     )
" > CMakeLists.txt

cd build
cmake ..
make

echo "\n successful! \n end!"

exit 0
