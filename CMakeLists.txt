cmake_minimum_required(VERSION 3.10)
project(mython VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

file(GLOB sources
    *.cpp
    *.h
)
add_executable(mython ${sources})