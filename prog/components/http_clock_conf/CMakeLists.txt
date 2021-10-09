idf_component_register(SRCS "http_clock_conf.c" "clock_conf.c"
        INCLUDE_DIRS "include"
        EMBED_FILES
                html/http_clock_conf.html
        REQUIRES http_html_cmn esp_http_server nvs_flash json_str)
