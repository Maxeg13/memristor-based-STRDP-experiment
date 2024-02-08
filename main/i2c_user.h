#ifndef MAIN_I2C_USER_H_
#define MAIN_I2C_USER_H_
//---------------------------------------------------------------------
#include <stdint.h>
#include "sdkconfig.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include <unistd.h>
//---------------------------------------------------------------------
esp_err_t i2c_ini(void);
void I2C_SendByteByADDR(uint8_t c,uint8_t addr);
//---------------------------------------------------------------------
#endif /* MAIN_I2C_USER_H_ */
