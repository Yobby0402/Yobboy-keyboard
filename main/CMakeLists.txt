idf_component_register(
    SRCS "hc165.c" "tusb_hid_example_main.c" "led.c" "key_bind.c"
    INCLUDE_DIRS "."
                 "./include"
    PRIV_REQUIRES driver
    )
