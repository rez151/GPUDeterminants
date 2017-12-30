cmake_minimum_required(VERSION 3.5)
project(GPUDeterminants)

set(CMAKE_CXX_STANDARD 11)
find_package(OpenCL 1.2 REQUIRED)

add_subdirectory(src)
add_subdirectory(test)

configure_file(src/determinants.cl src/determinants.cl COPYONLY)
include_directories(.)
