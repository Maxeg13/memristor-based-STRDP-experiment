#ifndef MAIN_LCD2004_H_
#define MAIN_LCD2004_H_
//---------------------------------------------------------------------
#include "i2c_user.h"
#include <unistd.h>
//---------------------------------------------------------------------
#define e_set() LCD_WriteByteI2CLCD(portlcd|=0x04)  ////��������� ����� � � 1
#define e_reset() LCD_WriteByteI2CLCD(portlcd&=~0x04) //��������� ����� � � 0
#define rs_set() LCD_WriteByteI2CLCD(portlcd|=0x01) //��������� ����� RS � 1
#define rs_reset() LCD_WriteByteI2CLCD(portlcd&=~0x01) //��������� ����� RS � 0
#define setled() LCD_WriteByteI2CLCD(portlcd|=0x08) //��������� ����� BL � 1
#define setwrite() LCD_WriteByteI2CLCD(portlcd&=~0x02) //��������� ����� RW � 0
#define setread() LCD_WriteByteI2CLCD(portlcd|=0x02) //��������� ����� RW � 1
//---------------------------------------------------------------------
void LCD_WriteByteI2CLCD(uint8_t bt);
void LCD_ini(void);
void LCD_SetPos(uint8_t x, uint8_t y);
void LCD_String(char* st);
//---------------------------------------------------------------------
#endif /* MAIN_LCD2004_H_ */
