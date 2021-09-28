idf_component_register(SRCS "http_wifi_conf.c" "wifi_conf.c"
        INCLUDE_DIRS "include"
        EMBED_FILES
                html/http_wifi_conf.html
        REQUIRES http_html_cmn simple_wifi esp_http_server nvs_flash json_str)
