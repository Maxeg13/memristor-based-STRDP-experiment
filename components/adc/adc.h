#ifndef WIFI_STA_UDP_SERVER_ADC_H
#define WIFI_STA_UDP_SERVER_ADC_H

#define DIODE_SWITCH  static_cast<gpio_num_t>(16)
#define AMP_SWITCH  static_cast<gpio_num_t>(14)

void adc_task(void*);
float adc_get();
void dac_send(float x);
void adc_init();

#endif //WIFI_STA_UDP_SERVER_ADC_H
