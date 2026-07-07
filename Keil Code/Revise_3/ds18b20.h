#ifndef __DS18B20_H_
#define __DS18B20_H_

#include<reg51.h>
#include<intrins.h>

/* 数据类型重定义 */
#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint
#define uint unsigned int
#endif

/* DS18B20数据线引脚定义 */
sbit DS18B20_DQ = P3^7;

/* 函数声明 */
void DS18B20_Reset(void);                  /* 复位DS18B20 */
void DS18B20_WriteByte(uchar dat);         /* 写入一个字节 */
uchar DS18B20_ReadByte(void);              /* 读取一个字节 */
void DS18B20_StartConvert(void);           /* 启动温度转换 */
bit DS18B20_GetTemp(int *temp);            /* 读取温度值 */
void DS18B20_SetResolution(uchar res);     /* 设置分辨率 */

#endif
