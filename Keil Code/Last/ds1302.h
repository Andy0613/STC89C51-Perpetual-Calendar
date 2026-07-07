/******************************************************************************
 * 文件名：ds1302.h
 * 功能：DS1302 实时时钟驱动头文件
 * 硬件接口：DSIO = P3^4，RST = P3^5，SCLK = P3^6
 * 编译器：Keil C51
 ******************************************************************************/

#ifndef __DS1302_H_
#define __DS1302_H_

#include<reg51.h>
#include<intrins.h>

/* 自定义数据类型 */
#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint
#define uint unsigned int
#endif

/* DS1302 引脚定义 */
sbit DSIO  = P3^4;    /* 数据线 */
sbit RST   = P3^5;    /* 复位/使能引脚（CE） */
sbit SCLK  = P3^6;    /* 串行时钟线 */

/* 函数声明 */
void Ds1302Write(uchar addr, uchar dat);      /* 向 DS1302 写入一字节数据 */
uchar Ds1302Read(uchar addr);                  /* 从 DS1302 读取一字节数据 */
void Ds1302Init(void);                         /* 初始化 DS1302 时钟芯片 */
void Ds1302ReadTime(void);                     /* 读取当前时间到 TIME 数组 */

/* 外部全局变量 */
extern uchar TIME[7];   /* 时间缓冲区：秒、分、时、日、月、周、年（BCD 码） */

#endif
