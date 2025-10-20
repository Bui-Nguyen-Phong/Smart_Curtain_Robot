idf_component_register(SRCS "SCR_IDF_Code.c"
                    PRIV_REQUIRES spi_flash
                    PRIV_REQUIRES esp_driver_rmt
                    PRIV_REQUIRES esp_driver_gpio
                    PRIV_REQUIRES esp_timer
                    PRIV_REQUIRES esp_system
                    INCLUDE_DIRS ".")
