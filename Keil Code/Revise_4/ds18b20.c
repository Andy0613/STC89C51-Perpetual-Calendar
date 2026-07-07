#include "ds18b20.h"

/* DS18B20复位 */
void DS18B20_Reset(void)
{
	uchar i;
	DS18B20_DQ = 0;             /* 拉低总线 */
	for(i=0; i<200; i++);      /* 延时约480us */
	DS18B20_DQ = 1;             /* 释放总线 */
	for(i=0; i<100; i++);      /* 等待从机应答 */
}

/* DS18B20写入一个字节 */
void DS18B20_WriteByte(uchar dat)
{
	uchar i, j;
	for(i=0; i<8; i++)
	{
		DS18B20_DQ = 0;         /* 拉低总线开始写时隙 */
		_nop_();
		if(dat & 0x01)          /* 写1 */
			DS18B20_DQ = 1;
		else                     /* 写0 */
			DS18B20_DQ = 0;
		for(j=0; j<30; j++);   /* 保持时间约60us */
		DS18B20_DQ = 1;         /* 释放总线 */
		_nop_();
		_nop_();
		dat >>= 1;               /* 下一位 */
	}
}

/* DS18B20读取一个字节 */
uchar DS18B20_ReadByte(void)
{
	uchar i, j, dat = 0;
	for(i=0; i<8; i++)
	{
		dat >>= 1;
		DS18B20_DQ = 0;         /* 拉低总线开始读时隙 */
		_nop_();
		DS18B20_DQ = 1;         /* 释放总线 */
		_nop_();
		_nop_();
		if(DS18B20_DQ)          /* 读取数据 */
			dat |= 0x80;
		for(j=0; j<20; j++);   /* 延时等待 */
	}
	return dat;
}

/* 启动温度转换 */
void DS18B20_StartConvert(void)
{
	DS18B20_Reset();             /* 复位 */
	DS18B20_WriteByte(0xCC);     /* 跳过ROM */
	DS18B20_WriteByte(0x44);     /* 启动温度转换 */
}

/* 设置DS18B20分辨率 */
/* res: 0x1F=9bit, 0x3F=10bit, 0x5F=11bit, 0x7F=12bit */
void DS18B20_SetResolution(uchar res)
{
	DS18B20_Reset();
	DS18B20_WriteByte(0xCC);       /* 跳过ROM */
	DS18B20_WriteByte(0x4E);       /* 写暂存器 */
	DS18B20_WriteByte(0x00);       /* TH报警值 */
	DS18B20_WriteByte(0x00);       /* TL报警值 */
	DS18B20_WriteByte(res);        /* 设置寄存器 */
	DS18B20_Reset();
	DS18B20_WriteByte(0xCC);       /* 跳过ROM */
	DS18B20_WriteByte(0x48);       /* 将暂存器写到EEPROM */
}

/* 获取温度值 */
/* 返回1表示成功，temp存放原始温度值 */
bit DS18B20_GetTemp(int *temp)
{
	uchar tempL, tempH;
	int raw;

	DS18B20_Reset();
	DS18B20_WriteByte(0xCC);     /* 跳过ROM */
	DS18B20_WriteByte(0xBE);     /* 读取暂存器 */

	tempL = DS18B20_ReadByte();  /* 读取温度低字节 */
	tempH = DS18B20_ReadByte();  /* 读取温度高字节 */

	raw = (tempH << 8) | tempL;  /* 合成16位温度值 */
	*temp = raw;

	return 1;
}