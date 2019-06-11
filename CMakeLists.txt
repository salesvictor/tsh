cmake_minimum_required(VERSION 3.5)
project(tsh)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

set(SOURCES
    src/Job.cc
    src/Shell.cc
    src/Main.cc)

include_directories(src)

add_executable(tsh ${SOURCES})
install(TARGETS tsh DESTINATION bin)