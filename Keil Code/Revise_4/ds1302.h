#ifndef __DS1302_H_
#define __DS1302_H_

/* 包含头文件 */
#include<reg51.h>
#include<intrins.h>

/* 数据类型重定义 */
#ifndef uchar
#define uchar unsigned char
#endif

#ifndef uint
#define uint unsigned int
#endif

/* DS1302使用的IO口定义 */
sbit DSIO=P3^4;       /* 数据线 */
sbit RST=P3^5;        /* 复位/使能脚 */
sbit SCLK=P3^6;       /* 时钟线 */

/* 外部函数声明 */
void Ds1302Write(uchar addr, uchar dat);   /* 写DS1302 */
uchar Ds1302Read(uchar addr);              /* 读DS1302 */
void Ds1302Init();                         /* 初始化DS1302 */
void Ds1302ReadTime();                     /* 读取DS1302时间 */

/* 外部全局变量声明 */
extern uchar TIME[7];   /* 时间缓冲区 */

#endif