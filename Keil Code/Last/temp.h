/******************************************************************************
 * 文件名：temp.h
 * 功能：DS18B20 温度传感器驱动头文件
 * 硬件接口：P3^7（单总线数据线）
 * 编译器：Keil C51
 ******************************************************************************/

#ifndef __TEMP_H_
#define __TEMP_H_

#include<reg51.h>

/* DS18B20 单总线数据引脚定义 */
sbit DSPORT=P3^7;

/* DS18B20 基本操作函数 */
unsigned char Ds18b20Init(void);              /* 初始化 DS18B20，返回 1=存在 0=不存在 */
void Ds18b20WriteByte(unsigned char dat);     /* 向 DS18B20 写入一个字节 */
unsigned char Ds18b20ReadByte(void);          /* 从 DS18B20 读取一个字节 */

#endif
