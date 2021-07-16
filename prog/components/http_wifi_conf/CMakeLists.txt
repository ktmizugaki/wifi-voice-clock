idf_component_register(SRCS "http_wifi_conf.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_http nvs_flash)
