/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern "C"{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
//#include "freertos/queue.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "../spi_component/spi.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_cali_scheme.h"
#include "math.h"
#include "adc.h"
#include "../../main/udp.h"
}

#define LED_STATE     static_cast<gpio_num_t>(21)
#define SYNC        static_cast<gpio_num_t>(17)
#define DT  0.5

const static char *TAG = "";

class Neuron {
    float v;
    float u;
    static const float a;
    static const float b;
    static const float c;
    static const float d;
public:
    Neuron():v(c), u(0){}
    bool eval(float I) {
        float _v = v;
        v += DT*(0.04*v*v + 5*v + 140 - u + I);
        u += DT*a*(b*_v - u);
        if(v>30) {
            v = c;
            u += d;
            return true;
        } else {
            return false;
        }
    }
};

const float Neuron::a = 0.02;
const float Neuron::b = 0.2;
const float Neuron::c = -65;
const float Neuron::d = 8;

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC1_CHAN4          ADC_CHANNEL_4
#define EXAMPLE_ADC1_CHAN1          ADC_CHANNEL_5
#else
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_2
#define EXAMPLE_ADC1_CHAN1          ADC_CHANNEL_3
#endif

#if (SOC_ADC_PERIPH_NUM >= 2) && !CONFIG_IDF_TARGET_ESP32C3
/**
 * On ESP32C3, ADC2 is no longer supported, due to its HW limitation.
 * Search for errata on espressif website for more details.
 */
#define EXAMPLE_USE_ADC2            0
#endif

#define DAC_RANGE 0x7FF

// common
extern float adc_zero;
extern float adc_to_current;

// trace
extern int sigmoid_delay;
extern float tau_p;
extern float F_p;
extern float F_m;

//protocol
extern state_t proj_state;
extern float prot_ampl;
extern bool prot_with_neurons;
extern int prot_adc_log_presc;

//protocol stimuli
extern int stimulus_T2;
extern float stimulusA1;
extern int stimulus_T1;
extern float stimulusA2;
extern int stimulus_delay2;

// VAC
extern float vac_a;
extern float vac_b;
size_t vac_ctr = 0;
extern float vac_step;
extern float vac_finish;
float vac_x = 0;
bool vac_up = true;

//////////////////////////

extern float stimulus_t1;
extern float stimulus_t2;

extern bool protocol_once;

float trace(float t) {
    return F_p * exp(-t/tau_p) - F_m/(1 + exp(-(t - sigmoid_delay)/tau_p));
}

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_11

static int adc_raw;
static int voltage;
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

// means after amplifier, memristor in
void dac_send(float x) {
    #define kOhm

    static const float tovolts = 10 kOhm /4.7 kOhm * 3.3/DAC_RANGE;
    static const float todac = 1/tovolts;

    uint16_t xi = x*todac + DAC_RANGE/2;

    static uint8_t mas[2];
    mas[0] = xi>>8;
    mas[1] = xi;

    gpio_set_level(SYNC, 0);
    spi_transfer(&mas[0], 2);
    gpio_set_level(SYNC, 1);
}

void adc_init() {
    gpio_reset_pin(LED_STATE);
    gpio_set_direction(LED_STATE, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_STATE, 1);
}

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc1_cali_chan4_handle = NULL;

void dac_init() {
    example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN4, EXAMPLE_ADC_ATTEN, &adc1_cali_chan4_handle);
}

extern bool wifi_started;

void adc_task(void*)
{
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 2000, // 1 tick = 0.5us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    //////////////////

    spi_init();

    gpio_reset_pin(DIODE_SWITCH);
    gpio_set_direction(DIODE_SWITCH, GPIO_MODE_OUTPUT);
    gpio_set_level(DIODE_SWITCH, 1);
    gpio_reset_pin(AMP_SWITCH);
    gpio_set_direction(AMP_SWITCH, GPIO_MODE_OUTPUT);
    gpio_set_level(AMP_SWITCH, 1);
    gpio_reset_pin(SYNC);
    gpio_set_direction(SYNC, GPIO_MODE_OUTPUT);

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
            .atten = EXAMPLE_ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN4, &config));

    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN4, EXAMPLE_ADC_ATTEN, &adc1_cali_chan4_handle);
    dac_send(0);

    Neuron n1{};
    Neuron n2{};

    while(!wifi_started);

    while (1) {
        static uint64_t main_count;
        static uint64_t test_count;
        static uint64_t now_count;

        ESP_ERROR_CHECK(gptimer_get_raw_count(gptimer, &now_count));
        if(!protocol_once) {
            protocol_once = true;
            main_count = now_count;
        }

//        // tests
        if(now_count >= test_count+2000) {
//            ESP_LOGI(TAG, "change lvl");
            test_count = now_count/2000*2000;
            static uint32_t lvl=0;
            lvl^=1;
            gpio_set_level(LED_STATE, lvl);

//            if (do_calibration1_chan0) {
//                ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan4_handle, adc_raw, &voltage));
//            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN4, voltage);
//            }
        }

        // main
        size_t count_diff = now_count - main_count;
        for (int step = 0; step < count_diff; step++) {
            main_count += DT;
            switch (proj_state) {
                case PROTOCOL: {
                    //            ESP_LOGI(TAG, "test_count: %d", (int)now_count);
                    static float trace_t = 0;

                    float stimulus1 = 0;
                    float stimulus2 = 0;

                    trace_t += DT;
                    stimulus_t1 += DT;
                    stimulus_t2 += DT;

                    if (stimulus_t1 > stimulus_T1) {
                        stimulus_t1 = 0;
                        stimulus1 = stimulusA1;
                    }

                    if(prot_with_neurons) {
                        if (n1.eval(stimulus1)) {
                            trace_t = 0;
                        }
                    } else {
                        trace_t = stimulus_t1;
                    }

                    dac_send(trace(trace_t));
///////////////////////////////////////

                    float I = 0;
                    I = (adc_get() - adc_zero) * adc_to_current;

                    if (2 > stimulus_T2) {
                        stimulus_t2 = 0;
                        stimulus2 = stimulusA2;
                    }

                    I += stimulus2;

                    if(prot_with_neurons) {
                        if (n2.eval(I)) {
                            dac_send(trace(trace_t) * prot_ampl);
                            esp_rom_delay_us(5);
                            dac_send(trace(trace_t));
                        }
                    } else {
                        if(!stimulus_t2) {
                            dac_send(trace(trace_t) * prot_ampl);
                            esp_rom_delay_us(5);
                            dac_send(trace(trace_t));
                        }
                    }
                }
                    break;

                case VAC: {
                    static int presc = 0;
                    presc++;
                    presc%=3;

                    if(presc == 0) {
                        vac_ctr++;
                        if (vac_up) {
                            vac_x += vac_step;
                            if (vac_x > vac_a) {
                                vac_up = false;
                            }
                        } else {
                            vac_x -= vac_step;
                            if (vac_x < vac_b) {
                                vac_up = true;
                            }
                        }

                        dac_send(vac_x);
                        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN4, &adc_raw));
                        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan4_handle, adc_raw, &voltage));

                        static char str[230] = {0};
                        static int send_ctr = 0;
                        send_ctr++;

                        snprintf(str+strlen(str), sizeof(str) - strlen(str) - 1, "[%4.2f, %4.2f], ",
                                 vac_x, voltage / 1000. - adc_zero);

                        if (send_ctr > 6) {
                            send_ctr = 0;
                            snprintf(str+strlen(str), 2, "\n");
                            proj_udp_send(str, strlen(str));
                            str[0] = 0;
                        }

                        if (vac_ctr > vac_finish) {
                            send_ctr = 0;
                            vac_ctr = 0;
                            vac_up = true;
                            vac_x = 0;
                            str[0] = 0;
                            proj_state = IDLE;
                            proj_udp_send("\n# ", 3);
                        }
                    }
                }
                    break;

                case IDLE:
                    break;
            }
        }
    }

//        vTaskDelay(pdMS_TO_TICKS(20));
//    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
//    if (do_calibration1_chan0) {
//        example_adc_calibration_deinit(adc1_cali_chan4_handle);
//    }

#if EXAMPLE_USE_ADC2
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc2_handle));
    if (do_calibration2) {
        example_adc_calibration_deinit(adc2_cali_handle);
    }
#endif //#if EXAMPLE_USE_ADC2
}

float adc_get() {
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN4, &adc_raw));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan4_handle, adc_raw, &voltage));
    return voltage/1000.;
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
