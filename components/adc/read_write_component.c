/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "../spi_component/spi.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali_scheme.h"

#define SYNC 17

QueueHandle_t queue;

const static char *TAG = "";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_4
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

#include "math.h"
float trace(float x) {
    const float alpha = 3.5;
    return exp(-alpha * x) - 1./(1 + exp(-alpha * (x - 2.7)));
}

#define SYNC 17

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_11

static int adc_raw;
static int voltage;
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

int adc_get() {
    int val;
    xQueueReceive(queue, &val, 1000);
    return val;
}

//0x7FF
void dac_send(uint16_t x) {
    static uint8_t mas[2];
    mas[0] = x>>8;
    mas[1] = x;

    gpio_set_level(SYNC, 0);
    spi_transfer(&mas[0], 2);
//    esp_rom_delay_us(1);
    gpio_set_level(SYNC, 1);
}

void adc_task(void*)
{
    spi_init();

    gpio_reset_pin(SYNC);
    gpio_set_direction(SYNC, GPIO_MODE_OUTPUT);

    queue = xQueueCreate(4, sizeof(uint32_t));

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN0, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw));

//        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, adc_raw);
        if (do_calibration1_chan0) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw, &voltage));
//            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, voltage);
        }
        xQueueSend(queue, &voltage,0);

//        esp_rom_delay_us(5);
        static float t = 0;
        t += 0.1;
        if(t>15) t=0;
        static uint16_t dac_val=0;
//        dac_val+=40;
//        if(dac_val > (dac_val&0x7FF)) dac_val = 0;
//
//        esp_timer_get_time
        dac_val = (1. + trace(t)) * DAC_RANGE/2;
        dac_send(dac_val);

//        vTaskDelay(pdMS_TO_TICKS(20));
    }

    //Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration1_chan0) {
        example_adc_calibration_deinit(adc1_cali_chan0_handle);
    }

#if EXAMPLE_USE_ADC2
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc2_handle));
    if (do_calibration2) {
        example_adc_calibration_deinit(adc2_cali_handle);
    }
#endif //#if EXAMPLE_USE_ADC2
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
