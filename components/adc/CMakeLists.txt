idf_component_register(SRCS "read_write_component.c"
                    INCLUDE_DIRS "."
#        "C:/Espressif/frameworks/esp-idf-v5.1.1/components/driver/gpio"
#        "C:/Espressif/frameworks/esp-idf-v5.1.1/components/driver/gpio/include"
        "C:/Espressif/frameworks/esp-idf-v5.1.1/components/esp_adc/include"
        "C:/Espressif/frameworks/esp-idf-v5.1.1/components/esp_adc/include/esp_adc"
        "C:/Espressif/frameworks/esp-idf-v5.1.1/components/esp_adc/${IDF_TARGET}/include"
                    REQUIRES driver)
