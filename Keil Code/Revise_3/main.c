#include<reg51.h>
#include<intrins.h>
#include"lcd.h"
#include"ds1302.h"
#include"ds18b20.h"

/* 按键引脚定义 */
sbit K1=P3^1;      /* 模式/确认键 */
sbit K2=P3^0;      /* 切换/返回键 */
sbit K3=P3^2;      /* 加/开始暂停键 */
sbit K4=P3^3;      /* 减/复位键 */
sbit Buzzer=P2^0;  /* 蜂鸣器引脚 */

/* 函数声明 */
void Delay10ms(void);
void LcdFramework(void);
void LcdUpdate(void);
void TempPros(void);
void KeyScan(void);
void SetAlarm(void);
void AlarmJudge(void);
void AlarmBuzPro(void);
void Timer1Init(void);
void KeySound(void);
void ReturnSound(void);
void DelayMs(unsigned int ms);

/* 全局变量 */
unsigned char mode = 0;           /* 模式: 0=时钟, 2=秒表 */
unsigned char last_mode = 0xFF;   /* 上一次模式，用于检测模式切换 */
int temperature = 0;              /* 温度值（10倍） */
unsigned char temp_cnt = 0;       /* 温度采集计数 */

/* 闹钟参数 */
unsigned char alarm_hour = 6;     /* 闹钟小时 */
unsigned char alarm_min = 0;      /* 闹钟分钟 */
unsigned char alarm_on = 0;       /* 闹钟开关 */
unsigned char alarm_buz = 0;      /* 闹钟蜂鸣标志 */
unsigned char alarm_buz_cnt = 0;  /* 闹钟蜂鸣计数 */

/* 秒表参数 */
unsigned int sw_sec = 0;          /* 秒表秒数 */
unsigned char sw_csec = 0;        /* 秒表百分秒 */
bit sw_running = 0;               /* 秒表运行状态 */

void main()
{
	Timer1Init();             /* 初始化定时器1 */
	LcdInit();                /* 初始化LCD1602 */

	/* 开机蜂鸣器响一声 */
	Buzzer = 0;
	Delay10ms();Delay10ms();Delay10ms();Delay10ms();Delay10ms();
	Delay10ms();Delay10ms();Delay10ms();Delay10ms();Delay10ms();
	Buzzer = 1;

	Ds1302Init();                         /* 初始化DS1302 */
	DS18B20_SetResolution(0x3F);          /* 设置DS18B20分辨率10位 */
	DS18B20_StartConvert();               /* 启动温度转换 */

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

		/* 每5个循环采集一次温度 */
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
		AlarmBuzPro();        /* 闹钟蜂鸣处理 */
		KeyScan();            /* 按键扫描 */
		LcdUpdate();          /* LCD显示更新 */
	}
}

/* 在LCD上显示两位数字 */
void ShowLcdNum(unsigned char num)
{
	LcdWriteData('0' + num / 10);   /* 十位 */
	LcdWriteData('0' + num % 10);   /* 个位 */
}

/* 在LCD上显示星期（英文缩写） */
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

/* LCD界面框架绘制 */
void LcdFramework()
{
	if(mode == 0)     /* 时钟模式界面 */
	{
		LcdWriteCom(0x0c);   /* 不显示光标 */

		/* 第一行：20   -  -      */
		LcdWriteCom(0x80);
		LcdWriteData('2'); LcdWriteData('0');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData('-');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData('-');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');

		/* 第二行：  :  :     温度 */
		LcdWriteCom(0x80+0x40);
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(':');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(':');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');
	}
	else if(mode == 2)    /* 秒表模式界面 */
	{
		LcdWriteCom(0x0c);   /* 不显示光标 */

		/* 第一行：显示 Stop Watch 标题 */
		LcdWriteCom(0x80);
		LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData('S'); LcdWriteData('t'); LcdWriteData('o');
		LcdWriteData('p'); LcdWriteData(' ');
		LcdWriteData('W'); LcdWriteData('a'); LcdWriteData('t');
		LcdWriteData('c'); LcdWriteData('h');
		LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');

		/* 第二行：  :  :  .    */
		LcdWriteCom(0x80+0x40);
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(':'); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(':');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData('.');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');
	}
}

/* LCD显示数据更新 */
void LcdUpdate()
{
	unsigned char t;
	int temp_disp;

	if(mode == 0)   /* 时钟模式 */
	{
		/* 显示年（BCD码转十进制） */
		LcdWriteCom(0x82);
		ShowLcdNum((TIME[6]>>4)*10 + (TIME[6]&0x0f));

		/* 显示月 */
		LcdWriteCom(0x85);
		ShowLcdNum((TIME[4]>>4)*10 + (TIME[4]&0x0f));

		/* 显示日 */
		LcdWriteCom(0x88);
		ShowLcdNum((TIME[3]>>4)*10 + (TIME[3]&0x0f));

		/* 显示星期 */
		LcdWriteCom(0x8C);
		ShowLcdWeek(TIME[5] & 0x07);

		/* 显示时 */
		LcdWriteCom(0xC0);
		ShowLcdNum((TIME[2]>>4)*10 + (TIME[2]&0x0f));

		/* 显示分 */
		LcdWriteCom(0xC3);
		ShowLcdNum((TIME[1]>>4)*10 + (TIME[1]&0x0f));

		/* 显示秒 */
		LcdWriteCom(0xC6);
		ShowLcdNum((TIME[0]>>4)*10 + (TIME[0]&0x0f));

		/* 显示温度，如 -12.3C */
		LcdWriteCom(0xC9);
		temp_disp = temperature;
		if(temp_disp < 0) { LcdWriteData('-'); temp_disp = -temp_disp; }
		else { LcdWriteData(' '); }
		LcdWriteData('0' + temp_disp / 100);
		LcdWriteData('0' + (temp_disp / 10) % 10);
		LcdWriteData('.');
		LcdWriteData('0' + temp_disp % 10);
		LcdWriteData('C');
	}
	else if(mode == 2)   /* 秒表模式 */
	{
		/* 显示时 */
		t = sw_sec / 3600;
		LcdWriteCom(0xC0);
		LcdWriteData('0' + t / 10);
		LcdWriteData('0' + t % 10);

		/* 显示分 */
		LcdWriteCom(0xC3);
		t = (sw_sec % 3600) / 60;
		LcdWriteData('0' + t / 10);
		LcdWriteData('0' + t % 10);

		/* 显示秒 */
		LcdWriteCom(0xC6);
		LcdWriteData('0' + (sw_sec % 60) / 10);
		LcdWriteData('0' + (sw_sec % 60) % 10);

		/* 显示百分秒 */
		LcdWriteCom(0xC9);
		LcdWriteData('0' + sw_csec / 10);
		LcdWriteData('0' + sw_csec % 10);
	}
}

/* 按键扫描处理 */
void KeyScan()
{
	if(mode == 0)   /* 时钟模式下的按键 */
	{
		/* K1：进入闹钟设置 */
		if(K1 == 0)
		{
			Delay10ms();
			if(K1 == 0)
			{
				while(!K1);
				SetAlarm();
				last_mode = 0xFF;   /* 强制刷新界面 */
				ReturnSound();
			}
		}
		/* K2：进入秒表模式 */
		if(K2 == 0)
		{
			Delay10ms();
			if(K2 == 0)
			{
				while(!K2);
				KeySound();
				mode = 2;
				sw_running = 0;   /* 秒表停止 */
				sw_sec = 0;       /* 秒表归零 */
				sw_csec = 0;
			}
		}
	}
	else if(mode == 2)   /* 秒表模式下的按键 */
	{
		/* K3：开始/暂停秒表 */
		if(K3 == 0)
		{
			Delay10ms();
			if(K3 == 0)
			{
				while(!K3);
				KeySound();
				sw_running = !sw_running;   /* 切换运行状态 */
				TR1 = sw_running;           /* 控制定时器启停 */
			}
		}
		/* K4：秒表复位（仅在停止时有效） */
		if(K4 == 0)
		{
			Delay10ms();
			if(K4 == 0)
			{
				while(!K4);
				KeySound();
				if(!sw_running)
				{
					sw_sec = 0;   /* 秒表归零 */
					sw_csec = 0;
				}
			}
		}
		/* K2：返回时钟模式 */
		if(K2 == 0)
		{
			Delay10ms();
			if(K2 == 0)
			{
				while(!K2);
				mode = 0;
				TR1 = 0;            /* 关闭定时器 */
				sw_running = 0;
				sw_sec = 0;
				sw_csec = 0;
				ReturnSound();
			}
		}
	}
}

/* 闹钟设置功能 */
void SetAlarm()
{
	/* 显示闹钟设置界面标题 */
	LcdWriteCom(0x80);
	LcdWriteData('A'); LcdWriteData('l'); LcdWriteData('a'); LcdWriteData('r');
	LcdWriteData('m'); LcdWriteData(' '); LcdWriteData('C'); LcdWriteData('l');
	LcdWriteData('o'); LcdWriteData('c'); LcdWriteData('k'); LcdWriteData(' ');
	LcdWriteData('S'); LcdWriteData('e'); LcdWriteData('t'); LcdWriteData(' ');

	/* 显示时间占位符 */
	LcdWriteCom(0xC0);
	LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');
	LcdWriteData(' '); LcdWriteData(':'); LcdWriteData(' '); LcdWriteData(' ');
	LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');
	LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');

	/* 显示当前闹钟小时 */
	LcdWriteCom(0xC3);
	LcdWriteData('0' + alarm_hour / 10);
	LcdWriteData('0' + alarm_hour % 10);

	/* 显示当前闹钟分钟 */
	LcdWriteCom(0xC6);
	LcdWriteData('0' + alarm_min / 10);
	LcdWriteData('0' + alarm_min % 10);

	/* 显示闹钟开关状态 */
	LcdWriteCom(0xCA);
	if(alarm_on) { LcdWriteData(' '); LcdWriteData('O'); LcdWriteData('N'); }
	else { LcdWriteData('O'); LcdWriteData('F'); LcdWriteData('F'); }

	/* 设置闹钟小时 */
	LcdWriteCom(0xC4);
	LcdWriteCom(0x0f);   /* 显示光标 */
	while(1)
	{
		if(K3 == 0)     /* K3加 */
		{
			Delay10ms();
			if(K3 == 0)
			{
				while(!K3);
				KeySound();
				if(alarm_hour < 23) alarm_hour++; else alarm_hour = 0;
				LcdWriteCom(0xC3);
				LcdWriteData('0' + alarm_hour / 10);
				LcdWriteData('0' + alarm_hour % 10);
				LcdWriteCom(0xC4);
				LcdWriteCom(0x0f);
			}
		}
		if(K4 == 0)     /* K4减 */
		{
			Delay10ms();
			if(K4 == 0)
			{
				while(!K4);
				KeySound();
				if(alarm_hour > 0) alarm_hour--; else alarm_hour = 23;
				LcdWriteCom(0xC3);
				LcdWriteData('0' + alarm_hour / 10);
				LcdWriteData('0' + alarm_hour % 10);
				LcdWriteCom(0xC4);
				LcdWriteCom(0x0f);
			}
		}
		if(K1 == 0)     /* K1确认 */
		{
			Delay10ms();
			if(K1 == 0)
			{
				while(!K1);
				KeySound();
				break;
			}
		}
	}

	/* 设置闹钟分钟 */
	LcdWriteCom(0xC7);
	LcdWriteCom(0x0f);   /* 显示光标 */
	while(1)
	{
		if(K3 == 0)     /* K3加 */
		{
			Delay10ms();
			if(K3 == 0)
			{
				while(!K3);
				KeySound();
				if(alarm_min < 59) alarm_min++; else alarm_min = 0;
				LcdWriteCom(0xC6);
				LcdWriteData('0' + alarm_min / 10);
				LcdWriteData('0' + alarm_min % 10);
				LcdWriteCom(0xC7);
				LcdWriteCom(0x0f);
			}
		}
		if(K4 == 0)     /* K4减 */
		{
			Delay10ms();
			if(K4 == 0)
			{
				while(!K4);
				KeySound();
				if(alarm_min > 0) alarm_min--; else alarm_min = 59;
				LcdWriteCom(0xC6);
				LcdWriteData('0' + alarm_min / 10);
				LcdWriteData('0' + alarm_min % 10);
				LcdWriteCom(0xC7);
				LcdWriteCom(0x0f);
			}
		}
		if(K1 == 0)     /* K1确认 */
		{
			Delay10ms();
			if(K1 == 0)
			{
				while(!K1);
				KeySound();
				break;
			}
		}
	}

	/* 设置闹钟开关 */
	LcdWriteCom(0xCC);
	LcdWriteCom(0x0f);   /* 显示光标 */
	while(1)
	{
		if(K3 == 0)     /* K3：开启闹钟 */
		{
			Delay10ms();
			if(K3 == 0)
			{
				while(!K3);
				KeySound();
				alarm_on = 1;
				LcdWriteCom(0xCA);
				LcdWriteData(' '); LcdWriteData('O'); LcdWriteData('N');
				LcdWriteCom(0xCC);
				LcdWriteCom(0x0f);
			}
		}
		if(K4 == 0)     /* K4：关闭闹钟 */
		{
			Delay10ms();
			if(K4 == 0)
			{
				while(!K4);
				KeySound();
				alarm_on = 0;
				LcdWriteCom(0xCA);
				LcdWriteData('O'); LcdWriteData('F'); LcdWriteData('F');
				LcdWriteCom(0xCC);
				LcdWriteCom(0x0f);
			}
		}
		if(K1 == 0)     /* K1确认退出 */
		{
			Delay10ms();
			if(K1 == 0)
			{
				while(!K1);
				KeySound();
				break;
			}
		}
	}

	/* 退出设置，保存闹钟参数到DS1302 RAM */
	LcdWriteCom(0x0c);                 /* 关闭光标 */
	Ds1302Write(0x8E, 0x00);           /* 允许写DS1302 */
	Ds1302Write(0xC0, alarm_hour);     /* 保存小时到RAM */
	Ds1302Write(0xC2, alarm_min);      /* 保存分钟到RAM */
	Ds1302Write(0xC4, alarm_on);       /* 保存开关状态到RAM */
	Ds1302Write(0x8E, 0x80);           /* 禁止写DS1302 */
}

/* 闹钟判断 */
void AlarmJudge()
{
	unsigned char h, m, s;
	h = (TIME[2] >> 4) * 10 + (TIME[2] & 0x0f);   /* 当前小时 */
	m = (TIME[1] >> 4) * 10 + (TIME[1] & 0x0f);   /* 当前分钟 */
	s = (TIME[0] >> 4) * 10 + (TIME[0] & 0x0f);   /* 当前秒 */

	/* 闹钟响铃中：每分钟的0秒检查是否超时 */
	if(alarm_buz)
	{
		if(s == 0)
		{
			alarm_buz_cnt++;
			if(alarm_buz_cnt > 50)   /* 超过50分钟则停止响铃 */
			{
				alarm_buz_cnt = 0;
				alarm_buz = 0;
			}
		}
	}

	/* 到达闹钟时间且前3秒触发响铃 */
	if(alarm_on && h == alarm_hour && m == alarm_min && s >= 0 && s < 3)
	{
		if(!alarm_buz) alarm_buz = 1;
	}

	/* 到达59秒时关闭闹钟 */
	if(s == 59)
		alarm_buz = 0;

	/* 按任意键关闭闹钟 */
	if(K1 == 0 || K2 == 0)
	{
		alarm_buz = 0;
		Buzzer = 1;
	}
}

/* 闹钟蜂鸣处理（蜂鸣器交替响） */
void AlarmBuzPro()
{
	if(alarm_buz)
	{
		Buzzer = 0;
		DelayMs(50);
		Buzzer = 1;
		DelayMs(50);
	}
}

/* 定时器1初始化（用于秒表） */
void Timer1Init()
{
	TMOD &= 0x0F;   /* 清除T1位 */
	TMOD |= 0x10;   /* 设置T1为模式1（16位定时器） */
	ET1 = 1;        /* 使能T1中断 */
	EA = 1;         /* 开总中断 */
}

/* 定时器1中断服务函数（约10ms中断一次） */
void Timer1() interrupt 3
{
	TH1 = 0xD8;     /* 重装初值 */
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

	if(!DS18B20_GetTemp(&raw))   /* 读取原始温度 */
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

/* 毫秒延时函数（12MHz晶振） */
void DelayMs(unsigned int ms)
{
	unsigned int i, j;
	for(i=0; i<ms; i++)
		for(j=0; j<112; j++);
}

/* 按键提示音（短响一声） */
void KeySound()
{
	Buzzer = 0;
	DelayMs(40);
	Buzzer = 1;
}

/* 返回提示音（短响两声） */
void ReturnSound()
{
	Buzzer = 0;
	DelayMs(40);
	Buzzer = 1;
	DelayMs(30);
	Buzzer = 0;
	DelayMs(40);
	Buzzer = 1;
}

/* 10ms延时函数（12MHz晶振） */
void Delay10ms(void)
{
	unsigned char a,b,c;
	for(c=1;c>0;c--)
		for(b=38;b>0;b--)
			for(a=130;a>0;a--);
}
