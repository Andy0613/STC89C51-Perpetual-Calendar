#ifndef __LCD_H_
#define __LCD_H_

/*
 * 使用4位总线时取消注释 LCD1602_4PINS
 * 使用8位总线时注释掉此行
 */
#define LCD1602_4PINS

/* 包含头文件 */
#include<reg51.h>

/* 数据类型重定义 */
#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint
#define uint unsigned int
#endif

/* 引脚定义 */
#define LCD1602_DATAPINS P0   /* LCD数据口 */
sbit LCD1602_E=P2^7;          /* 使能信号 */
sbit LCD1602_RW=P2^5;         /* 读写选择 */
sbit LCD1602_RS=P2^6;         /* 指令/数据选择 */

/* 函数声明 */
void Lcd1602_Delay1ms(uint c);      /* 延时函数(12MHz下) */
void LcdWriteCom(uchar com);        /* 写指令 */
void LcdWriteData(uchar dat);       /* 写数据 */
void LcdInit();                     /* 初始化LCD */

#endif