idf_component_register(SRCS "clock.c" "clock_debug.c"
                "clock_sntp.c"
        INCLUDE_DIRS "include"
        REQUIRES lwip)
