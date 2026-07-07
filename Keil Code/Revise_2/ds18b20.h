#ifndef __DS18B20_H_
#define __DS18B20_H_

#include<reg51.h>
#include<intrins.h>

#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint
#define uint unsigned int
#endif

sbit DS18B20_DQ = P3^7;

void DS18B20_Reset(void);
void DS18B20_WriteByte(uchar dat);
uchar DS18B20_ReadByte(void);
void DS18B20_StartConvert(void);
bit DS18B20_GetTemp(int *temp);
void DS18B20_SetResolution(uchar res);

#endif
