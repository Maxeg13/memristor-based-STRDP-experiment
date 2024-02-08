#include "udp.h"
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
// lcd smth
  int sockfd;
  char buf[10] = {};
// lcd smth
  char str1[10];
// lcd smth
// lcd smth
  struct sockaddr_in servaddr, cliaddr;
// lcd smth
// lcd smth
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
      recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr,
            &client_addr_len);

      static uint8_t x = '0';
      snprintf(str1, sizeof(str1), "%6f", *(short*)buf);
      *(short*)buf = 32767 - *(short*)buf;


      sendto(sockfd, &x, 1,  0, (struct sockaddr*) &cliaddr,  sizeof(cliaddr));
        x++;
      // lcd smth
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
