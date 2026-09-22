cmake_minimum_required(VERSION 3.16)
project(SequenceState C)
include(CheckLanguage)
check_language(CXX)
if(CMAKE_CXX_COMPILER)
  enable_language(CXX)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()

add_library(sscore STATIC
  core/src/trace.c
  core/src/compose.c
  core/src/range.c)
target_include_directories(sscore PUBLIC include)

option(SS_BUILD_RESEARCH "Build historical candidate and baseline programs" OFF)
option(SS_BUILD_FUZZ "Build the Clang libFuzzer public-API target" OFF)

add_executable(ss_example_trace_c examples/example_trace_c.c)
target_link_libraries(ss_example_trace_c PRIVATE sscore)
add_executable(ss_example_compose_h examples/example_compose_h.c)
target_link_libraries(ss_example_compose_h PRIVATE sscore)
add_executable(ss_example_sliding_h2 examples/example_sliding_h2.c)
target_link_libraries(ss_example_sliding_h2 PRIVATE sscore)
if(CMAKE_CXX_COMPILER)
  add_executable(ss_public_header_cpp tests/public_header_cpp.cpp)
  target_link_libraries(ss_public_header_cpp PRIVATE sscore)
endif()

add_executable(ss_tests tests/test_main.c)
target_link_libraries(ss_tests PRIVATE sscore)
add_executable(ss_property_stress tests/property_stress.c)
target_link_libraries(ss_property_stress PRIVATE sscore)

if(SS_BUILD_RESEARCH)
  # Historical implementation retained for reproducibility. It is not part
  # of the production library or its public include path.
  add_library(ssresearch STATIC
    core/src/sequence_state.c
    candidates/candidates.c
    candidates/affine_h.c
    candidates/h2.c
    baselines/baselines.c)
  target_include_directories(ssresearch PUBLIC core/include candidates baselines)
  add_executable(ss_benchmark benchmarks/smoke_benchmark.c)
  target_link_libraries(ss_benchmark PRIVATE ssresearch)
  add_executable(ss_long_range experiments/long_range_profile.c)
  target_link_libraries(ss_long_range PRIVATE ssresearch)
  add_executable(ss_survivor_golden experiments/survivor_golden.c)
  target_link_libraries(ss_survivor_golden PRIVATE ssresearch)
  add_executable(ss_rollback_profile experiments/rollback_profile.c)
  target_link_libraries(ss_rollback_profile PRIVATE ssresearch)
  add_executable(ss_survivor_benchmark experiments/survivor_benchmark.c)
  target_link_libraries(ss_survivor_benchmark PRIVATE ssresearch)
endif()

if(SS_BUILD_FUZZ)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "SS_BUILD_FUZZ requires a Clang C compiler")
  endif()
  add_executable(ss_fuzz_public tests/fuzz_public.c)
  target_link_libraries(ss_fuzz_public PRIVATE sscore)
  target_compile_options(ss_fuzz_public PRIVATE -fsanitize=fuzzer,address,undefined)
  target_link_options(ss_fuzz_public PRIVATE -fsanitize=fuzzer,address,undefined)
endif()

enable_testing()
add_test(NAME unit_and_properties COMMAND ss_tests)
add_test(NAME property_stress COMMAND ss_property_stress)
add_test(NAME golden_vectors
  COMMAND ${CMAKE_COMMAND} -DGOLDEN_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify_golden.cmake)
add_test(NAME example_trace_c COMMAND ss_example_trace_c)
add_test(NAME example_compose_h COMMAND ss_example_compose_h)
add_test(NAME example_sliding_h2 COMMAND ss_example_sliding_h2)
if(TARGET ss_public_header_cpp)
  add_test(NAME public_header_cpp COMMAND ss_public_header_cpp)
endif()

include(GNUInstallDirs)
install(TARGETS sscore ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
