#include "main.h"
#include "adc.h"
//-------------------------------------------------------------
static const char *TAG = "main";
//-------------------------------------------------------------
void app_main(void)
{
  gpio_reset_pin(CONFIG_LED_GPIO);
  gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(CONFIG_LED_GPIO, 0);
  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ret = nvs_flash_erase();
    ESP_LOGI(TAG, "nvs_flash_erase: 0x%04x", ret);
    ret = nvs_flash_init();
    ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
  }
  ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
  ret = esp_netif_init();
  ESP_LOGI(TAG, "esp_netif_init: %d", ret);
  ret = esp_event_loop_create_default();
  ESP_LOGI(TAG, "esp_event_loop_create_default: %d", ret);
//  ret = i2c_ini();
//  ESP_LOGI(TAG, "i2c_ini: %d", ret);
//  LCD_ini();
  ret = wifi_init_sta();
  ESP_LOGI(TAG, "wifi_init_sta: %d", ret);
  xTaskCreate(udp_task, "udp_task", 4096, NULL, 5, NULL);
    xTaskCreate(adc_task, "adc task", 4096, NULL, 5, NULL);

}
