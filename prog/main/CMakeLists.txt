idf_component_register(SRCS "app_main.c"
        REQUIRES nvs_flash)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
