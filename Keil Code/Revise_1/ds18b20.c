#include "ds18b20.h"

void DS18B20_Reset(void)
{
	uchar i;
	DS18B20_DQ = 0;
	for(i=0; i<200; i++);
	DS18B20_DQ = 1;
	for(i=0; i<100; i++);
}

void DS18B20_WriteByte(uchar dat)
{
	uchar i, j;
	for(i=0; i<8; i++)
	{
		DS18B20_DQ = 0;
		_nop_();
		if(dat & 0x01)
			DS18B20_DQ = 1;
		else
			DS18B20_DQ = 0;
		for(j=0; j<30; j++);
		DS18B20_DQ = 1;
		_nop_();
		_nop_();
		dat >>= 1;
	}
}

uchar DS18B20_ReadByte(void)
{
	uchar i, j, dat = 0;
	for(i=0; i<8; i++)
	{
		dat >>= 1;
		DS18B20_DQ = 0;
		_nop_();
		DS18B20_DQ = 1;
		_nop_();
		_nop_();
		if(DS18B20_DQ)
			dat |= 0x80;
		for(j=0; j<20; j++);
	}
	return dat;
}

void DS18B20_StartConvert(void)
{
	DS18B20_Reset();
	DS18B20_WriteByte(0xCC);
	DS18B20_WriteByte(0x44);
}

void DS18B20_SetResolution(uchar res)
{
	// res: 0x1F=9bit, 0x3F=10bit, 0x5F=11bit, 0x7F=12bit
	DS18B20_Reset();
	DS18B20_WriteByte(0xCC);	// Skip ROM
	DS18B20_WriteByte(0x4E);	// Write Scratchpad
	DS18B20_WriteByte(0x00);	// TH (user byte 1)
	DS18B20_WriteByte(0x00);	// TL (user byte 2)
	DS18B20_WriteByte(res);		// Configuration register
	DS18B20_Reset();
	DS18B20_WriteByte(0xCC);	// Skip ROM
	DS18B20_WriteByte(0x48);	// Copy Scratchpad to EEPROM
}

bit DS18B20_GetTemp(int *temp)
{
	uchar tempL, tempH;
	int raw;

	DS18B20_Reset();
	DS18B20_WriteByte(0xCC);
	DS18B20_WriteByte(0xBE);

	tempL = DS18B20_ReadByte();
	tempH = DS18B20_ReadByte();

	raw = (tempH << 8) | tempL;
	*temp = raw;

	return 1;
}
