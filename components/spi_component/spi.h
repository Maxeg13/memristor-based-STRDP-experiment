//
// Created by nlhr on 2024/2/28.
//

#ifndef WIFI_STA_UDP_SERVER_SPI_H
#define WIFI_STA_UDP_SERVER_SPI_H

void spi_init();

void spi_transfer(
        void* buf,
        const size_t size
);

#endif //WIFI_STA_UDP_SERVER_SPI_H
