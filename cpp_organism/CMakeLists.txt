cmake_minimum_required(VERSION 3.15)
project(genesis_organism LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(genesis_organism STATIC
    src/IModelRuntime.cpp
)

target_include_directories(genesis_organism PUBLIC include)

add_executable(genesis_organism_demo
    demo/main.cpp
)
target_link_libraries(genesis_organism_demo PRIVATE genesis_organism)
