#include"lcd.h"

/*
 * 函数名：Lcd1602_Delay1ms
 * 功能  ：延时1ms
 * 输入  ：c=延时ms数
 * 输出  ：无
 * 说明  ：12MHz晶振，12分频下的延时函数
 */
void Lcd1602_Delay1ms(uint c)
{
    uchar a,b;
	for (; c>0; c--)
	{
		 for (b=199;b>0;b--)
		 {
		  	for(a=1;a>0;a--);
		 }
	}
}

/*
 * 函数名：LcdWriteCom
 * 功能  ：向LCD写入8位命令
 * 输入  ：com=命令字节
 * 输出  ：无
 */
#ifndef 	LCD1602_4PINS    /* 8位数据线模式 */
void LcdWriteCom(uchar com)
{
	LCD1602_E = 0;          /* 使能信号置低 */
	LCD1602_RS = 0;         /* 选择指令寄存器 */
	LCD1602_RW = 0;         /* 选择写操作 */

	LCD1602_DATAPINS = com; /* 写入命令 */
	Lcd1602_Delay1ms(1);    /* 等待数据稳定 */

	LCD1602_E = 1;          /* 写入时序 */
	Lcd1602_Delay1ms(5);    /* 保持时间 */
	LCD1602_E = 0;
}
#else                     /* 4位数据线模式 */
void LcdWriteCom(uchar com)
{
	LCD1602_E = 0;
	LCD1602_RS = 0;
	LCD1602_RW = 0;

	LCD1602_DATAPINS = com;         /* 写入高4位 */
	Lcd1602_Delay1ms(1);

	LCD1602_E = 1;
	Lcd1602_Delay1ms(1);
	LCD1602_E = 0;

	Lcd1602_Delay1ms(1);
	LCD1602_DATAPINS = com << 4;    /* 写入低4位 */
	Lcd1602_Delay1ms(1);

	LCD1602_E = 1;
	Lcd1602_Delay1ms(1);
	LCD1602_E = 0;
}
#endif

/*
 * 函数名：LcdWriteData
 * 功能  ：向LCD写入8位数据
 * 输入  ：dat=数据字节
 * 输出  ：无
 */
#ifndef 	LCD1602_4PINS    /* 8位数据线模式 */
void LcdWriteData(uchar dat)
{
	LCD1602_E = 0;          /* 使能信号置低 */
	LCD1602_RS = 1;         /* 选择数据寄存器 */
	LCD1602_RW = 0;         /* 选择写操作 */

	LCD1602_DATAPINS = dat; /* 写入数据 */
	Lcd1602_Delay1ms(1);

	LCD1602_E = 1;          /* 写入时序 */
	Lcd1602_Delay1ms(5);
	LCD1602_E = 0;
}
#else                     /* 4位数据线模式 */
void LcdWriteData(uchar dat)
{
	LCD1602_E = 0;
	LCD1602_RS = 1;
	LCD1602_RW = 0;

	LCD1602_DATAPINS = dat;         /* 写入高4位 */
	Lcd1602_Delay1ms(1);

	LCD1602_E = 1;
	Lcd1602_Delay1ms(1);
	LCD1602_E = 0;

	Lcd1602_Delay1ms(1);
	LCD1602_DATAPINS = dat << 4;    /* 写入低4位 */
	Lcd1602_Delay1ms(1);

	LCD1602_E = 1;
	Lcd1602_Delay1ms(1);
	LCD1602_E = 0;
}
#endif

/*
 * 函数名：LcdInit
 * 功能  ：初始化LCD1602液晶
 * 输入  ：无
 * 输出  ：无
 */
#ifndef		LCD1602_4PINS    /* 8位数据线模式 */
void LcdInit()
{
 	LcdWriteCom(0x38);   /* 设置显示模式：16x2, 5x7点阵, 8位 */
	LcdWriteCom(0x0c);   /* 开显示，不显示光标 */
	LcdWriteCom(0x06);   /* 写入数据后地址自动加1 */
	LcdWriteCom(0x01);   /* 清屏 */
	LcdWriteCom(0x80);   /* 设置DDRAM地址为0x00（第一行起始） */
}
#else                     /* 4位数据线模式 */
void LcdInit()
{
	LcdWriteCom(0x32);   /* 从8位模式转换为4位模式 */
	LcdWriteCom(0x28);   /* 4位模式下的初始化 */
	LcdWriteCom(0x0c);   /* 开显示，不显示光标 */
	LcdWriteCom(0x06);   /* 写入数据后地址自动加1 */
	LcdWriteCom(0x01);   /* 清屏 */
	LcdWriteCom(0x80);   /* 设置DDRAM地址为0x00（第一行起始） */
}
#endif
