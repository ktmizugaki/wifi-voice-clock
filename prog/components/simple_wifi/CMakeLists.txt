idf_component_register(SRCS "simple_wifi.c" "simple_sta.c" "simple_softap.c"
        INCLUDE_DIRS "include"
        REQUIRES nvs_flash)
