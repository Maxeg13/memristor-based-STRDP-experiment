#include "udp.h"
#include "adc.h"

//-------------------------------------------------------------
static const char *TAG = "udp";
//-------------------------------------------------------------
typedef struct
{
    unsigned char y_pos;
    unsigned char x_pos;
    char *str;
} qLCDData;
//------------------------------------------------
// lcd smth
//------------------------------------------------
//void vLCDTask(void* arg)
//{
//  BaseType_t xStatus;
//  qLCDData xReceivedData;
//  for(;;) {
//    xStatus = xQueueReceive(lcd_string_queue, &xReceivedData, 10000 /portTICK_PERIOD_MS);
//    if (xStatus == pdPASS)
//    {
//      LCD_SetPos(xReceivedData.x_pos,xReceivedData.y_pos);
//      LCD_String(xReceivedData.str);
//    }
//  }
//}
//------------------------------------------------
void udp_task(void *pvParameters)
{
    int sockfd;
    char buf[10] = {};
    char str1[10];

    struct sockaddr_in servaddr, cliaddr;

    while(1)
    {
        ESP_LOGI(TAG, "Create socket...\n");
        if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0 ) {
            ESP_LOGE(TAG, "socket not created\n");
            break;
        }
        ESP_LOGI(TAG, "Socket created");
        memset(&servaddr, 0, sizeof(servaddr));
        memset(&cliaddr, 0, sizeof(cliaddr));
        uint32_t client_addr_len = sizeof(cliaddr);
        //���������� ���������� � �������
        servaddr.sin_family    = AF_INET; // IPv4
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(CONFIG_SERVER_PORT);
        //������ ����� � ������� �������
        if (bind(sockfd, (const struct sockaddr *)&servaddr,  sizeof(struct sockaddr_in)) < 0 )
        {
            ESP_LOGI(TAG,"socket not binded");
            shutdown(sockfd, 0);
            close(sockfd);
            break;
        }
        ESP_LOGI(TAG,"socket binded");
        while(1)
        {
            size_t rec_size = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr,
                     &client_addr_len);
            // rec from ncat and parse
            while(1) {
//                static int x = 32000;
                int x = adc_get();
                snprintf(str1, sizeof(str1), "%6d\n", x);
//      *(short*)buf = 32767 - *(short*)buf;
                sendto(sockfd, str1, 10, 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            }
        }
        if (sockfd != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sockfd, 0);
            close(sockfd);
        }
    }
// lcd smth
// lcd smth
    vTaskDelete(NULL);
}
//-------------------------------------------------------------
