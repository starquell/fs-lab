cmake_minimum_required(VERSION 3.9)
project(fslab)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(fmt REQUIRED)

set(SRC_LIST src/IO.cpp)

add_executable(main main.cpp ${SRC_LIST})

target_link_libraries(main PRIVATE fmt::fmt)
