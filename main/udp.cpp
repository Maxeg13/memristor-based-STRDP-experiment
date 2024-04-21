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
state_t proj_state = IDLE;
int stimulus_T1 = 80;
int stimulus_T2 = 80;
int stimulus_delay2 = 40;

// VAC
float vac_a = 0.5;
float vac_b = -0.5;

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
    stream_trim();
    uint8_t offset = strlen(word);
    if(memcmp(stream.p, word, offset)==0) {
        stream.p += offset;
        stream.left-= offset;

        return true;
    }
    return false;
}

int stream_parse_int() {
    stream_trim();
    int x = atoi(stream.p);
    return x;
}

float stream_parse_float() {
    stream_trim();
    float x = atof(stream.p);
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
                const char *help = ""
                                   "  Memristive STRDP board 1.0\n"
                                   "usage:\n"
                                   "    trace set <sigmoid delay (int ms)> <tau plus (float ms)> "
                                   "<F plus (float volts)> "
                                   "<F minus (float volts)>\n"
                                   ""
                                   "    protocol set <stimulus_T1 (int ms)> <stimulus_T2 (int ms)> "
                                   "<stimulus_delay2 (int ms)>\n"
                                   "    [protocol [on | off]] | <Enter>\t\t\t\t- start/stop stimuli\n\n"
                                   ""
                                   "tests usage:\n"
                                   "    dac set <value in Volts (float)>\n"
                                   "    adc get\t\t\t\t\t\t\t- get current adc value in Volts (float)\n"
                                   "    vac [A|B] [+= | -=] <value in Volts (float)> \t\t- increment/decrement VAC limits\n"
                                   "    vac [on | off]\t\t\t\t\t\t- activate/deactivate VAC measurement\n# ";
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
                proj_state = PROTOCOL;
                std::string s = "protocol started\n# ";
                sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("dac set")) {
                float x = stream_parse_float();
                dac_send(x);
                std::string s = "# ";
                sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("vac")) {
                if(stream_parse_word("A")) {
                    if (stream_parse_word("+=")) {
                        vac_a += stream_parse_float();
                    } else if (stream_parse_word("-=")) {
                        vac_a -= stream_parse_float();
                    }
                    std::string s = "vac_a = ";
                    s += std::to_string(vac_a);
                    s += "\n# ";
                    sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                } else if(stream_parse_word("B")) {
                    if(stream_parse_word("+=")) {
                        vac_b += stream_parse_float();
                    } else if(stream_parse_word("-=")) {
                        vac_b -= stream_parse_float();
                    }
                    std::string s = "vac_b = ";
                    s+=std::to_string(vac_b);
                    s+="\n# ";
                    sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));

                } else if("on") {
                    proj_state = VAC;
                    std::string s = "VAC measurement started\n# ";
                    sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                } else if("off") {
                    proj_state = IDLE;
                    std::string s = "idle state\n# ";
                    sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));

                }
            }
            else if(stream_parse_word("amp switch")) {
                bool state = stream_parse_int();
                gpio_set_level(AMP_SWITCH, !state);
                std::string s = "# ";
                sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            }
            else if(stream_parse_word("adc get")) {
                float x = adc_get();
                auto sf = std::to_string(x);
                sf+="\n# ";
                sendto(sockfd, sf.c_str(), strlen(sf.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("protocol off") || (stream.left == 1)) {
                proj_state = IDLE;
                char* str = "idle state\n# ";
                sendto(sockfd, str, strlen(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else {
                const char* help =  "failed to parse the incoming packet\n# ";
                sendto(sockfd, help, strlen(help), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            }
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
