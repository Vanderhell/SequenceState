idf_component_register(
  SRCS
    "sequence_state_hw_main.c"
    "../../../core/src/sequence_state.c"
    "../../../candidates/candidates.c"
    "../../../candidates/affine_h.c"
    "../../../candidates/h2.c"
  INCLUDE_DIRS
    "../../../core/include"
    "../../../candidates"
  PRIV_REQUIRES esp_timer esp_hw_support esp_psram spi_flash heap freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic)
