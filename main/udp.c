#include "udp.h"
#include "adc.h"
#include "string.h"

//-------------------------------------------------------------
static const char *TAG = "udp";
//-------------------------------------------------------------
typedef struct
{
    unsigned char y_pos;
    unsigned char x_pos;
    char *str;
} qLCDData;

void udp_task(void *pvParameters)
{
    int sockfd;
    char buf[30] = {};
    char str1[30];

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

            if(memcmp(buf, "help", 4) == 0) {
                const char* help = "\tMemristive STDRP board\n"
                                   "parameters available:\t\n"
                                   "\tsigmoid delay\n";
                sendto(sockfd, help, strlen(help), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(memcmp(buf, "sigmoid delay", 4) == 0) {
                const char* sd = "\tEnter new sigmoid delay value\n";
                sendto(sockfd, sd, strlen(sd), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));

            }

            // rec from ncat and parse
//            while(1) {
//                int x = adc_get();
//                snprintf(str1, sizeof(str1), "%6d\n", x);
//                sendto(sockfd, str1, 10, 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
//            }
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
