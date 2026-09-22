if(NOT DEFINED GOLDEN_ROOT)
  message(FATAL_ERROR "GOLDEN_ROOT is required")
endif()

file(SHA256 "${GOLDEN_ROOT}/tests/golden_vectors.txt" actual_hash)
file(READ "${GOLDEN_ROOT}/tests/golden_vectors.sha256" recorded)
string(REGEX MATCH "^[0-9A-Fa-f]+" expected_hash "${recorded}")

string(LENGTH "${expected_hash}" expected_length)
if(NOT expected_length EQUAL 64 OR NOT expected_hash MATCHES "^[0-9A-Fa-f]+$")
  message(FATAL_ERROR "invalid golden vector checksum manifest")
endif()
string(TOLOWER "${expected_hash}" expected_hash)

if(NOT actual_hash STREQUAL expected_hash)
  message(FATAL_ERROR "golden vector checksum mismatch: ${actual_hash} != ${expected_hash}")
endif()

message(STATUS "golden vector checksum verified: ${actual_hash}")
