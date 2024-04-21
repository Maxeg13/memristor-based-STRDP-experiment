#ifndef MAIN_UDP_H_
#define MAIN_UDP_H_
//-------------------------------------------------------------
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "lcd2004.h"

typedef enum
{
    IDLE = 0,
    PROTOCOL,
    VAC
}state_t;
//-------------------------------------------------------------
void udp_task(void *pvParameters);
void proj_udp_send(char* data, size_t size);
//-------------------------------------------------------------
#endif /* MAIN_UDP_H_ */
