extern "C" {
#include "udp.h"
#include "adc.h"
#include "string.h"
}

#include <string>

//-------------------------------------------------------------
static const char *TAG = "udp";
//-------------------------------------------------------------

// trace
int sigmoid_delay = 50;
float tau_p = 1/0.4;
float F_p = 0.7;
float F_m = 0.7;

//protocol
bool protocol_on = false;
int stimulus_T1 = 80;
int stimulus_T2 = 80;
int stimulus_delay2 = 40;

struct sockaddr_in servaddr, cliaddr;
int sockfd;

void proj_udp_send(char* data, size_t size) {
    sendto(sockfd, data, size, 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}

struct {
    char* p;
    uint8_t left;
} stream;

bool stream_trim() {
    while(stream.left && (*stream.p == ' ') ) {
        stream.p++;
        stream.left--;
    }
    if(stream.left==0) return false;
    return true;
}

bool stream_parse_word(char* word) {
    uint8_t offset = strlen(word);
    if(memcmp(stream.p, word, offset)==0) {
        stream.p += offset;
        stream.left-= offset;
        return true;
    }
    return false;
}

bool stream_next() {
    while(stream.left && (*stream.p != ' ') ) {
        stream.p++;
        stream.left--;
    }
    if(stream.left==0) return false;

    return stream_trim();
}

int stream_parse_int() {
    stream_trim();
    int x = atoi(stream.p);
    stream_next();
    return x;
}

float stream_parse_float() {
    stream_trim();
    float x = atof(stream.p);
    stream_next();
    return x;
}

void udp_task(void *pvParameters)
{
    static char buf[140] = {};
    static char str[140];

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
        while(1) {
            size_t rec_size = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &cliaddr,
                                       &client_addr_len);

            stream.p = buf;
            stream.left = rec_size;

            if (stream_parse_word("help")) {
                const char *help = "___________________________\n"
                                   "  Memristive STDRP board\n"
                                   "usage:\n"
                                   "    trace set <sigmoid delay (int ms)> <tau plus (float ms)>"
                                   "<F plus (float volts)> "
                                   "<F minus (float volts)>\n"
                                   ""
                                   "    protocol on\t\t\t\tstart stimuli\n"
                                   "    <Enter>\t\t\t\tstop stimuli\n"
                                   "    protocol set <stimulus_T1 (int ms)> <stimulus_T2 (int ms)> "
                                   "<stimulus_delay2 (int ms)>\n"
                                   "___________________________\n# ";
                sendto(sockfd, help, strlen(help), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if (stream_parse_word("trace set")) {
                sigmoid_delay = stream_parse_int();
                tau_p = stream_parse_float();
                F_p = stream_parse_float();
                F_m = stream_parse_float();

                snprintf(str, sizeof(str), "    settings done:\n"
                                             "\tsigmoid delay\t%d\n"
                                             "\ttau plus\t%4.2f\n"
                                             "\tF plus\t\t%4.2f\n"
                                             "\tF minus\t\t%4.2f\n# ",
                         sigmoid_delay, tau_p, F_p, F_m);
                sendto(sockfd, str, sizeof(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("protocol set")) { 
                stimulus_T1 = stream_parse_int();
                stimulus_T2 = stream_parse_int();
                stimulus_delay2 = stream_parse_int();

                snprintf(str, sizeof(str), "    settings done:\n"
                                             "\tstimulus T1\t%d\n"
                                             "\tstimulus T2\t%d\n"
                                             "\tstimulus 2 delay\t%d\n# ",
                         stimulus_T1, stimulus_T2, stimulus_delay2);
                sendto(sockfd, str, sizeof(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("protocol on")) {
                protocol_on = true;
            } else if(stream_parse_word("dac set")) {
                float x = stream_parse_float();
                dac_send(x);
                std::string s = "\n# ";
                sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("amp switch")) {
                int state = stream_parse_int();
                gpio_set_level(AMP_SWITCH, state);
                std::string s = "\n# ";
                sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            }
            else if(stream_parse_word("adc get")) {
                int x = adc_get();
                char str[8];
                auto sf = std::to_string(x);
                sf+="\n# ";
                sendto(sockfd, sf.c_str(), strlen(sf.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream.left == 1) {
                protocol_on = false;
                char* str = "protocol off\n# ";
                sendto(sockfd, str, strlen(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else {
                const char* help =  "failed to parse the incoming packet\n# ";
                sendto(sockfd, help, strlen(help), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            }

            // rec from ncat and parse
//            while(1) {
//                int x = adc_get();
//                snprintf(str, sizeof(str), "%6d\n", x);
//                sendto(sockfd, str, 10, 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
//            }
        }
        if (sockfd != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sockfd, 0);
            close(sockfd);
        }
    }
    vTaskDelete(NULL);
}
//-------------------------------------------------------------
