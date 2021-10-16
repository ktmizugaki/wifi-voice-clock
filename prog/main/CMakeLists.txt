idf_component_register(SRCS "app_main.c" "app_mode_suspend.c"
                "app_mode_initial.c" "app_mode_initialsync.c" "app_mode_clock.c" "app_mode_settings.c"
                "app_display_clock.c" "app_display_softap.c"
                "misc.c"
                "util/app_wifi.c" "util/app_display.c" "util/app_clock.c" "util/app_switches.c" "util/app_event.c"
                "util/power.c"
        PRIV_INCLUDE_DIRS "util"
        EMBED_FILES
                gen/nvskey.dat
                gen/font_shinonome14.fnt
                gen/font_shinonome12.fnt
                html/index.html
        REQUIRES ssd1306 gfx udplog vcc switches clock lan_manager
                http_clock_conf http_alarm_conf http_wifi_conf simple_wifi
                esp_http_server nvs_flash)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
