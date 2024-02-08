#include "lcd2004.h"
//------------------------------------------------
uint8_t portlcd=0; //������ ��� �������� ������ ����� ���������� ����������
uint32_t CUR_POS;
//------------------------------------------------
inline void LCD_WriteByteI2CLCD(uint8_t bt)
{
  I2C_SendByteByADDR(bt,0x4E);
}
//------------------------------------------------
void sendhalfbyte(uint8_t c)
{
  c<<=4;
  LCD_WriteByteI2CLCD((portlcd|=0x04)|c);//�������� ����� E
  usleep(1);
  LCD_WriteByteI2CLCD(portlcd|c);
  LCD_WriteByteI2CLCD((portlcd&=~0x04)|c);//��������� ����� E
  usleep(1);
}
//------------------------------------------------
void sendbyte(uint8_t c, uint8_t mode)
{
  if(mode==0) rs_reset();
  else rs_set();
  uint8_t hc=0;
  hc=c>>4;
  sendhalfbyte(hc);sendhalfbyte(c);
}
//------------------------------------------------
void LCD_SetPos(uint8_t x, uint8_t y)
{
  switch(y)
  {
    case 0:
      CUR_POS = x|0x80;
      break;
    case 1:
      CUR_POS = (0x40+x)|0x80;
      break;
    case 2:
      CUR_POS = (0x14+x)|0x80;
      break;
    case 3:
      CUR_POS = (0x54+x)|0x80;
      break;
  }
  sendbyte(CUR_POS,0);
}
//------------------------------------------------
void LCD_String(char* st)
{
  uint8_t i=0;
  while(st[i]!=0)
  {
    sendbyte(st[i],1);
    i++;
  }
}
//------------------------------------------------
void LCD_ini(void)
{
  usleep(50000);
  LCD_WriteByteI2CLCD(0);
  setwrite();//������
  usleep(50000);
  usleep(50000);
  sendhalfbyte(0x03);
  usleep(4500);
  sendhalfbyte(0x03);
  usleep(4500);
  sendhalfbyte(0x03);
  usleep(4500);
  sendhalfbyte(0x02);
  sendbyte(0x28,0);//����� 4 ���, 2 ����� (�� ������ �������� ������ ��� 4 �����, ����� 5�8
  sendbyte(0x08,0);//������� ���� ���������
  usleep(1000);
  sendbyte(0x01,0);//������ �����
  usleep(2000);
  sendbyte(0x06,0);// ����� ������
  usleep(1000);
  sendbyte(0x0C,0);//������� �������� (D=1), ������� ������� �� �����
  sendbyte(0x02,0);//������ �� �����
  sendbyte(0X80,0);//SET POS LINE 0
  usleep(2000);
  setled();//���������
}
//------------------------------------------------
