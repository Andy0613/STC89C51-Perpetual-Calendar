/******************************************************************************
 * 文件名：main.c
 * 功能：DS1302 实时时钟 + DS18B20 温度采集 + LCD1602 显示
 *       支持按键切换时钟/秒表模式，可设置时间。
 * 硬件配置：
 *   LCD1602（4 位模式）：RS=P2^6, RW=P2^5, EN=P2^7, 数据口=P0
 *   DS1302：DSIO=P3^4, RST=P3^5, SCLK=P3^6
 *   DS18B20：DQ=P3^7
 *   按键：K1=P3^1, K2=P3^0, K3=P3^2, K4=P3^3
 *   蜂鸣器/报警：已禁用（函数为空）
 * 编译器：Keil C51
 ******************************************************************************/

#include<reg51.h>
#include<intrins.h>
#include"lcd.h"
#include"ds1302.h"
#include"temp.h"


/*========================== 引脚定义 ==========================*/

sbit K1 = P3^1;      /* 模式切换/确认键 */
sbit K2 = P3^0;      /* 切换/返回键 */
sbit K3 = P3^2;      /* 加/开始暂停键 */
sbit K4 = P3^3;      /* 减/清零键 */


/*========================== 全局变量 ==========================*/

unsigned char mode = 0;               /* 工作模式：0=时钟，2=秒表 */
unsigned char last_mode = 0xFF;       /* 前一次模式，用于检测模式切换 */
int temperature = 0;                  /* 温度值（放大 10 倍，如 253 表示 25.3°C） */
unsigned char temp_state = 0;         /* 温度采集状态机：0=空闲，1=转换中 */
unsigned int temp_wait = 0;           /* 温度转换等待计数器 */

unsigned char alarm_hour = 6;         /* 闹钟小时 */
unsigned char alarm_min  = 0;         /* 闹钟分钟 */
unsigned char alarm_on   = 0;         /* 闹钟开关：0=关，1=开 */

unsigned int  sw_sec   = 0;           /* 秒表累计秒数 */
unsigned char sw_csec  = 0;           /* 秒表百分秒（0~99） */
bit sw_running = 0;                   /* 秒表运行状态：0=停止，1=运行 */


/*========================== 宏定义 ==========================*/

/* BCD 码转十进制：0x12 → 12 */
#define BCD2DEC(bcd)  (((bcd)>>4)*10 + ((bcd)&0x0f))

/* 十进制转 BCD 码：12 → 0x12 */
#define DEC2BCD(dec)  (((dec)/10<<4) | ((dec)%10))


/*========================== 函数声明 ==========================*/

void Delay10ms(void);

void Timer1Init(void);
void TempPros(void);
void AlarmJudge(void);
void AlarmBuzPro(void);
void SetTime(void);
void LcdFramework(void);
void LcdUpdate(void);
void KeyScan(void);


/*======================= 按键检测函数 ========================*/

/*
 * K1_Pressed：检测 K1 键是否被按下
 * 功能：软件消抖，等待按键释放后返回 1
 * 返回：1=按下了 K1，0=未按下
 */
bit K1_Pressed(void)
{
	if(K1) return 0;         /* 首次检测，高电平表示未按下 */
	Delay10ms();             /* 软件消抖 */
	if(K1) return 0;         /* 再次检测，确认按下 */
	while(!K1);              /* 等待按键释放 */
	return 1;                /* 有效按下 */
}

/*
 * K2_Pressed：检测 K2 键是否被按下
 */
bit K2_Pressed(void)
{
	if(K2) return 0;
	Delay10ms();
	if(K2) return 0;
	while(!K2);
	return 1;
}

/*
 * K3_Pressed：检测 K3 键是否被按下
 */
bit K3_Pressed(void)
{
	if(K3) return 0;
	Delay10ms();
	if(K3) return 0;
	while(!K3);
	return 1;
}

/*
 * K4_Pressed：检测 K4 键是否被按下
 */
bit K4_Pressed(void)
{
	if(K4) return 0;
	Delay10ms();
	if(K4) return 0;
	while(!K4);
	return 1;
}


/*===================== LCD 辅助函数 ======================*/

/*
 * LcdWriteStr：在 LCD 当前位置显示字符串
 * 输入：str = 指向 code 区字符串的指针
 */
void LcdWriteStr(unsigned char code *str)
{
	while(*str) LcdWriteData(*str++);
}

/*
 * LcdShowNumAt：在 LCD 指定位置显示两位十进制数
 * 输入：addr = DDRAM 地址，num = 要显示的数值（0~99）
 */
void LcdShowNumAt(unsigned char addr, unsigned char num)
{
	LcdWriteCom(addr);
	LcdWriteData('0' + num / 10);    /* 十位 ASCII 码 */
	LcdWriteData('0' + num % 10);    /* 个位 ASCII 码 */
}

/*
 * AdjustValue：通用参数调节函数
 * 功能：在指定位置显示数值，K3/K4 加减，K1 确认退出
 *       光标停留在数值的最后一位上。
 * 输入：val = 待调节变量指针
 *       maxVal = 最大值
 *       dispAddr = 显示地址
 *       cursorAddr = 光标地址（函数内会自动加 1 移到末位）
 */
void AdjustValue(unsigned char *val, unsigned char maxVal,
                 unsigned char dispAddr, unsigned char cursorAddr)
{
	LcdWriteCom(cursorAddr + 1);     /* 光标放在数值最后一位 */
	LcdWriteCom(0x0f);               /* 显示闪烁光标 */

	while(1)
	{
		if(K3_Pressed())             /* K3：加 */
		{
			if(*val < maxVal) (*val)++;
			else *val = 0;
			LcdShowNumAt(dispAddr, *val);
			LcdWriteCom(cursorAddr + 1);  /* 光标回到末位 */
			LcdWriteCom(0x0f);
		}
		if(K4_Pressed())             /* K4：减 */
		{
			if(*val > 0) (*val)--;
			else *val = maxVal;
			LcdShowNumAt(dispAddr, *val);
			LcdWriteCom(cursorAddr + 1);
			LcdWriteCom(0x0f);
		}
		if(K1_Pressed())             /* K1：确认退出 */
		{
			LcdWriteCom(0x0c);       /* 关闭光标 */
			return;
		}
	}
}


/*========================== 主函数 ==========================*/

void main()
{
	Timer1Init();             /* 初始化定时器 1（用于秒表计时） */
	LcdInit();                /* 初始化 LCD1602 液晶 */

	Ds1302Init();             /* 初始化 DS1302 时钟芯片 */

	/* 从 DS1302 RAM 读取闹钟参数（上电保持） */
	alarm_hour = Ds1302Read(0xC1);
	if(alarm_hour > 23) alarm_hour = 6;   /* 数据异常时使用默认值 */
	alarm_min  = Ds1302Read(0xC3);
	if(alarm_min > 59)  alarm_min  = 0;
	alarm_on   = Ds1302Read(0xC5);
	if(alarm_on > 1)    alarm_on   = 0;

	/* 主循环 */
	while(1)
	{
		Ds1302ReadTime();     /* 读取 DS1302 当前时间到 TIME 数组 */

		TempPros();           /* 温度采集状态机（非阻塞式） */

		/* 模式切换时刷新 LCD 静态框架 */
		if(mode != last_mode)
		{
			LcdFramework();
			last_mode = mode;
		}

		AlarmJudge();         /* 闹钟判断（当前为空函数） */
		AlarmBuzPro();        /* 蜂鸣器驱动（当前为空函数） */
		KeyScan();            /* 按键扫描 */
		LcdUpdate();          /* 刷新 LCD 动态显示 */
	}
}


/*
 * ShowLcdWeek：在 LCD 当前位置显示星期缩写
 * 输入：week = 1~7（对应 Sunday~Saturday）
 */
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


/*===================== LCD 框架刷新 ======================*/

/*
 * LcdFramework：刷新 LCD 静态框架
 * 功能：根据当前模式，绘制 LCD 的固定显示内容
 *       模式 0（时钟）：显示日期和时间占位符
 *       模式 2（秒表）：显示秒表标题和数字占位符
 */
void LcdFramework()
{
	LcdWriteCom(0x0c);        /* 关闭光标 */

	if(mode == 0)             /* 时钟模式 */
	{
		LcdWriteCom(0x80);
		LcdWriteStr("20  -  -        ");   /* 第一行：年-月-日 */
		LcdWriteCom(0xC0);
		LcdWriteStr("  :  :          ");   /* 第二行：时:分:秒 + 温度 */
	}
	else if(mode == 2)        /* 秒表模式 */
	{
		LcdWriteCom(0x80);
		LcdWriteStr("   Stop Watch   ");   /* 第一行：标题居中 */
		LcdWriteCom(0xC0);
		LcdWriteStr("    :  :  .     ");   /* 第二行：HH:MM:SS.cs 居中 */
	}
}


/*======================= LCD 动态更新 ========================*/

/*
 * LcdUpdate：刷新 LCD 动态显示数据
 * 功能：根据当前模式，更新温度/时间/秒表等动态数值
 *       时钟模式显示年月日时分秒星期和温度
 *       秒表模式显示时、分、秒、百分秒
 */
void LcdUpdate()
{
	int temp_disp;

	if(mode == 0)             /* 时钟模式 */
	{
		/* 显示日期 */
		LcdShowNumAt(0x82, BCD2DEC(TIME[6]));  /* 年 */
		LcdShowNumAt(0x85, BCD2DEC(TIME[4]));  /* 月 */
		LcdShowNumAt(0x88, BCD2DEC(TIME[3]));  /* 日 */

		/* 显示星期 */
		LcdWriteCom(0x8C);
		ShowLcdWeek(TIME[5] & 0x07);

		/* 显示时间 */
		LcdShowNumAt(0xC0, BCD2DEC(TIME[2]));  /* 时 */
		LcdShowNumAt(0xC3, BCD2DEC(TIME[1]));  /* 分 */
		LcdShowNumAt(0xC6, BCD2DEC(TIME[0]));  /* 秒 */

		/* 显示温度：格式 "-12.3C" 或 " 25.0C" */
		LcdWriteCom(0xC9);
		temp_disp = temperature;
		if(temp_disp < 0)
		{
			LcdWriteData('-');
			temp_disp = -temp_disp;
		}
		else
		{
			LcdWriteData(' ');      /* 正数前留空格 */
		}
		LcdWriteData('0' + temp_disp / 100);          /* 十位 */
		LcdWriteData('0' + (temp_disp / 10) % 10);    /* 个位 */
		LcdWriteData('.');                             /* 小数点 */
		LcdWriteData('0' + temp_disp % 10);            /* 十分位 */
		LcdWriteData('C');                             /* 单位 */
	}
	else if(mode == 2)        /* 秒表模式 */
	{
		/* 居中显示，位置 0xC2/0xC5/0xC8/0xCB */
		LcdShowNumAt(0xC2, sw_sec / 3600);            /* 小时 */
		LcdShowNumAt(0xC5, (sw_sec % 3600) / 60);     /* 分钟 */
		LcdShowNumAt(0xC8, sw_sec % 60);              /* 秒钟 */
		LcdShowNumAt(0xCB, sw_csec);                  /* 百分秒 */
	}
}


/*========================== 按键扫描 ==========================*/

/*
 * KeyScan：按键扫描处理
 * 功能：根据当前模式处理按键事件
 *       时钟模式：K1=设置时间，K2=切换到秒表
 *       秒表模式：K3=启动/暂停，K4=清零（暂停时），K2=返回时钟
 */
void KeyScan()
{
	if(mode == 0)             /* 时钟模式 */
	{
		if(K1_Pressed())      /* K1：进入时间设置 */
		{
			SetTime();
			last_mode = 0xFF;   /* 强制刷新 LCD */
		}
		if(K2_Pressed())      /* K2：切换到秒表 */
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
	else if(mode == 2)        /* 秒表模式 */
	{
		if(K3_Pressed())      /* K3：启动/暂停秒表 */
		{
			sw_running = !sw_running;
			TR1 = sw_running;       /* TR1 控制定时器 1 的启停 */
		}
		if(K4_Pressed())      /* K4：暂停时清零 */
		{
			if(!sw_running)
			{
				sw_sec = 0;
				sw_csec = 0;
			}
		}
		if(K2_Pressed())      /* K2：返回时钟模式 */
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


/*========================== 时间设置 ==========================*/

/*
 * SetTime：时间和日期设置
 * 功能：对年、月、日、时、分、秒逐一进行调节，
 *       星期通过 K3/K4 手动调节，完成后全部写入 DS1302。
 * 操作：K1 切换调节字段，K3/K4 加减，调节完秒后自动写入。
 */
void SetTime()
{
	unsigned char y, mo, d, w, h, mi, s;

	/* 将 TIME 数组中的 BCD 码转换为十进制 */
	y  = BCD2DEC(TIME[6]);
	mo = BCD2DEC(TIME[4]);
	d  = BCD2DEC(TIME[3]);
	w  = TIME[5] & 0x07;
	h  = BCD2DEC(TIME[2]);
	mi = BCD2DEC(TIME[1]);
	s  = BCD2DEC(TIME[0]);

	/* 绘制设置界面 */
	LcdWriteCom(0x80);
	LcdWriteStr("20  -  -        ");   /* 第一行：20YY-MM-DD */
	LcdWriteCom(0xC0);
	LcdWriteStr("  :  :          ");   /* 第二行：HH:MM:SS */

	/* 显示当前时间 */
	LcdShowNumAt(0x82, y);             /* 年 */
	LcdShowNumAt(0x85, mo);            /* 月 */
	LcdShowNumAt(0x88, d);             /* 日 */
	LcdWriteCom(0x8C); ShowLcdWeek(w); /* 星期 */
	LcdShowNumAt(0xC0, h);             /* 时 */
	LcdShowNumAt(0xC3, mi);            /* 分 */
	LcdShowNumAt(0xC6, s);             /* 秒 */

	/* 逐一调节数值（K3/K4 加减，K1 跳到下一项） */
	AdjustValue(&y,  99, 0x82, 0x82);   /* 年 */
	AdjustValue(&mo, 12, 0x85, 0x85);   /* 月 */
	AdjustValue(&d,  31, 0x88, 0x88);   /* 日 */
	AdjustValue(&h,  23, 0xC0, 0xC0);   /* 时 */
	AdjustValue(&mi, 59, 0xC3, 0xC3);   /* 分 */
	AdjustValue(&s,  59, 0xC6, 0xC6);   /* 秒 */

	/* 手动调节星期 */
	LcdWriteCom(0x8E); LcdWriteCom(0x0f);   /* 光标在星期最后一位 */
	while(1)
	{
		if(K3_Pressed())
		{
			if(w < 7) w++; else w = 1;
			LcdWriteCom(0x8C); ShowLcdWeek(w);
			LcdWriteCom(0x8E); LcdWriteCom(0x0f);
		}
		if(K4_Pressed())
		{
			if(w > 1) w--; else w = 7;
			LcdWriteCom(0x8C); ShowLcdWeek(w);
			LcdWriteCom(0x8E); LcdWriteCom(0x0f);
		}
		if(K1_Pressed()) { break; }
	}

	/* 将调整后的值转回 BCD 码写入 TIME 数组 */
	TIME[6] = DEC2BCD(y);
	TIME[4] = DEC2BCD(mo);
	TIME[3] = DEC2BCD(d);
	TIME[5] = w;
	TIME[2] = DEC2BCD(h);
	TIME[1] = DEC2BCD(mi);
	TIME[0] = DEC2BCD(s);

	/* 写入 DS1302 */
	Ds1302Write(0x8E, 0x00);       /* 关闭写保护 */
	Ds1302Write(0x80, TIME[0]);    /* 秒 */
	Ds1302Write(0x82, TIME[1]);    /* 分 */
	Ds1302Write(0x84, TIME[2]);    /* 时 */
	Ds1302Write(0x86, TIME[3]);    /* 日 */
	Ds1302Write(0x88, TIME[4]);    /* 月 */
	Ds1302Write(0x8a, TIME[5]);    /* 周 */
	Ds1302Write(0x8c, TIME[6]);    /* 年 */
	Ds1302Write(0x8E, 0x80);       /* 开启写保护 */

	/* 恢复显示 */
	LcdWriteCom(0x0c);             /* 关闭光标 */
	last_mode = 0xFF;              /* 强制刷新 LCD */
}


/*========================= 定时器初始化 =========================*/

/*
 * Timer1Init：初始化定时器 1
 * 功能：配置 T1 为模式 1（16 位定时器），启用中断
 *       用于秒表计时，每 10ms 中断一次
 */
void Timer1Init()
{
	TMOD &= 0x0F;      /* 清空 T1 相关位 */
	TMOD |= 0x10;      /* 设置 T1 为模式 1（16 位定时器） */
	ET1 = 1;           /* 使能 T1 中断 */
	EA  = 1;           /* 开启总中断 */
}


/*====================== 定时器 1 中断服务 ======================*/

/*
 * Timer1：定时器 1 中断服务函数
 * 功能：每 10ms 中断一次（12MHz 晶振），秒表运行时累加百分秒
 *       百分秒到 100 时进位 1 秒
 */
void Timer1() interrupt 3
{
	TH1 = 0xD8;        /* 重装初值：定时 10ms */
	TL1 = 0xF0;

	if(sw_running)      /* 秒表正在运行时才累加 */
	{
		sw_csec++;
		if(sw_csec >= 100)   /* 百分秒满 100 进 1 秒 */
		{
			sw_csec = 0;
			sw_sec++;
		}
	}
}


/*======================= 温度采集 ========================*/

/*
 * TempPros：温度采集状态机（非阻塞式）
 * 功能：分两步完成温度采集，避免长时间阻塞主循环
 *   state=0：发送转换命令（复位→0xCC→0x44），切换到等待状态
 *   state=1：等待约 800ms（40 次主循环），然后读取温度
 *           （复位→0xCC→0xBE→读 2 字节→计算→回 state=0）
 * 说明：DS18B20 在 12 位分辨率下需要最长 750ms 完成转换，
 *       等待期间主循环正常运行（按键响应、显示刷新不受影响）。
 */
void TempPros()
{
	unsigned char tmL, tmH;
	int raw;
	unsigned char sign = 0;

	if(temp_state == 0)
	{
		/* 第一步：启动温度转换 */
		Ds18b20Init();
		Ds18b20WriteByte(0xCC);     /* 跳过 ROM（单设备模式） */
		Ds18b20WriteByte(0x44);     /* 启动温度转换命令 */
		temp_state = 1;              /* 切换到等待状态 */
		temp_wait = 0;
	}
	else if(temp_state == 1)
	{
		/* 第二步：等待转换完成 */
		temp_wait++;
		if(temp_wait >= 40)          /* 约 800ms（每循环 ~20ms） */
		{
			Ds18b20Init();
			Ds18b20WriteByte(0xCC); /* 跳过 ROM */
			Ds18b20WriteByte(0xBE); /* 读取暂存器命令 */
			tmL = Ds18b20ReadByte();/* 读取温度低字节 */
			tmH = Ds18b20ReadByte();/* 读取温度高字节 */
			raw = (tmH << 8) | tmL; /* 合并为 16 位原始值 */

			/* 负数处理：符号位为 1 时取反加 1 */
			if(raw & 0x8000)
			{
				raw = ~raw + 1;
				sign = 1;
			}

			/* 转换为放大 10 倍的温度值：原始值 × 10 / 16 */
			temperature = raw * 10 / 16;
			if(sign)
				temperature = -temperature;

			temp_state = 0;          /* 回到空闲状态，准备下一次采集 */
		}
	}
}


/*======================== 闹钟/蜂鸣器 ========================*/

/*
 * AlarmJudge：闹钟判断函数（蜂鸣器已损坏，保留为空）
 */
void AlarmJudge(void)
{
}

/*
 * AlarmBuzPro：蜂鸣器驱动函数（蜂鸣器已损坏，保留为空）
 */
void AlarmBuzPro(void)
{
}


/*========================= 延时函数 ==========================*/

/*
 * Delay10ms：软件延时约 10ms（12MHz 晶振，12T 模式）
 * 用于按键消抖
 */
void Delay10ms(void)
{
	unsigned char a, b, c;
	for(c = 1; c > 0; c--)
		for(b = 38; b > 0; b--)
			for(a = 130; a > 0; a--);
}
