idf_component_register(SRCS "http_alarm_conf.c" "alarm.c"
        INCLUDE_DIRS "include"
        EMBED_FILES
                html/http_alarm_conf.html
        REQUIRES http_html_cmn esp_http_server nvs_flash json_str)
