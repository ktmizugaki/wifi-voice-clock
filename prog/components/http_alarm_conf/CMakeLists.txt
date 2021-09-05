idf_component_register(SRCS "http_alarm_conf.c" "alarm.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_http nvs_flash json_str)
