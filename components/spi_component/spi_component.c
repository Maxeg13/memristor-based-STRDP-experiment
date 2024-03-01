#include "esp_err.h"
#include "include/driver/spi_master.h"
#include "include/driver/spi_common.h"
#include "spi.h"

#define SPI_INSTANCE SPI2_HOST

static spi_device_handle_t s_handle;

static spi_device_interface_config_t s_devcfg={
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL,
};

/**/
#include "esp_log.h"
void spi_init()
{
    spi_bus_config_t buscfg={
            .miso_io_num = 26,
            .mosi_io_num = 25,
            .sclk_io_num = 27,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
    };

    ESP_LOGI("", "spi_bus_initialize try");

    esp_err_t ret;
    ret = spi_bus_initialize(SPI_INSTANCE, &buscfg, SPI_DMA_CH_AUTO);

    ESP_LOGI("", "spi_bus_initialize: %d", ret);

    s_devcfg.clock_speed_hz = 10000000;
    spi_bus_add_device(SPI_INSTANCE, &s_devcfg, &s_handle);
}

/**/

void spi_transfer(
        void* buf,
        const size_t size)
{
    static uint16_t x;
    spi_transaction_t desc = {
            .tx_buffer = buf,
            .length = size*8,
            .rx_buffer = NULL,
            .rxlength = 0
    };

//    ESP_LOGI("", "rec: %d", x);

    ESP_ERROR_CHECK(spi_device_transmit(s_handle, &desc));
}