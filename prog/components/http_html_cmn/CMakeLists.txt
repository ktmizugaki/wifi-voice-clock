idf_component_register(SRCS "http_html_cmn.c"
                "queryparser.c"
        INCLUDE_DIRS "include"
        EMBED_FILES
                html/cmn.js
                html/cmn.css
        REQUIRES esp_http_server nvs_flash)
