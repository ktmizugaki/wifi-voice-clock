idf_component_register(SRCS "app_main.c" "app_mode_suspend.c"
                "app_mode_initial.c" "app_mode_initialsync.c" "app_mode_clock.c" "app_mode_settings.c"
                "app_display_clock.c" "app_display_softap.c"
                "misc.c" "voice.c"
                "util/app_wifi.c" "util/app_display.c" "util/app_clock.c" "util/app_switches.c" "util/app_event.c"
                "util/sound.c" "util/power.c" "util/storage.c"
                "lib/liblistview.c" "lib/libmenu.c"
                "gen/batt.bmp.c"
        PRIV_INCLUDE_DIRS "util" "lib"
        EMBED_FILES
                data/alarm.wav
                gen/nvskey.dat
                gen/font_shinonome14.fnt
                gen/font_shinonome12.fnt
                html/index.html
        REQUIRES ssd1306 gfx udplog vcc audio switches
                clock lan_manager
                http_firmware http_clock_conf http_alarm_conf http_wifi_conf simple_wifi
                esp_http_server spiffs nvs_flash)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)


### add spiffs targets
set(spiffs_dir "spiffs")
set(spiffs_partition "storage")
set(spiffs_path ${PROJECT_DIR}/${spiffs_dir})

spiffs_create_partition_image(${spiffs_partition} ${spiffs_path})
# add target to flash spiffs. this may not be needed for newer version of esp-idf
# modified version of esptool_py_custom_target from esptool_py/project_include.cmake
idf_component_get_property(esptool_py_dir esptool_py COMPONENT_DIR)
add_custom_target(storage-flash DEPENDS "spiffs_storage_bin"
    COMMAND ${CMAKE_COMMAND}
    -D IDF_PATH="${IDF_PATH}"
    -D ESPTOOLPY="${ESPTOOLPY}"
    -D ESPTOOL_ARGS="write_flash;@flash_storage_args"
    -D WORKING_DIRECTORY="${build_dir}"
    -P run_esptool.cmake
    WORKING_DIRECTORY "${esptool_py_dir}"
    USES_TERMINAL
    )
