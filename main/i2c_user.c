#include "i2c_user.h"
//---------------------------------------------------------------------
#define ACK_CHECK_EN    0x1     /*!< I2C master will check ack from slave*/
//---------------------------------------------------------------------
i2c_port_t i2c_port = I2C_NUM_0;
#define I2C_FREQUENCY   400000
//---------------------------------------------------------------------
void I2C_SendByteByADDR(uint8_t c,uint8_t addr)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, addr, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, c, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  usleep(1000*2);
}
//---------------------------------------------------------------------
esp_err_t i2c_ini(void)
{
  esp_err_t ret;
  i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_SDA_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = CONFIG_SCL_GPIO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQUENCY
  };
  ret = i2c_param_config(i2c_port, &conf);
  if (ret != ESP_OK) return ret;
  ret = i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, 0);
  return ret;
}
//---------------------------------------------------------------------
