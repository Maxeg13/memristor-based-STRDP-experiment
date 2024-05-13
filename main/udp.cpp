extern "C" {
#include "udp.h"
#include "adc.h"
#include "string.h"
}

#include <string>

//-------------------------------------------------------------
static const char *TAG = "udp";
//-------------------------------------------------------------
//////////////////////////

// common
float adc_zero = 1.215;
float adc_to_current = 1;

// trace
int sigmoid_delay = 50;
float tau_p = 1/0.4;
float F_p = 0.7;
float F_m = 0.7;

//protocol
state_t proj_state = IDLE;
float prot_ampl = 2;
bool prot_with_neurons = true;
int prot_adc_log_presc = 1;

//protocol stimuli
int stimulus_T1 = 80;
float stimulusA1 = 0.1;
int stimulus_T2 = 80;
float stimulusA2 = 0.1;
int stimulus_delay2 = 40;

// VAC
float vac_step   = 0.04;
float vac_finish = 2 * 2 * 3.3 / vac_step;
float vac_a = 0.5;
float vac_b = -0.5;

///////
float stimulus_t1 = 0;
float stimulus_t2 = 0;

bool protocol_once = false;

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

bool stream_next() {
    while(stream.left && (*stream.p != ' ') ) {
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
    static char buf[150] = {};
    static char str[150];

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

        servaddr.sin_family    = AF_INET; // IPv4
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(CONFIG_SERVER_PORT);

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
                                   "    trace set <sigmoid delay (uint ms)> <tau plus (float ms)> "
                                   "<F plus (float volts)>  <F minus (float volts)>\n"
                                   "    stimuli set <stimulus_T1 (uint ms)> <stimulus_A1 (float)> <stimulus_T2 (uint ms)> "
                                   "<stimulus_A2 (float)> <stimulus_delay2 (uint ms)>\n"
                                   "    protocol set <amplification (float)> <with neurons [1 | 0]> <adc log prescaller (uint)>\n"
                                   "    adc common set <adc zero (float Volts)> <adc_to_current (float Amps/Volts)>\n"
                                   "    [protocol [on | off]] | <Enter>\t\t\t\t- start/stop stimuli\n\n"
                                   ""
                                   "tests usage:\n"
                                   "    dac set <value in Volts (float)>\n"
                                   "    adc get\t\t\t\t\t\t\t- get current adc value in Volts (float)\n"
                                   "    vac [A|B] [+= | -=] <value in Volts (float)> \t\t- increment/decrement VAC limits\n"
                                   "    vac [on | off]\t\t\t\t\t\t- activate/deactivate VAC measurement\n"
                                   "    [amp | diode] switch [0 | 1] \t\t\t\t- implementation of bidirectional keys\n"
                                   "    want a spider\n# ";
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
                sendto(sockfd, str, strlen(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("stimuli set")) {
                stimulus_T1 = stream_parse_int();
                stimulusA1 = stream_parse_float();
                stimulus_T2 = stream_parse_int();
                stimulusA2 = stream_parse_float();
                stimulus_delay2 = stream_parse_int();

                snprintf(str, sizeof(str), "    settings done:\n"
                                             "\tstimulus T1\t%d\n"
                                             "\tstimulus A1\t%4.2f\n"
                                             "\tstimulus T2\t%d\n"
                                             "\tstimulus A2\t%4.2f\n"
                                             "\tstimulus 2 delay\t%d\n# ",
                         stimulus_T1, stimulusA1, stimulus_T2, stimulusA2, stimulus_delay2);
                sendto(sockfd, str, strlen(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("protocol")) {
                if(stream_parse_word("set"))
                {
                    prot_ampl = stream_parse_float();
                    prot_with_neurons = stream_parse_int();
                    prot_adc_log_presc = stream_parse_int();

                    snprintf(str, sizeof(str), "    settings done:\n"
                                               "\tprot amp\t\t%4.2f\n"
                                               "\tprot with neurons\t%d\n"
                                               "\tprot adc log prescaller\t%d\n# ",
                             prot_ampl, prot_with_neurons, prot_adc_log_presc);
                    sendto(sockfd, str, strlen(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                } else if(stream_parse_word("on")) {
                    stimulus_t1 = 0;
                    stimulus_t2 = stimulus_delay2;
                    protocol_once = true;
                    proj_state = PROTOCOL;

                    std::string s = "protocol started\n# ";
                    sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                } else if(stream_parse_word("off")) {
                    proj_state = IDLE;
                    char* str = "idle state\n# ";
                    sendto(sockfd, str, strlen(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                }
            }
            else if(stream_parse_word("adc common set")) {
                if(stream_parse_word("common")&& stream_parse_word("set"))
                {
                    adc_zero = stream_parse_float();
                    adc_to_current = stream_parse_float();

                    snprintf(str, sizeof(str), "    settings done:\n"
                                               "\tadc zero\t\t%4.2f\n"
                                               "\tadc_to_current\t%4.2f\n# ",
                             adc_zero, adc_to_current);
                    sendto(sockfd, str, sizeof(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                } else if(stream_parse_word("get")) {
                    float x = adc_get();
                    auto sf = std::to_string(x);
                    sf+="\n# ";
                    sendto(sockfd, sf.c_str(), strlen(sf.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                }
            } else if(stream_parse_word("dac") && stream_parse_word("set")) {
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

                } else if(stream_parse_word("on")) {
                    proj_state = VAC;
                    std::string s = "VAC measurement started\n# ";
                    sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
                } else if(stream_parse_word("off")) {
                    proj_state = IDLE;
                    std::string s = "idle state\n# ";
                    sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));

                }

                vac_finish = 2 * (vac_a - vac_b) / vac_step;
            }
            else if(stream_parse_word("amp switch")) {
                bool state = stream_parse_int();
                gpio_set_level(AMP_SWITCH, !state);
                std::string s = "# ";
                sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("diode switch")) {
                bool state = stream_parse_int();
                gpio_set_level(DIODE_SWITCH, !state);
                std::string s = "# ";
                sendto(sockfd, s.c_str(), strlen(s.c_str()), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream.left == 1) {
                proj_state = IDLE;
                char* str = "idle state\n# ";
                sendto(sockfd, str, strlen(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            } else if(stream_parse_word("want a spider")) {
                char* str = "\n"
                            "{\\__/}\n"
                            "( ' . ')\n"
                            "/ > * hotite pavuka?\n"
                            "\n"
                            " {\\__/}\n"
                            "( ' ^ ')\n"
                            "*<   \\ fig vam, a ne pavuk!\n"
                            "\n"
                            "{\\__/}\n"
                            "(._.  )\n"
                            " | < \\ stop, a ti gde?\n"
                            "\n"
                            "*\\__/}\n"
                            "( ' 0')\n"
                            "/>  |  pomogite!\n\n# ";
                sendto(sockfd, str, strlen(str), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
            }
            else {
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
