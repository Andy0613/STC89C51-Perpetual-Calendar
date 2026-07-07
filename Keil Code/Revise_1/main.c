/*******************************************************************************
* ʵ����			   : ������ʵ��
* ʹ�õ�IO	     : 
* ʵ��Ч��       :1602��ʾʱ�ӣ���K3����ʱ�����ã���K1ѡ�����õ�ʱ�������£���K2ѡ��
*ѡ�����ü�1�� 
*	ע��					 ��
*******************************************************************************/

#include<reg51.h>
#include"lcd.h"
#include"ds1302.h"
#include"ds18b20.h"

sbit K1=P3^1;
sbit K2=P3^0;
sbit K3=P3^2;
sbit K4=P3^3;	 //�ܽŽ���

void Int0Configuration();
void LcdDisplay();
void TempPros(void);
unsigned char SetState,SetPlace;
int temperature = 0;
unsigned char temp_cnt = 0;
void Delay10ms(void);   //��� 0us
/*******************************************************************************
* ������         : main
* ��������		   : ������
* ����           : ��
* ���         	 : ��
*******************************************************************************/

void main()
{
	unsigned char i;
	Int0Configuration();
	LcdInit();
	Ds1302Init();
	DS18B20_SetResolution(0x3F);	// 10λ���ȣ�ת��ʱ��187.5ms
	DS18B20_StartConvert();
	while(1)
	{	
		if(SetState==0)
		{
			Ds1302ReadTime();
		}
		else
		{
			if(K1==0)		//��ⰴ��K1�Ƿ���
			{
				Delay10ms();	//��������
				if(K1==0)
				{
					SetPlace++;
					if(SetPlace>=7)
						SetPlace=0;					
				}

				while((i<50)&&(K1==0))	 //��ⰴ���Ƿ��ɿ�
				{
					Delay10ms();
					i++;
				}
				i=0;
			}
			if(K2==0)		//��ⰴ��K2�Ƿ���
			{
				Delay10ms();	//��������
				if(K2==0)
				{
					TIME[SetPlace]++;
					if((TIME[SetPlace]&0x0f)>9)					 //����BCD�롣
					{
						TIME[SetPlace]=TIME[SetPlace]+6;
					}
					if((TIME[SetPlace]>=0x60)&&(SetPlace<2))		//����ֻ�ܵ�59
					{
						TIME[SetPlace]=0;
					}
					if((TIME[SetPlace]>=0x24)&&(SetPlace==2))		//Сʱֻ�ܵ�23
					{
						TIME[SetPlace]=0;
					}
					if((TIME[SetPlace]>=0x32)&&(SetPlace==3))		//��ֻ�ܵ�31
					{
						TIME[SetPlace]=0;	
					}
					if((TIME[SetPlace]>=0x13)&&(SetPlace==4))		//��ֻ�ܵ�12
					{
						TIME[SetPlace]=0;
					}	
					if((TIME[SetPlace]>=0x7)&&(SetPlace==5))		//��ֻ�ܵ�7
					{
						TIME[SetPlace]=1;
					}		
				}
				
				while((i<50)&&(K2==0))	 //��ⰴ���Ƿ��ɿ�
				{
					Delay10ms();
					i++;
				}
				i=0;
				
			}						
		}
		
		temp_cnt++;
		if(temp_cnt >= 5)
		{
			temp_cnt = 0;
			TempPros();
		}
		
		LcdDisplay();	
	}
	
}
/*******************************************************************************
* ������         : LcdDisplay()
* ��������		   : ��ʾ����
* ����           : ��
* ���         	 : ��
*******************************************************************************/

void LcdDisplay()
{
	int temp_disp;
	
	// Row 0: 20YY-MM-DD W
	LcdWriteCom(0x80);
	LcdWriteData('2');
	LcdWriteData('0');
	LcdWriteData('0'+TIME[6]/16);			//��
	LcdWriteData('0'+(TIME[6]&0x0f));
	LcdWriteData('-');
	LcdWriteData('0'+TIME[4]/16);			//��
	LcdWriteData('0'+(TIME[4]&0x0f));
	LcdWriteData('-');
	LcdWriteData('0'+TIME[3]/16);			//��
	LcdWriteData('0'+(TIME[3]&0x0f));
	LcdWriteCom(0x8B);
	LcdWriteData('0'+(TIME[5]&0x07));	//����

	// Row 1: HH:MM:SS xx.xC
	LcdWriteCom(0x80+0X40);
	LcdWriteData('0'+TIME[2]/16);				//ʱ
	LcdWriteData('0'+(TIME[2]&0x0f));				 
	LcdWriteData(':');
	LcdWriteData('0'+TIME[1]/16);				//��
	LcdWriteData('0'+(TIME[1]&0x0f));	
	LcdWriteData(':');
	LcdWriteData('0'+TIME[0]/16);				//��
	LcdWriteData('0'+(TIME[0]&0x0f));

	// Temperature display at position 9
	LcdWriteCom(0x80+0x40+0x09);
	temp_disp = temperature;
	if(temp_disp < 0)
	{
		LcdWriteData('-');
		temp_disp = -temp_disp;
	}
	else
	{
		LcdWriteData(' ');
	}
	LcdWriteData('0' + temp_disp / 100);
	LcdWriteData('0' + (temp_disp / 10) % 10);
	LcdWriteData('.');
	LcdWriteData('0' + temp_disp % 10);
	LcdWriteData('C');		 
}
/*******************************************************************************
* ������         : TempPros
* ��������		   : 读取DS18B20温度并启动下一次转换
* ����           : ��
* ���         	 : ��
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
* ������         : Int0Configuration()
* ��������		   : �����ⲿ�ж�0
* ����           : ��
* ���         	 : ��
*******************************************************************************/

void Int0Configuration()
{
	//����INT0
	IT0=1;//�����س�����ʽ���½��أ�
	EX0=1;//��INT0���ж�������
	EA=1;//�����ж�	
}
/*******************************************************************************
* ������         : Int0() 
* ��������		   : �ⲿ�ж�0 �жϺ���
* ����           : ��
* ���         	 : ��
*******************************************************************************/

void Int0() interrupt 0		 
{
	Delay10ms();
	if(K3==0)
	{
		if(SetState == 1)		// �˳�����ģʽʱ������ʱ�䱣�浽DS1302
		{
			Ds1302WriteTime();
		}
		SetState=~SetState;
		SetPlace=0;
	}
}
/*******************************************************************************
* ������         : Delay10ms
* ��������		   : ��ʱ��������ʱ10ms
* ����           : ��
* ���         	 : ��
*******************************************************************************/
void Delay10ms(void)   //��� 0us
{
    unsigned char a,b,c;
    for(c=1;c>0;c--)
        for(b=38;b>0;b--)
            for(a=130;a>0;a--);
}