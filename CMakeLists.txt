cmake_minimum_required(VERSION 3.24)
project(Genesis VERSION 0.10.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(GENESIS_ENABLE_SANITIZERS "Enable address and undefined-behaviour sanitizers" OFF)
include(GNUInstallDirs)

add_library(genesis_core STATIC
    src/common/text.cpp
    src/cognition/workspace.cpp
    src/cognition/persistence.cpp
    src/development/maturity.cpp
    src/genetics/expression.cpp
    src/genetics/genome.cpp
    src/genetics/persistence.cpp
    src/genetics/reproduction.cpp
    src/identity/lineage.cpp
    src/learning/consolidation.cpp
    src/learning/persistence.cpp
    src/memory/origin.cpp
    src/memory/graph.cpp
    src/memory/persistence.cpp
    src/organism/anatomy.cpp
    src/organism/systems.cpp
    src/provenance/ledger.cpp
    src/requirements/registry.cpp
    src/runtime/runtime.cpp
)
target_include_directories(genesis_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(genesis_core PUBLIC cxx_std_20)
target_compile_options(genesis_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
if(GENESIS_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(genesis_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(genesis_core PUBLIC -fsanitize=address,undefined)
endif()
enable_testing()
add_executable(genesis_tests tests/genesis_tests.cpp)
target_link_libraries(genesis_tests PRIVATE genesis_core)
target_compile_options(genesis_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
add_test(NAME genesis_tests COMMAND genesis_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/registry/requirements.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/provenance/SOURCE_MANIFEST.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/registry/domains.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/registry/canonical_sections.tsv)
add_executable(genesis_bench benchmarks/core_benchmark.cpp)
target_link_libraries(genesis_bench PRIVATE genesis_core)
add_executable(genesis_runtime_tests tests/runtime_tests.cpp)
target_link_libraries(genesis_runtime_tests PRIVATE genesis_core)
target_compile_options(genesis_runtime_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
add_test(NAME genesis_runtime_tests COMMAND genesis_runtime_tests)
add_executable(genesis_runtime_bench benchmarks/runtime_benchmark.cpp)
target_link_libraries(genesis_runtime_bench PRIVATE genesis_core)
add_executable(genesis_genetics_tests tests/genetics_tests.cpp)
target_link_libraries(genesis_genetics_tests PRIVATE genesis_core)
target_compile_options(genesis_genetics_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
add_test(NAME genesis_genetics_tests COMMAND genesis_genetics_tests)
add_executable(genesis_genetics_bench benchmarks/genetics_benchmark.cpp)
target_link_libraries(genesis_genetics_bench PRIVATE genesis_core)
add_executable(genesis_research_registry_tests tests/research_registry_tests.cpp)
target_link_libraries(genesis_research_registry_tests PRIVATE genesis_core)
target_compile_options(genesis_research_registry_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
add_test(NAME genesis_research_registry_tests COMMAND genesis_research_registry_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/registry/research_items.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/registry/research_classes.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/registry/research_dead_ends.tsv)
add_executable(genesis_organism_tests tests/organism_tests.cpp)
target_link_libraries(genesis_organism_tests PRIVATE genesis_core)
target_compile_options(genesis_organism_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
add_test(NAME genesis_organism_tests COMMAND genesis_organism_tests)
add_executable(genesis_organism_bench benchmarks/organism_benchmark.cpp)
target_link_libraries(genesis_organism_bench PRIVATE genesis_core)
add_executable(genesis_memory_cognition_tests tests/memory_cognition_tests.cpp)
target_link_libraries(genesis_memory_cognition_tests PRIVATE genesis_core)
target_compile_options(genesis_memory_cognition_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
add_test(NAME genesis_memory_cognition_tests COMMAND genesis_memory_cognition_tests)
add_executable(genesis_learning_tests tests/learning_tests.cpp)
target_link_libraries(genesis_learning_tests PRIVATE genesis_core)
target_compile_options(genesis_learning_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->)
add_test(NAME genesis_learning_tests COMMAND genesis_learning_tests)
add_executable(genesis_memory_bench benchmarks/memory_benchmark.cpp)
target_link_libraries(genesis_memory_bench PRIVATE genesis_core)
add_executable(genesis_learning_bench benchmarks/learning_benchmark.cpp)
target_link_libraries(genesis_learning_bench PRIVATE genesis_core)

install(TARGETS genesis_core EXPORT GenesisTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT GenesisTargets
    FILE GenesisTargets.cmake
    NAMESPACE Genesis::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Genesis)
