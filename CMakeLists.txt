cmake_minimum_required(VERSION 3.6)
project(linq)

set(CMAKE_CXX_STANDARD 11)

include_directories(lib tests)
set(SOURCE_FILES tests/tests_main.cpp tests/coveo/linq/all_tests.cpp tests/coveo/linq/linq_tests.cpp tests/coveo/enumerable/enumerable_tests.cpp)
add_executable(linq ${SOURCE_FILES})