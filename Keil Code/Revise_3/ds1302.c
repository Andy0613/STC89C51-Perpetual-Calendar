#include"ds1302.h"

/* DS1302读写地址 */
/* 存储顺序为秒分时日月周年 */
uchar code READ_RTC_ADDR[7] = {0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d};
uchar code WRITE_RTC_ADDR[7] = {0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c};

/* DS1302默认时间：2013年1月1日周二 12:00:00 */
/* 存储顺序：秒 分 时 日 月 星期 年 */
/* 存储格式：BCD码 */
uchar TIME[7] = {0, 0, 0x12, 0x01, 0x01, 0x02, 0x13};

/*
 * 函数名：Ds1302Write
 * 功能  ：向DS1302写入数据（地址+数据）
 * 输入  ：addr=寄存器地址, dat=要写入的数据
 * 输出  ：无
 */
void Ds1302Write(uchar addr, uchar dat)
{
	uchar n;
	RST = 0;
	_nop_();

	SCLK = 0;         /* 先将时钟线置低 */
	_nop_();
	RST = 1;          /* 然后RST(CE)置高，开始通信 */
	_nop_();

	for (n=0; n<8; n++)    /* 发送8位地址命令 */
	{
		DSIO = addr & 0x01;     /* 数据从低位开始 */
		addr >>= 1;
		SCLK = 1;               /* 上升沿时DS1302读取数据 */
		_nop_();
		SCLK = 0;
		_nop_();
	}
	for (n=0; n<8; n++)    /* 写入8位数据 */
	{
		DSIO = dat & 0x01;
		dat >>= 1;
		SCLK = 1;               /* 上升沿时DS1302读取数据 */
		_nop_();
		SCLK = 0;
		_nop_();
	}

	RST = 0;              /* 结束数据传输 */
	_nop_();
}

/*
 * 函数名：Ds1302Read
 * 功能  ：从DS1302读取一个字节数据
 * 输入  ：addr=寄存器地址
 * 输出  ：读取到的数据
 */
uchar Ds1302Read(uchar addr)
{
	uchar n,dat,dat1;
	RST = 0;
	_nop_();

	SCLK = 0;         /* 先将时钟线置低 */
	_nop_();
	RST = 1;          /* 然后RST(CE)置高，开始通信 */
	_nop_();

	for(n=0; n<8; n++)    /* 发送8位地址命令 */
	{
		DSIO = addr & 0x01;     /* 数据从低位开始 */
		addr >>= 1;
		SCLK = 1;               /* 上升沿时DS1302读取数据 */
		_nop_();
		SCLK = 0;               /* 下降沿时DS1302输出数据 */
		_nop_();
	}
	_nop_();
	for(n=0; n<8; n++)    /* 读取8位数据 */
	{
		dat1 = DSIO;            /* 从高位开始 */
		dat = (dat>>1) | (dat1<<7);
		SCLK = 1;
		_nop_();
		SCLK = 0;               /* 下降沿时DS1302输出数据 */
		_nop_();
	}

	RST = 0;
	_nop_();            /* 以下为DS1302复位时序，确保稳定 */
	SCLK = 1;
	_nop_();
	DSIO = 0;
	_nop_();
	DSIO = 1;
	_nop_();
	return dat;
}

/*
 * 函数名：Ds1302Init
 * 功能  ：初始化DS1302时钟芯片
 * 输入  ：无
 * 输出  ：无
 * 说明  ：判断CH位，若时钟已运行则跳过初始化
 */
void Ds1302Init()
{
	uchar n;
	uchar sec;
	sec = Ds1302Read(0x81);      /* 读取秒寄存器，检查CH位 */
	if(!(sec & 0x80))            /* CH=0表示时钟已运行，无需初始化 */
		return;                  /* 首次上电，写入默认时间 */
	Ds1302Write(0x8E,0X00);      /* 关闭写保护 */
	for (n=0; n<7; n++)
	{
		Ds1302Write(WRITE_RTC_ADDR[n],TIME[n]);
	}
	Ds1302Write(0x8E,0x80);      /* 开启写保护 */
}

/*
 * 函数名：Ds1302ReadTime
 * 功能  ：读取DS1302当前时间到TIME数组
 * 输入  ：无
 * 输出  ：无
 */
void Ds1302ReadTime()
{
	uchar n;
	for (n=0; n<7; n++)          /* 读取7个字节的时间数据 */
	{
		TIME[n] = Ds1302Read(READ_RTC_ADDR[n]);
	}
}
