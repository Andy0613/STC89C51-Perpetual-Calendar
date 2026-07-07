/*******************************************************************************
* ʵ����     : ʱ��+�¶�+����+����
* K1=P3^1: ���ӣ�����
* K2=P3^0: ������
* K3=P3^2: + / ������ʼ/��ͣ
* K4=P3^3: - / ��������
* Buzzer=P2^0
*******************************************************************************/

#include<reg51.h>
#include<intrins.h>
#include"lcd.h"
#include"ds1302.h"
#include"ds18b20.h"

sbit K1=P3^1;
sbit K2=P3^0;
sbit K3=P3^2;
sbit K4=P3^3;
sbit Buzzer=P2^0;

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

unsigned char mode = 0;
unsigned char last_mode = 0xFF;
int temperature = 0;
unsigned char temp_cnt = 0;

unsigned char alarm_hour = 6;
unsigned char alarm_min = 0;
unsigned char alarm_on = 0;
unsigned char alarm_buz = 0;
unsigned int alarm_buz_cnt = 0;

unsigned int sw_sec = 0;
unsigned char sw_csec = 0;
bit sw_running = 0;

/*******************************************************************************
* main
*******************************************************************************/
void main()
{
	Timer1Init();
	LcdInit();

	Buzzer = 0;
	Delay10ms();Delay10ms();Delay10ms();Delay10ms();Delay10ms();
	Delay10ms();Delay10ms();Delay10ms();Delay10ms();Delay10ms();	// 100ms
	Buzzer = 1;

	Ds1302Init();
	DS18B20_SetResolution(0x3F);
	DS18B20_StartConvert();

	alarm_hour = Ds1302Read(0xC1);
	if(alarm_hour > 23) alarm_hour = 6;
	alarm_min = Ds1302Read(0xC3);
	if(alarm_min > 59) alarm_min = 0;
	alarm_on = Ds1302Read(0xC5);
	if(alarm_on > 1) alarm_on = 0;

	while(1)
	{
		Ds1302ReadTime();

		temp_cnt++;
		if(temp_cnt >= 5)
		{
			temp_cnt = 0;
			TempPros();
		}

		if(mode != last_mode)
		{
			LcdFramework();
			last_mode = mode;
		}

		AlarmJudge();
		AlarmBuzPro();
		KeyScan();
		LcdUpdate();
	}
}

void ShowLcdNum(unsigned char num)
{
	LcdWriteData('0' + num / 10);
	LcdWriteData('0' + num % 10);
}

void ShowLcdWeek(unsigned char week)
{
	switch(week)
	{
		case 1: LcdWriteData('S'); LcdWriteData('u'); LcdWriteData('n'); break;
		case 2: LcdWriteData('M'); LcdWriteData('o'); LcdWriteData('n'); break;
		case 3: LcdWriteData('T'); LcdWriteData('u'); LcdWriteData('e'); break;
		case 4: LcdWriteData('W'); LcdWriteData('e'); LcdWriteData('d'); break;
		case 5: LcdWriteData('T'); LcdWriteData('h'); LcdWriteData('u'); break;
		case 6: LcdWriteData('F'); LcdWriteData('r'); LcdWriteData('i'); break;
		case 7: LcdWriteData('S'); LcdWriteData('a'); LcdWriteData('t'); break;
	}
}

/*******************************************************************************
* LcdFramework - д��̬֡��
*******************************************************************************/
void LcdFramework()
{
	if(mode == 0)
	{
		LcdWriteCom(0x0c);

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
	else if(mode == 2)
	{
		LcdWriteCom(0x0c);

		LcdWriteCom(0x80);
		LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');
		LcdWriteData('S'); LcdWriteData('t'); LcdWriteData('o');
		LcdWriteData('p'); LcdWriteData(' ');
		LcdWriteData('W'); LcdWriteData('a'); LcdWriteData('t');
		LcdWriteData('c'); LcdWriteData('h');
		LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');

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

/*******************************************************************************
* LcdUpdate - ������ֵ
*******************************************************************************/
void LcdUpdate()
{
	unsigned char t;
	int temp_disp;

	if(mode == 0)
	{
		LcdWriteCom(0x82);
		ShowLcdNum((TIME[6]>>4)*10 + (TIME[6]&0x0f));

		LcdWriteCom(0x85);
		ShowLcdNum((TIME[4]>>4)*10 + (TIME[4]&0x0f));

		LcdWriteCom(0x88);
		ShowLcdNum((TIME[3]>>4)*10 + (TIME[3]&0x0f));

		LcdWriteCom(0x8C);
		ShowLcdWeek(TIME[5] & 0x07);

		LcdWriteCom(0xC0);
		ShowLcdNum((TIME[2]>>4)*10 + (TIME[2]&0x0f));

		LcdWriteCom(0xC3);
		ShowLcdNum((TIME[1]>>4)*10 + (TIME[1]&0x0f));

		LcdWriteCom(0xC6);
		ShowLcdNum((TIME[0]>>4)*10 + (TIME[0]&0x0f));

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
	else if(mode == 2)
	{
		t = sw_sec / 3600;
		LcdWriteCom(0xC0);
		LcdWriteData('0' + t / 10);
		LcdWriteData('0' + t % 10);

		LcdWriteCom(0xC3);
		t = (sw_sec % 3600) / 60;
		LcdWriteData('0' + t / 10);
		LcdWriteData('0' + t % 10);

		LcdWriteCom(0xC6);
		LcdWriteData('0' + (sw_sec % 60) / 10);
		LcdWriteData('0' + (sw_sec % 60) % 10);

		LcdWriteCom(0xC9);
		LcdWriteData('0' + sw_csec / 10);
		LcdWriteData('0' + sw_csec % 10);
	}
}

/*******************************************************************************
* KeyScan
*******************************************************************************/
void KeyScan()
{
	if(mode == 0)
	{
		if(K1 == 0)
		{
			Delay10ms();
			if(K1 == 0)
			{
				while(!K1);
				SetAlarm();
				last_mode = 0xFF;
				ReturnSound();
			}
		}
		if(K2 == 0)
		{
			Delay10ms();
			if(K2 == 0)
			{
				while(!K2);
				KeySound();
				mode = 2;
				sw_running = 0;
				sw_sec = 0;
				sw_csec = 0;
			}
		}
	}
	else if(mode == 2)
	{
		if(K3 == 0)
		{
			Delay10ms();
			if(K3 == 0)
			{
				while(!K3);
				KeySound();
				sw_running = !sw_running;
				TR1 = sw_running;
			}
		}
		if(K4 == 0)
		{
			Delay10ms();
			if(K4 == 0)
			{
				while(!K4);
				KeySound();
				if(!sw_running)
				{
					sw_sec = 0;
					sw_csec = 0;
				}
			}
		}
		if(K2 == 0)
		{
			Delay10ms();
			if(K2 == 0)
			{
				while(!K2);
				mode = 0;
				TR1 = 0;
				sw_running = 0;
				sw_sec = 0;
				sw_csec = 0;
				ReturnSound();
			}
		}
	}
}

/*******************************************************************************
* SetAlarm - 设置闹钟（阻塞式，仿照示例代码KeyScanf2）
*******************************************************************************/
void SetAlarm()
{
	LcdWriteCom(0x80);
	LcdWriteData('A'); LcdWriteData('l'); LcdWriteData('a'); LcdWriteData('r');
	LcdWriteData('m'); LcdWriteData(' '); LcdWriteData('C'); LcdWriteData('l');
	LcdWriteData('o'); LcdWriteData('c'); LcdWriteData('k'); LcdWriteData(' ');
	LcdWriteData('S'); LcdWriteData('e'); LcdWriteData('t'); LcdWriteData(' ');

	LcdWriteCom(0xC0);
	LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');
	LcdWriteData(' '); LcdWriteData(':'); LcdWriteData(' '); LcdWriteData(' ');
	LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');
	LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' '); LcdWriteData(' ');

	LcdWriteCom(0xC3);
	LcdWriteData('0' + alarm_hour / 10);
	LcdWriteData('0' + alarm_hour % 10);
	LcdWriteCom(0xC6);
	LcdWriteData('0' + alarm_min / 10);
	LcdWriteData('0' + alarm_min % 10);
	LcdWriteCom(0xCA);
	if(alarm_on) { LcdWriteData(' '); LcdWriteData('O'); LcdWriteData('N'); }
	else { LcdWriteData('O'); LcdWriteData('F'); LcdWriteData('F'); }

	LcdWriteCom(0xC4);
	LcdWriteCom(0x0f);
	while(1)
	{
		if(K3 == 0)
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
		if(K4 == 0)
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
		if(K1 == 0)
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

	LcdWriteCom(0xC7);
	LcdWriteCom(0x0f);
	while(1)
	{
		if(K3 == 0)
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
		if(K4 == 0)
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
		if(K1 == 0)
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

	LcdWriteCom(0xCC);
	LcdWriteCom(0x0f);
	while(1)
	{
		if(K3 == 0)
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
		if(K4 == 0)
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
		if(K1 == 0)
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

	LcdWriteCom(0x0c);
	Ds1302Write(0x8E, 0x00);
	Ds1302Write(0xC0, alarm_hour);
	Ds1302Write(0xC2, alarm_min);
	Ds1302Write(0xC4, alarm_on);
	Ds1302Write(0x8E, 0x80);
}

/*******************************************************************************
* AlarmJudge
*******************************************************************************/
void AlarmJudge()
{
	unsigned char h, m, s;
	h = (TIME[2] >> 4) * 10 + (TIME[2] & 0x0f);
	m = (TIME[1] >> 4) * 10 + (TIME[1] & 0x0f);
	s = (TIME[0] >> 4) * 10 + (TIME[0] & 0x0f);

	if(mode == 0 && alarm_on && !alarm_buz)
	{
		if(h == alarm_hour && m == alarm_min && s < 5)
		{
			alarm_buz = 1;
			alarm_buz_cnt = 0;
		}
	}

	if(s == 59)
		alarm_buz = 0;
}

void AlarmBuzPro()
{
	if(!alarm_buz)
		return;

	if(!K1 || !K2 || !K3 || !K4)
	{
		DelayMs(10);
		if(!K1 || !K2 || !K3 || !K4)
		{
			alarm_buz = 0;
			Buzzer = 1;
			return;
		}
	}

	alarm_buz_cnt++;
	if(alarm_buz_cnt > 50)
	{
		alarm_buz = 0;
		Buzzer = 1;
		return;
	}

	Buzzer = 0;
	DelayMs(50);
	Buzzer = 1;
	DelayMs(50);
}

/*******************************************************************************
* Timer1Init
*******************************************************************************/
void Timer1Init()
{
	TMOD &= 0x0F;
	TMOD |= 0x10;
	ET1 = 1;
	EA = 1;
}

/*******************************************************************************
* Timer1 interrupt - 10ms
*******************************************************************************/
void Timer1() interrupt 3
{
	TH1 = 0xD8;
	TL1 = 0xF0;

	if(sw_running)
	{
		sw_csec++;
		if(sw_csec >= 100)
		{
			sw_csec = 0;
			sw_sec++;
		}
	}
}

/*******************************************************************************
* TempPros
*******************************************************************************/
void TempPros()
{
	int raw;
	unsigned char sign = 0;

	if(!DS18B20_GetTemp(&raw))
		return;

	if(raw & 0x8000)
	{
		raw = ~raw + 1;
		sign = 1;
	}

	temperature = raw * 10 / 16;
	if(sign)
		temperature = -temperature;

	DS18B20_StartConvert();
}

/*******************************************************************************
* Delay10ms
*******************************************************************************/
void DelayMs(unsigned int ms)
{
	unsigned int i, j;
	for(i=0; i<ms; i++)
		for(j=0; j<112; j++);
}

void KeySound()
{
	Buzzer = 0;
	DelayMs(40);
	Buzzer = 1;
}

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

void Delay10ms(void)
{
	unsigned char a,b,c;
	for(c=1;c>0;c--)
		for(b=38;b>0;b--)
			for(a=130;a>0;a--);
}
