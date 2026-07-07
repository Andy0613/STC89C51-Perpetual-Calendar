#include<reg51.h>
#include<intrins.h>
#include"lcd.h"
#include"ds1302.h"
#include"ds18b20.h"

/* 端口定义 */
sbit K1=P3^1;      /* 模式/确认键 */
sbit K2=P3^0;      /* 切换/返回键 */
sbit K3=P3^2;      /* 加/开始暂停 */
sbit K4=P3^3;      /* 减/复位清0 */


/* 全局变量 */
unsigned char mode = 0;           /* 模式: 0=时钟, 2=秒表 */
unsigned char last_mode = 0xFF;   /* 上一模式，用于检测模式切换 */
int temperature = 0;              /* 温度值(10倍) */
unsigned char temp_cnt = 0;       /* 温度采集计数 */
unsigned char alarm_hour=6, alarm_min=0, alarm_on=0;  /* 闹钟参数 */


/* 秒表变量 */
unsigned int sw_sec = 0;          /* 秒表秒数 */
unsigned char sw_csec = 0;        /* 秒表百分秒 */
bit sw_running = 0;               /* 秒表运行状态 */

/* 宏定义 */
#define BCD2DEC(bcd)  (((bcd)>>4)*10 + ((bcd)&0x0f))
#define DEC2BCD(dec)  (((dec)/10<<4) | ((dec)%10))

/* 函数声明 */
void Delay10ms(void);
void DelayMs(unsigned int ms);
void Timer1Init(void);
void TempPros(void);
void AlarmJudge(void);
void AlarmBuzPro(void);
void SetAlarm(void);
void SetTime(void);
void LcdFramework(void);
void LcdUpdate(void);
void KeyScan(void);

/* K1按键检测：去抖 + 确认释放后返回1 */
bit K1_Pressed(void)
{
	if(K1) return 0;
	Delay10ms();
	if(K1) return 0;
	while(!K1);
	return 1;
}

/* K2按键检测：去抖 + 确认释放后返回1 */
bit K2_Pressed(void)
{
	if(K2) return 0;
	Delay10ms();
	if(K2) return 0;
	while(!K2);
	return 1;
}

/* K3按键检测：去抖 + 确认释放后返回1 */
bit K3_Pressed(void)
{
	if(K3) return 0;
	Delay10ms();
	if(K3) return 0;
	while(!K3);
	return 1;
}

/* K4按键检测：去抖 + 确认释放后返回1 */
bit K4_Pressed(void)
{
	if(K4) return 0;
	Delay10ms();
	if(K4) return 0;
	while(!K4);
	return 1;
}

/* LCD写字符串 */
void LcdWriteStr(unsigned char code *str)
{
	while(*str) LcdWriteData(*str++);
}

/* LCD定位 + 2位十进制数显示 */
void LcdShowNumAt(unsigned char addr, unsigned char num)
{
	LcdWriteCom(addr);
	LcdWriteData('0' + num / 10);
	LcdWriteData('0' + num % 10);
}

/* 通用参数调整函数：循环检测K3/K4加减，K1确认退出 */
void AdjustValue(unsigned char *val, unsigned char maxVal,
                 unsigned char dispAddr, unsigned char cursorAddr)
{
	LcdWriteCom(cursorAddr); LcdWriteCom(0x0f);
	while(1)
	{
		if(K3_Pressed())
		{
			if(*val < maxVal) (*val)++; else *val = 0;
			LcdShowNumAt(dispAddr, *val);
			LcdWriteCom(cursorAddr); LcdWriteCom(0x0f);
		}
		if(K4_Pressed())
		{
			if(*val > 0) (*val)--; else *val = maxVal;
			LcdShowNumAt(dispAddr, *val);
			LcdWriteCom(cursorAddr); LcdWriteCom(0x0f);
		}
		if(K1_Pressed()) { LcdWriteCom(0x0c); return; }
	}
}

/* 主函数 */
void main()
{
	Timer1Init();             /* 初始化定时器1 */
	LcdInit();                /* 初始化LCD1602 */

	

	Ds1302Init();                         /* 初始化DS1302 */
	DS18B20_SetResolution(0x3F);          /* 设置DS18B20分辨率10位 */
	DS18B20_StartConvert();               /* 开始温度转换 */

	/* 从DS1302 RAM读取闹钟参数 */
	alarm_hour = Ds1302Read(0xC1);
	if(alarm_hour > 23) alarm_hour = 6;
	alarm_min = Ds1302Read(0xC3);
	if(alarm_min > 59) alarm_min = 0;
	alarm_on = Ds1302Read(0xC5);
	if(alarm_on > 1) alarm_on = 0;

	/* 主循环 */
	while(1)
	{
		Ds1302ReadTime();     /* 读取DS1302时间 */

		/* 每5次循环采集一次温度 */
		temp_cnt++;
		if(temp_cnt >= 5)
		{
			temp_cnt = 0;
			TempPros();
		}

		/* 模式切换时刷新LCD框架 */
		if(mode != last_mode)
		{
			LcdFramework();
			last_mode = mode;
		}

		AlarmJudge();         /* 闹钟判断 */
		AlarmBuzPro();        /* 闹钟蜂鸣器处理 */
		KeyScan();            /* 按键扫描 */
		LcdUpdate();          /* LCD显示更新 */
	}
}

/* LCD写2位十进制数(当前位置) */
void ShowLcdNum(unsigned char num)
{
	LcdWriteData('0' + num / 10);
	LcdWriteData('0' + num % 10);
}

/* LCD显示星期写入 */
void ShowLcdWeek(unsigned char week)
{
	switch(week)
	{
		case 1: LcdWriteData('S'); LcdWriteData('u'); LcdWriteData('n'); break;  /* Sunday */
		case 2: LcdWriteData('M'); LcdWriteData('o'); LcdWriteData('n'); break;  /* Monday */
		case 3: LcdWriteData('T'); LcdWriteData('u'); LcdWriteData('e'); break;  /* Tuesday */
		case 4: LcdWriteData('W'); LcdWriteData('e'); LcdWriteData('d'); break;  /* Wednesday */
		case 5: LcdWriteData('T'); LcdWriteData('h'); LcdWriteData('u'); break;  /* Thursday */
		case 6: LcdWriteData('F'); LcdWriteData('r'); LcdWriteData('i'); break;  /* Friday */
		case 7: LcdWriteData('S'); LcdWriteData('a'); LcdWriteData('t'); break;  /* Saturday */
	}
}

/* LCD框架：刷新静态结构(按模式) */
void LcdFramework()
{
	LcdWriteCom(0x0c);   /* 关闭显示光标 */

	if(mode == 0)     /* 时钟模式框架 */
	{
		LcdWriteCom(0x80);
		LcdWriteStr("20  -  -        ");
		LcdWriteCom(0xC0);
		LcdWriteStr("  :  :          ");
	}
	else if(mode == 2)    /* 秒表模式框架 */
	{
		LcdWriteCom(0x80);
		LcdWriteStr("   Stop Watch  ");
		LcdWriteCom(0xC0);
		LcdWriteStr("  :  :  .      ");
	}
}

/* LCD：刷新动态数据 */
void LcdUpdate()
{
	int temp_disp;

	if(mode == 0)   /* 时钟模式 */
	{
		LcdShowNumAt(0x82, BCD2DEC(TIME[6]));  /* 年 */
		LcdShowNumAt(0x85, BCD2DEC(TIME[4]));  /* 月 */
		LcdShowNumAt(0x88, BCD2DEC(TIME[3]));  /* 日 */
		LcdWriteCom(0x8C);
		ShowLcdWeek(TIME[5] & 0x07);           /* 星期 */
		LcdShowNumAt(0xC0, BCD2DEC(TIME[2]));  /* 时 */
		LcdShowNumAt(0xC3, BCD2DEC(TIME[1]));  /* 分 */
		LcdShowNumAt(0xC6, BCD2DEC(TIME[0]));  /* 秒 */

		/* 显示温度：-12.3C */
		LcdWriteCom(0xC9);
		temp_disp = temperature;
		if(temp_disp < 0) { LcdWriteData('-'); temp_disp = -temp_disp; }
		else               { LcdWriteData(' '); }
		LcdWriteData('0' + temp_disp / 100);
		LcdWriteData('0' + (temp_disp / 10) % 10);
		LcdWriteData('.');
		LcdWriteData('0' + temp_disp % 10);
		LcdWriteData('C');
	}
	else if(mode == 2)   /* 秒表模式 */
	{
		LcdShowNumAt(0xC0, sw_sec / 3600);           /* 时 */
		LcdShowNumAt(0xC3, (sw_sec % 3600) / 60);    /* 分 */
		LcdShowNumAt(0xC6, sw_sec % 60);              /* 秒 */
		LcdShowNumAt(0xC9, sw_csec);                  /* 百分秒 */
	}
}

/* 按键扫描 */
void KeyScan()
{
	if(mode == 0)   /* 时钟模式按键 */
	{
		if(K1_Pressed())
		{
			SetTime();
			last_mode = 0xFF;   /* 强制刷新显示 */
		}
		if(K2_Pressed())
		{
			mode = 2;
			sw_running = 0;
			sw_sec = 0;
			sw_csec = 0;
			LcdFramework();
			last_mode = mode;
			LcdUpdate();
		}
	}
	else if(mode == 2)   /* 秒表模式按键 */
	{
		if(K3_Pressed())
		{
			sw_running = !sw_running;   /* 切换运行状态 */
			TR1 = sw_running;           /* 控制定时器启停 */
		}
		if(K4_Pressed())
		{
			if(!sw_running) { sw_sec = 0; sw_csec = 0; }  /* 停止时清零 */
		}
		if(K2_Pressed())
		{
			mode = 0;
			TR1 = 0;
			sw_running = 0;
			sw_sec = 0;
			sw_csec = 0;
			LcdFramework();
			last_mode = mode;
			LcdUpdate();
		}
	}
}

/* 时间设置：K1切换字段，K3/K4加/减，最后一个字段后自动写入DS1302 */
void SetTime()
{
	unsigned char y,mo,d,w,h,mi,s;

	/* BCD转十进制 */
	y = BCD2DEC(TIME[6]);
	mo = BCD2DEC(TIME[4]);
	d = BCD2DEC(TIME[3]);
	w = TIME[5] & 0x07;
	h = BCD2DEC(TIME[2]);
	mi = BCD2DEC(TIME[1]);
	s = BCD2DEC(TIME[0]);

	/* 清除显示 */
	LcdWriteCom(0x80);
	LcdWriteStr("20  -  -        ");
	LcdWriteCom(0xC0);
	LcdWriteStr("  :  :          ");

	/* 显示当前时间 */
	LcdShowNumAt(0x82, y);
	LcdShowNumAt(0x85, mo);
	LcdShowNumAt(0x88, d);
	LcdWriteCom(0x8C); ShowLcdWeek(w);
	LcdShowNumAt(0xC0, h);
	LcdShowNumAt(0xC3, mi);
	LcdShowNumAt(0xC6, s);

	/* 日期调节 */
	AdjustValue(&y, 99, 0x82, 0x82);
	AdjustValue(&mo, 12, 0x85, 0x85);
	AdjustValue(&d, 31, 0x88, 0x88);
	AdjustValue(&h, 23, 0xC0, 0xC0);
	AdjustValue(&mi, 59, 0xC3, 0xC3);
	AdjustValue(&s, 59, 0xC6, 0xC6);

	/* 星期调节(手动) */
	LcdWriteCom(0x8C); LcdWriteCom(0x0f);
	while(1)
	{
		if(K3_Pressed()) { if(w<7) w++; else w=1; LcdWriteCom(0x8C); ShowLcdWeek(w); LcdWriteCom(0x8C); LcdWriteCom(0x0f); }
		if(K4_Pressed()) { if(w>1) w--; else w=7; LcdWriteCom(0x8C); ShowLcdWeek(w); LcdWriteCom(0x8C); LcdWriteCom(0x0f); }
		if(K1_Pressed()) { break; }
	}

	/* 转换为BCD存入TIME数组 */
	TIME[6] = DEC2BCD(y);
	TIME[4] = DEC2BCD(mo);
	TIME[3] = DEC2BCD(d);
	TIME[5] = w;
	TIME[2] = DEC2BCD(h);
	TIME[1] = DEC2BCD(mi);
	TIME[0] = DEC2BCD(s);

	/* 写入DS1302 */
	Ds1302Write(0x8E, 0x00);
	Ds1302Write(0x80, TIME[0]);
	Ds1302Write(0x82, TIME[1]);
	Ds1302Write(0x84, TIME[2]);
	Ds1302Write(0x86, TIME[3]);
	Ds1302Write(0x88, TIME[4]);
	Ds1302Write(0x8a, TIME[5]);
	Ds1302Write(0x8c, TIME[6]);
	Ds1302Write(0x8E, 0x80);

	/* 恢复显示 */
	LcdWriteCom(0x0c);
	last_mode = 0xFF;
}

/* 闹钟设置：使用AdjustValue重复利用代码 */
void SetAlarm()
{
	LcdWriteCom(0x80);
	LcdWriteStr("Alarm Clock Set ");
	LcdWriteCom(0xC0);
	LcdWriteStr("    :          ");

	LcdShowNumAt(0xC3, alarm_hour);
	LcdShowNumAt(0xC6, alarm_min);

	LcdWriteCom(0xCA);
	LcdWriteStr(alarm_on ? " ON" : "OFF");

	AdjustValue(&alarm_hour, 23, 0xC3, 0xC4);  /* 调整小时 */
	AdjustValue(&alarm_min,  59, 0xC6, 0xC7);  /* 调整分钟 */

	LcdWriteCom(0xCC); LcdWriteCom(0x0f);
	while(1)   /* 闹钟开关 */
	{
		if(K3_Pressed()) { alarm_on = 1; LcdWriteCom(0xCA); LcdWriteStr(" ON"); LcdWriteCom(0xCC); LcdWriteCom(0x0f); }
		if(K4_Pressed()) { alarm_on = 0; LcdWriteCom(0xCA); LcdWriteStr("OFF"); LcdWriteCom(0xCC); LcdWriteCom(0x0f); }
		if(K1_Pressed()) { break; }
	}

	/* 保存闹钟参数到DS1302 RAM */
	LcdWriteCom(0x0c);                 /* 关闭光标 */
	Ds1302Write(0x8E, 0x00);           /* 允许写DS1302 */
	Ds1302Write(0xC0, alarm_hour);     /* 存小时到RAM */
	Ds1302Write(0xC2, alarm_min);      /* 存分钟到RAM */
	Ds1302Write(0xC4, alarm_on);       /* 存开关状态到RAM */
	Ds1302Write(0x8E, 0x80);           /* 禁止写DS1302 */
}




/* 定时器1初始化(供秒表计时) */
void Timer1Init()
{
	TMOD &= 0x0F;   /* 清T1位 */
	TMOD |= 0x10;   /* 设置T1为模式1(16位定时器) */
	ET1 = 1;        /* 使能T1中断 */
	EA = 1;         /* 总中断使能 */
}

/* 定时器1中断服务(约10ms中断一次) */
void Timer1() interrupt 3
{
	TH1 = 0xD8;     /* 装载初值 */
	TL1 = 0xF0;

	if(sw_running)   /* 秒表运行时 */
	{
		sw_csec++;
		if(sw_csec >= 100)   /* 百分秒满100进1秒 */
		{
			sw_csec = 0;
			sw_sec++;
		}
	}
}

/* 温度处理：读取DS18B20并转换为10倍温度值 */
void TempPros()
{
	int raw;
	unsigned char sign = 0;

	if(!DS18B20_GetTemp(&raw))   /* 获取原始温度 */
		return;

	if(raw & 0x8000)             /* 负温度处理 */
	{
		raw = ~raw + 1;
		sign = 1;
	}

	temperature = raw * 10 / 16;  /* 转换为10倍温度值 */
	if(sign)
		temperature = -temperature;

	DS18B20_StartConvert();      /* 启动下一次温度转换 */
}

/* 闹钟判断(空函数) */
void AlarmJudge(void)
{
}

/* 闹钟蜂鸣器处理(蜂鸣器已坏) */
void AlarmBuzPro(void)
{
}

/* 毫秒延时 @12MHz */
void DelayMs(unsigned int ms)
{
	unsigned int i, j;
	for(i=0; i<ms; i++)
		for(j=0; j<112; j++);
}



/* 10ms延时 @12MHz */
void Delay10ms(void)
{
	unsigned char a,b,c;
	for(c=1;c>0;c--)
		for(b=38;b>0;b--)
			for(a=130;a>0;a--);
}