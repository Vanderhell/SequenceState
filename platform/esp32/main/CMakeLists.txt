idf_component_register(
  SRCS
    "sequence_state_release_main.c"
    "../../../core/src/trace.c"
    "../../../core/src/compose.c"
    "../../../core/src/range.c"
  INCLUDE_DIRS
    "../../../include"
  PRIV_REQUIRES esp_timer esp_hw_support esp_psram spi_flash heap freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wpedantic)
