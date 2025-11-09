cmake_minimum_required(VERSION 3.30)
project(CustomAllocatorDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(CustomAllocatorDemo
    src/main.cpp
)

# no linking needed; header-only-ish modules are included
