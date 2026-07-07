/******************************************************************************
 * 文件名：lcd.h
 * 功能：LCD1602 液晶驱动头文件
 * 硬件接口：4 位数据总线模式
 *          RS = P2^6，RW = P2^5，EN = P2^7，数据口 = P0
 * 编译器：Keil C51
 ******************************************************************************/

#ifndef __LCD_H_
#define __LCD_H_

/* 使用 4 位数据线模式（注释此行则使用 8 位模式） */
#define LCD1602_4PINS

/* 标准头文件 */
#include<reg51.h>

/* 自定义数据类型 */
#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint
#define uint unsigned int
#endif

/* LCD 引脚定义 */
#define LCD1602_DATAPINS P0   /* LCD 数据口（P0 口） */
sbit LCD1602_E  = P2^7;      /* 使能信号 */
sbit LCD1602_RW = P2^5;      /* 读写选择：0=写，1=读 */
sbit LCD1602_RS = P2^6;      /* 寄存器选择：0=指令，1=数据 */

/* 函数声明 */
void Lcd1602_Delay1ms(uint c);      /* 毫秒延时（12MHz 晶振） */
void LcdWriteCom(uchar com);        /* 写入 8 位指令 */
void LcdWriteData(uchar dat);       /* 写入 8 位数据 */
void LcdInit(void);                 /* 初始化 LCD1602 */

#endif
