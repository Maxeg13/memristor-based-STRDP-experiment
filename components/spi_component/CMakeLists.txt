set(SRC_NAME spi_component)

idf_component_register(
        SRCS  ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_NAME}.c     # list the source files of this component
        INCLUDE_DIRS   "C:/Espressif/frameworks/esp-idf-v5.1.1/components/driver/spi"       # optional, add here public include directories
                        "C:/Espressif/frameworks/esp-idf-v5.1.1/components/driver/spi/include"
        PRIV_INCLUDE_DIRS   # optional, add here private include directories
        REQUIRES            # optional, list the public requirements (component names)
        PRIV_REQUIRES       # optional, list the private requirements
)