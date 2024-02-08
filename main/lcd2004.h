#ifndef MAIN_LCD2004_H_
#define MAIN_LCD2004_H_
//---------------------------------------------------------------------
#include "i2c_user.h"
#include <unistd.h>
//---------------------------------------------------------------------
#define e_set() LCD_WriteByteI2CLCD(portlcd|=0x04)  ////установка линии Е в 1
#define e_reset() LCD_WriteByteI2CLCD(portlcd&=~0x04) //установка линии Е в 0
#define rs_set() LCD_WriteByteI2CLCD(portlcd|=0x01) //установка линии RS в 1
#define rs_reset() LCD_WriteByteI2CLCD(portlcd&=~0x01) //установка линии RS в 0
#define setled() LCD_WriteByteI2CLCD(portlcd|=0x08) //установка линии BL в 1
#define setwrite() LCD_WriteByteI2CLCD(portlcd&=~0x02) //установка линии RW в 0
#define setread() LCD_WriteByteI2CLCD(portlcd|=0x02) //установка линии RW в 1
//---------------------------------------------------------------------
void LCD_WriteByteI2CLCD(uint8_t bt);
void LCD_ini(void);
void LCD_SetPos(uint8_t x, uint8_t y);
void LCD_String(char* st);
//---------------------------------------------------------------------
#endif /* MAIN_LCD2004_H_ */
