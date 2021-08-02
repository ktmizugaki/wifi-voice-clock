idf_component_register(SRCS "app_main.c" "app_clock.c"
        EMBED_FILES
                gen/nvskey.dat
        REQUIRES clock lan_manager http_wifi_conf simple_wifi esp_http nvs_flash)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
