idf_component_register(SRCS "http_firmware.c" "ota_helper.c"
                "spiffs_upd.c"
        INCLUDE_DIRS "include"
        EMBED_FILES
                html/http_firmware.html
        REQUIRES http_html_cmn app_update spiffs esp_http_server json_str)
