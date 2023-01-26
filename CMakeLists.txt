cmake_minimum_required(VERSION 3.24)
project(itmo_parallel_algo)
set(CMAKE_CXX_STANDARD 17)

add_executable(quicksort quicksort.cpp)
add_executable(bfs bfs.cpp)

include(FetchContent)
FetchContent_Declare(parlaylib
        GIT_REPOSITORY  https://github.com/cmuparlay/parlaylib.git
        GIT_TAG         master
        )
FetchContent_GetProperties(parlaylib)
if(NOT parlaylib_POPULATED)
    FetchContent_Populate(parlaylib)
    add_subdirectory(${parlaylib_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

target_link_libraries(quicksort PUBLIC parlay)
target_link_libraries(bfs PUBLIC parlay)
