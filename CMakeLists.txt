cmake_minimum_required(VERSION 3.16)
project(SequenceState C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()

add_library(sscore STATIC
  core/src/sequence_state.c
  candidates/candidates.c
  baselines/baselines.c)
target_include_directories(sscore PUBLIC core/include candidates baselines)

add_executable(ss_tests tests/test_main.c)
target_link_libraries(ss_tests PRIVATE sscore)

add_executable(ss_benchmark benchmarks/smoke_benchmark.c)
target_link_libraries(ss_benchmark PRIVATE sscore)

enable_testing()
add_test(NAME unit_and_properties COMMAND ss_tests)
add_test(NAME smoke_benchmark COMMAND ss_benchmark)
