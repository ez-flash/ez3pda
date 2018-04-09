#include <agb.h>
#include "inram.h"
#include "hard.h"
#include "shell.h"

extern u32 gl_currentpage ;
extern EZ4_CARTTYPE g_EZ4CardType;
extern u32 gl_norsize;
void _SetEZ2Control(u32* FuncAddr ,vu16 control)
{
	*(vu16 *)0x9fe0000 = 0xd200;
	*(vu16 *)0x8000000 = 0x1500;
	*(vu16 *)0x8020000 = 0xd200;
	*(vu16 *)0x8040000 = 0x1500; 
	*(vu16 *)0x9E00000 = control;
	*(vu16 *)0x9fc0000 = 0x1500;
}

void		OpenWrite()
{
	*(vu16 *)0x9fe0000 = 0xd200;
	*(vu16 *)0x8000000 = 0x1500;
	*(vu16 *)0x8020000 = 0xd200;
	*(vu16 *)0x8040000 = 0x1500;
	*(vu16 *)0x9C40000 = 0x1500;
	*(vu16 *)0x9fc0000 = 0x1500;
}

void		CloseWrite()
{
	*(vu16 *)0x9fe0000 = 0xd200;
	*(vu16 *)0x8000000 = 0x1500;
	*(vu16 *)0x8020000 = 0xd200;
	*(vu16 *)0x8040000 = 0x1500;
	*(vu16 *)0x9C40000 = 0xd200;
	*(vu16 *)0x9fc0000 = 0x1500;
}

void  OpenRamWrite()
{
	*(vu16 *)0x9fe0000 = 0xd200;
	*(vu16 *)0x8000000 = 0x1500;
	*(vu16 *)0x8020000 = 0xd200;
	*(vu16 *)0x8040000 = 0x1500;
	if(g_EZ4CardType ==  EZ4_1Chip)
		*(vu16 *)0x9C40000 = 0x5A00; 
	else
		*(vu16 *)0x9C40000 = 0xA500;
	*(vu16 *)0x9fc0000 = 0x1500;
}

void CloseRamWrite()
{
	*(vu16 *)0x9fe0000 = 0xd200;
	*(vu16 *)0x8000000 = 0x1500;
	*(vu16 *)0x8020000 = 0xd200;
	*(vu16 *)0x8040000 = 0x1500;
	*(vu16 *)0x9C40000 = 0xA200;
	*(vu16 *)0x9fc0000 = 0x1500;
}

void Clock_Enable()
{
	*(vu16*)CLOCK_DEVICE = 1 ;
}

void Clock_Disable()
{
	*(vu16*)CLOCK_DEVICE = 0 ;
}

void Clock_EnableRead()
{
	*(vu16*)CLOCK_CONTROL = CLOCK_CSOUT+0+CLOCK_SCKOUT ;
}

void Clock_EnableWrite()
{
	*(vu16*)CLOCK_CONTROL = CLOCK_CSOUT+CLOCK_SIOOUT+CLOCK_SCKOUT ;
}

void Clock_DisableReadWrite()
{  
	*(vu16*)CLOCK_CONTROL =0 ; 
}

u8 Clock_ReadBit()
{
	register vu16 bb =0;//	cs	  data clk
	*(vu16*)CLOCK_DATA = CLOCK_CS + 0 + 0 ;
	*(vu16*)CLOCK_DATA = CLOCK_CS + 0 + 1 ;
	bb = *(vu16*)CLOCK_DATA;
	bb = (bb&2)?1:0 ;
	return bb&0xFF ;
}

void Clock_WriteBit(u8 bb)
{
	bb = (bb&1)?2:0 ; //   cs     data  clk
	*(vu16*)CLOCK_DATA=CLOCK_CS+bb+0 ;
	*(vu16*)CLOCK_DATA=CLOCK_CS+bb+1 ;
}

u8 Clock_ReadByte()
{
	register u8 i = 0 ; 
	register u8 byReturn = 0 , bb =0 ;
	for( i=0;i<8;i++)
	{
		bb = Clock_ReadBit();	
		byReturn += bb<<i ;
	}
	return byReturn ;
}

u8 Clock_WriteByte(u8 bdata)
{
	register u8 i ;
	for(i=0;i<8;i++)
	{
		Clock_WriteBit((bdata>>i)&0x1);	
	}
}

u8 Clock_FromTime(u8 time)
{
	register u8 bb , dd ;
	bb = time & 0xF ;
	dd = (time>>4)&0xF ;
	return bb+dd*10 ;
}

u8 Clock_ToTime(u8 data )
{
	register u8 bb , dd ;
	bb = data%10 ;
	dd = data/10 ;
	return (dd<<4)+bb ;
}

void GetTime(CLOCK_TIME *cs)
{
	u8 year,month,day,week,hour,minute,second ;
	Clock_Enable();
	Clock_EnableWrite();
	//cs 从0 到1
	*(vu16*)CLOCK_DATA=0+CLOCK_SIO+CLOCK_SCK ;
	*(vu16*)CLOCK_DATA=CLOCK_CS+CLOCK_SIO+CLOCK_SCK ;
	//命令字
	Clock_WriteByte(0xA6);
	Clock_EnableRead();
	year = Clock_ReadByte();
	month = Clock_ReadByte();
	day = Clock_ReadByte();
	week = Clock_ReadByte();
	hour = Clock_ReadByte();
	minute = Clock_ReadByte();
	second = Clock_ReadByte();
	//关闭cs
	*(vu16*)CLOCK_DATA=0+CLOCK_SIO+CLOCK_SCK ;
	Clock_DisableReadWrite();
	Clock_Disable();
	//保存至结构中
	cs->year = Clock_FromTime(year);
	cs->month = Clock_FromTime(month);
	cs->data = Clock_FromTime(day);
	cs->week = Clock_FromTime(week);
	cs->hour = Clock_FromTime(hour&0x7F);
	cs->minute = Clock_FromTime(minute);
	cs->second = Clock_FromTime(second);
}

void SetTime(CLOCK_TIME *cs)
{
	u8 year,month,day,week,hour,minute,second ;
	year = Clock_ToTime(cs->year);
	month = Clock_ToTime(cs->month);
	day = Clock_ToTime(cs->data);
	week = Clock_ToTime(cs->week);
	hour = Clock_ToTime(cs->hour);
	minute = Clock_ToTime(cs->minute);
	second = Clock_ToTime(cs->second);
	if(cs->hour>=12)
		hour |= 0x80 ;

	Clock_Enable();
	Clock_EnableWrite();
	//cs 从0 到1
	*(vu16*)CLOCK_DATA=0+CLOCK_SIO+CLOCK_SCK ;
	*(vu16*)CLOCK_DATA=CLOCK_CS+CLOCK_SIO+CLOCK_SCK ;
	//命令字
	Clock_WriteByte(0x26);

	Clock_WriteByte(year);
	Clock_WriteByte(month);
	Clock_WriteByte(day);
	Clock_WriteByte(week);
	Clock_WriteByte(hour);
	Clock_WriteByte(minute);
	Clock_WriteByte(second);
	//关闭cs
	*(vu16*)CLOCK_DATA=0+CLOCK_SIO+CLOCK_SCK ;
	Clock_DisableReadWrite();
	Clock_Disable();
	//保存至结构中
}


void GetTime_Orignal(CLOCK_TIME *cs)
{
	u8 tmp , tmp2 ;
	Clock_Enable();
	Clock_EnableWrite();
	//cs 从0 到1
	*(vu16*)CLOCK_DATA=0+CLOCK_SIO+CLOCK_SCK ;
	*(vu16*)CLOCK_DATA=CLOCK_CS+CLOCK_SIO+CLOCK_SCK ;
	//命令字
	Clock_WriteByte(0xA6);
	Clock_EnableRead();
	cs->year = Clock_ReadByte();
	cs->month = Clock_ReadByte();
	cs->data = Clock_ReadByte();
	cs->week = Clock_ReadByte();
	cs->hour = Clock_ReadByte();
	cs->minute = Clock_ReadByte();
	cs->second = Clock_ReadByte();
	//关闭cs
	*(vu16*)CLOCK_DATA=0+CLOCK_SIO+CLOCK_SCK ;
	Clock_DisableReadWrite();
	Clock_Disable();
	cs->hour = cs->hour&0x7F ;

	/*
	tmp = cs->hour&0x80 ;
	tmp2= Clock_FromTime(cs->hour&0x7F);
	if(tmp)
	{		
		if(tmp2<=12)
			tmp2 += 12 ;
	}
	cs->hour = Clock_ToTime(tmp2);
	*/
}

void		Set24Hour()
{
	Clock_Enable();
	Clock_EnableWrite();
	//cs 从0 到1
	*(vu16*)CLOCK_DATA=0+CLOCK_SIO+CLOCK_SCK ;
	*(vu16*)CLOCK_DATA=CLOCK_CS+CLOCK_SIO+CLOCK_SCK ;
	//命令字
	Clock_WriteByte(0x46);
	Clock_WriteByte(0x40);

	//关闭cs
	*(vu16*)CLOCK_DATA=0+CLOCK_SIO+CLOCK_SCK ;
	Clock_DisableReadWrite();
	Clock_Disable();
}
/*
EZ4卡类型:
		VZ064 4*64M 
(410)	Light H6H6 2*128
(413)   Light turbo VZ128 384M
*/
EZ4_CARTTYPE	CheckEz4Card()
{ 
	vu16 id1,id2,id3,id4;
	u8  bPSRAM,data1,data2;;
	char path[256];
	IsSingleCard();
	if(g_EZ4CardType==EZ4_1Chip)
	{
		gl_norsize = 0x800000;
		g_EZ4CardType =  EZ4_1Chip;
		return EZ4_1Chip;
	}
	//首先确定是128M PSRAM还是256M PSRAM
	SetRompage(gl_currentpage);
	*((u8 *)_Ez3PsRAM) = 0x55;
	SetRompage(gl_currentpage+0x800);
	*((u8 *)_Ez3PsRAM) = 0xAA;	
	SetRompage(gl_currentpage);
	data1 = 	*((u8 *)_Ez3PsRAM);
	SetRompage(gl_currentpage+0x800);
	data2 = *((u8 *)_Ez3PsRAM);
	bPSRAM = 0;
	SetRompage(gl_currentpage);
	if(data1==0x55 && data2==0xAA)
	{
		//256M PSRAM
		bPSRAM=1;
		gl_norsize = 0x4000000;
		
	}
	else
	{
		gl_norsize = 0x2000000;
	}
	
	//下面是读flash ID,判断是哪种类型的EZ4卡
	if(bPSRAM==0)
	{
		//
		*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0x90 ;
		
		*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0x90 ;
/*			
		*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x2555*2)) = 0x90 ;

		*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x3555*2)) = 0x90 ;
*/		
		id1 = *((vu16 *)(FlashBase+0x2)) ;
		id2 = *((vu16 *)(FlashBase+0x2002)) ;
//		id3 = *((vu16 *)(FlashBase+0x4002)) ;
//		id4 = *((vu16 *)(FlashBase+0x6002)) ;
		if((id1!=0x227E)||(id2!=0x227E)/*||(id3!=0x227E)||(id4!=0x227E)*/)
		{
			g_EZ4CardType =EZ4_NoCard;
		}
		id1 = *((vu16 *)(FlashBase+0xE*2)) ;
		id2 = *((vu16 *)(FlashBase+0x100e*2)) ;
//		id3 = *((vu16 *)(FlashBase+0x200e*2)) ;
//		id4 = *((vu16 *)(FlashBase+0x300e*2)) ;
		if(id1==0x2202 && id2==0x2202 /*&& id3==0x2202 && id4==0x2202*/)
		{
			g_EZ4CardType =  EZ4_4Chip;			//VZ064
		}
		else
		{
//			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
//			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
//			*((vu16 *)(FlashBase+0x555*2)) = 0x90 ;
			
//			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
//			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
//			*((vu16 *)(FlashBase+0x1555*2)) = 0x90 ;
				
			
//			id1 = *((vu16 *)(FlashBase+0xE*2));
//			id2 = *((vu16 *)(FlashBase+0x100e*2)) ;
			if(id1==0x2218 && id2==0x2218)			//H6H6
				g_EZ4CardType =  EZ4_2Chip;
			else
				g_EZ4CardType = EZ4_NoCard;
				
		}		
			
	}
	else if(bPSRAM==1)
	{
		*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0x90 ;
		
		*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0x90 ;
			
//		*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
//		*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
//		*((vu16 *)(FlashBase+0x2555*2)) = 0x90 ;

//		*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
//		*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
//		*((vu16 *)(FlashBase+0x3555*2)) = 0x90 ;
		
		id1 = *((vu16 *)(FlashBase+0x2)) ;
		id2 = *((vu16 *)(FlashBase+0x2002)) ;
//		id3 = *((vu16 *)(FlashBase+0x4002)) ;
//		id4 = *((vu16 *)(FlashBase+0x6002)) ;
		if((id1!=0x227E)||(id2!=0x227E)/*||(id3!=0x227E)||(id4!=0x227E)*/)
			g_EZ4CardType =  EZ4_NoCard;
		id1 = *((vu16 *)(FlashBase+0xE*2)) ;
		id2 = *((vu16 *)(FlashBase+0x100e*2)) ;
//		id3 = *((vu16 *)(FlashBase+0x200e*2)) ;
//		id4 = *((vu16 *)(FlashBase+0x300e*2)) ;
		if(id1==0x2220 && id2==0x2220)
		{
			g_EZ4CardType =  EZ4_2Chip;	
			gl_norsize = 0x3000000;
		}
		if(id1==0x2218 && id2==0x2218)			//H6H6
		{
			g_EZ4CardType =  EZ4_2Chip;	
			gl_norsize = 0x3000000;
		}
	}		
	else 
	{
		g_EZ4CardType= EZ4_NoCard;
	}
	*((vu16 *)(FlashBase)) = 0xF0 ;
	*((vu16 *)(FlashBase+0x1000*2)) = 0xF0 ;
	SetRompage(gl_currentpage+0x4000);
	*((vu16 *)(FlashBase)) = 0xF0 ;
	*((vu16 *)(FlashBase+0x1000*2)) = 0xF0 ;
	SetRompage(gl_currentpage);
	return g_EZ4CardType;

}
u32 Nor_id ;
u32 chip_id()
{
	vu16 id1,id2,id3,id4;
	*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
	*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
	*((vu16 *)(FlashBase+0x555*2)) = 0x90 ;
	
	*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
	*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
	*((vu16 *)(FlashBase+0x1555*2)) = 0x90 ;
		
	*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
	*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
	*((vu16 *)(FlashBase+0x2555*2)) = 0x90 ;

	*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
	*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
	*((vu16 *)(FlashBase+0x3555*2)) = 0x90 ;
	
	id1 = *((vu16 *)(FlashBase+0x2)) ;
	id2 = *((vu16 *)(FlashBase+0x2002)) ; 
	id3 = *((vu16 *)(FlashBase+0x4002)) ;
	id4 = *((vu16 *)(FlashBase+0x6002)) ;
	if((id1!=0x227E)||(id2!=0x227E)||(id3!=0x227E)||(id4!=0x227E))
		goto try_fujistu ;
	id1 = *((vu16 *)(FlashBase+0xe*2)) ;
	id2 = *((vu16 *)(FlashBase+0x100e*2)) ;
	id3 = *((vu16 *)(FlashBase+0x200e*2)) ;
	id4 = *((vu16 *)(FlashBase+0x300e*2)) ;
	if((id1!=0x2202)||(id2!=0x2202)||(id3!=0x2202)||(id4!=0x2202))
		goto try_fujistu ;
	return Nor_id = 0x7E027E02 ;			//VZ064 Chip
try_fujistu:
	chip_reset();
	*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
	*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
	*((vu16 *)(FlashBase+0x555*2)) = 0x90 ;
	
	*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
	*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
	*((vu16 *)(FlashBase+0x1555*2)) = 0x90 ;
	
	id1 = *((vu16 *)(FlashBase+0x2)) ;
	id2 = *((vu16 *)(FlashBase+0x2002)) ;
	if((id1!=0x227E)|(id2!=0x227E))
		return 0xFFFFFFFF ;
	id1 = *((vu16 *)(FlashBase+0xe*2)) ;
	id2 = *((vu16 *)(FlashBase+0x100e*2)) ;
	if(id1==0x2220 && id2==0x2215)
	{
		Nor_id = 0x227E2221;
		return Nor_id;
	}
	if((id1!=0x2221)||(id2!=0x2221) && ((id1!=0x2218)||(id2!=0x2218)))
		return (id1<<16)|id2 ;
	return Nor_id = 0x227E2221 ;

}

u32 currentpage ;


void chip_erase(u32 id)
{
	vu16 v1,v2=0 ;
	SetRompage(gl_currentpage);
	currentpage = 0 ;
	if(id==0x227E2221)
	{//fujistu
		*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0x10 ;
		
		*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0x10 ;
		do
		{
			WaitingErase();
			v1 = *((vu16 *)(FlashBase+0x0)) ;
			v2 = *((vu16 *)(FlashBase+0x0)) ;
		}while(v1!=v2);
		do
		{
			WaitingErase();
			v1 = *((vu16 *)(FlashBase+0x2000)) ;
			v2 = *((vu16 *)(FlashBase+0x2000)) ;
		}while(v1!=v2);
		SetRompage(gl_currentpage+0x2000);
		*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0x10 ;
		
		*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0x10 ;
		do
		{
			WaitingErase();
			v1 = *((vu16 *)(FlashBase+0x0)) ;
			v2 = *((vu16 *)(FlashBase+0x0)) ;
		}while(v1!=v2);
		do
		{
			WaitingErase();
			v1 = *((vu16 *)(FlashBase+0x2000)) ;
			v2 = *((vu16 *)(FlashBase+0x2000)) ;
		}while(v1!=v2);
		SetRompage(gl_currentpage);
		currentpage = 0 ;
	}
	else if(id == 0x7E027E02)
	{//AMD
		*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x555*2)) = 0x10 ;
		
		*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0x10 ;

		do
		{
			WaitingErase();
			v1 = *((vu16 *)(FlashBase+0x0)) ;
			v2 = *((vu16 *)(FlashBase+0x0)) ;
		}while(v1!=v2);
		do
		{
			WaitingErase();
			v1 = *((vu16 *)(FlashBase+0x2000)) ;
			v2 = *((vu16 *)(FlashBase+0x2000)) ;
		}while(v1!=v2);
		
		*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x2555*2)) = 0x80 ;
		*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x2555*2)) = 0x10 ;

		*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x3555*2)) = 0x80 ;
		*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
		*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
		*((vu16 *)(FlashBase+0x3555*2)) = 0x10 ;
		
		do
		{
			WaitingErase();
			v1 = *((vu16 *)(FlashBase+0x4000)) ;
			v2 = *((vu16 *)(FlashBase+0x4000)) ;
		}while(v1!=v2);
		do
		{
			WaitingErase();
			v1 = *((vu16 *)(FlashBase+0x6000)) ;
			v2 = *((vu16 *)(FlashBase+0x6000)) ;
		}while(v1!=v2);
	}
}

void WriteFlash(u32 id ,u32 address, u8* buffer,u32 size)
{
	vu16 v1,v2=0 ;
	u32 mapaddress,m ,size2,j,lop;
	vu16 page;
	vu16* buf = (vu16*)buffer ;
	register u32 i ;
	mapaddress = address%FlashWindow ;
	page =  address/FlashWindow ;
	SetRompage(gl_currentpage+page*0x1000);
	currentpage = page;
	if(id==0x227E2221)
	{//fujistu,写2个
		if(size>0x4000)
		{
			size2 = size >>1 ;
			lop = 2;
		}
		else 
		{
			size2 = size  ;
			lop = 1;
		}
		for(j=0;j<lop;j++)
		{
			if(j)
			{
				mapaddress = (address+0x4000)%FlashWindow ;
				page =  (address+0x4000)/FlashWindow ;
				buf = (vu16*)(buffer+0x4000);

			}
			for(i=0;i<(size2>>2) ;i++)
			{
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0xA0 ;
				*((vu16 *)(FlashBase+mapaddress+i*2)) = buf[i];
				
				*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x1555*2)) = 0xA0 ;			
				*((vu16 *)(FlashBase+mapaddress+0x2000+i*2)) = buf[0x1000+i];
				do
				{
					v1 = *((vu16 *)(FlashBase+mapaddress+i)) ;
					v2 = *((vu16 *)(FlashBase+mapaddress+i)) ;
				}while(v1!=v2);
				do
				{
					v1 = *((vu16 *)(FlashBase+mapaddress+0x2000+i*2)) ;
					v2 = *((vu16 *)(FlashBase+mapaddress+0x2000+i*2)) ;
				}while(v1!=v2);
			}
		}
	}
	else if(id == 0x7E027E02)
	{//amd
		for(i=0;i<(size/8) ;i++)
		{
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0xA0 ;
			*((vu16 *)(FlashBase+mapaddress+i*2)) = buf[i];
			
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0xA0 ;			
			*((vu16 *)(FlashBase+mapaddress+0x2000+i*2)) = buf[0x1000+i];

			*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x2555*2)) = 0xA0 ;			
			*((vu16 *)(FlashBase+mapaddress+0x4000+i*2)) = buf[0x2000+i];

			*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x3555*2)) = 0xA0 ;			
			*((vu16 *)(FlashBase+mapaddress+0x6000+i*2)) = buf[0x3000+i];
			do
			{
				v1 = *((vu16 *)(FlashBase+mapaddress+i)) ;
				v2 = *((vu16 *)(FlashBase+mapaddress+i)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+mapaddress+0x2000+i*2)) ;
				v2 = *((vu16 *)(FlashBase+mapaddress+0x2000+i*2)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+mapaddress+0x4000+i*2)) ;
				v2 = *((vu16 *)(FlashBase+mapaddress+0x4000+i*2)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+mapaddress+0x6000+i*2)) ;
				v2 = *((vu16 *)(FlashBase+mapaddress+0x6000+i*2)) ;
			}while(v1!=v2);
		}
	}
}

void chip_reset()
{
	switch(g_EZ4CardType)
	{
	case EZ4_1Chip:
		*((vu16 *)(FlashBase)) = 0xF0 ;
		break;
	case EZ4_4Chip:
	case EZ4_4ChipPsram:
		*((vu16 *)(FlashBase)) = 0xF0 ;
		*((vu16 *)(FlashBase+0x1000*2)) = 0xF0 ;
		*((vu16 *)(FlashBase+0x2000*2)) = 0xF0 ;
		*((vu16 *)(FlashBase+0x3000*2)) = 0xF0 ;
		break;
	case EZ4_2Chip:
		*((vu16 *)(FlashBase)) = 0xF0 ;
		*((vu16 *)(FlashBase+0x1000*2)) = 0xF0 ;
		break;
	default:
		*((vu16 *)(FlashBase)) = 0xF0 ;
		*((vu16 *)(FlashBase+0x1000*2)) = 0xF0 ;
		*((vu16 *)(FlashBase+0x2000*2)) = 0xF0 ;
		*((vu16 *)(FlashBase+0x3000*2)) = 0xF0 ;
		break;
	}
}
void Block_Erase(u32 blockAdd)
{
	vu16 page,v1,v2;  
	u32 Address;
	u32 loop;
	page=gl_currentpage;
	Address=blockAdd;
	while(Address>=0x800000)
	{  
		Address-=0x800000;
		page+=0x1000;
	}
	
	SetRompage(page);
	v1=0;v2=1;
	switch(g_EZ4CardType)
	{
	case EZ4_1Chip:
		if(blockAdd==0) 
		{
			Address=0;
			for(loop=0;loop<0x10000;loop+=0x2000)
			{
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+Address+loop)) = 0x30 ;
				do
				{  
					
					v1 = *((vu16 *)(FlashBase+Address+loop)) ;
					v2 = *((vu16 *)(FlashBase+Address+loop)) ;
				}while(v1!=v2);
			}
			for(loop=0x10000;loop<0x40000;loop+=0x10000)
			{
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+Address+loop)) = 0x30 ;
				do
				{  
					
					v1 = *((vu16 *)(FlashBase+Address+loop)) ;
					v2 = *((vu16 *)(FlashBase+Address+loop)) ;
				}while(v1!=v2);
			}
		}
		else
		{
			for(loop=Address;loop<(Address+0x40000);loop+=0x10000)
			{
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+loop)) = 0x30 ;
				do
				{  
					
					v1 = *((vu16 *)(FlashBase+loop)) ;
					v2 = *((vu16 *)(FlashBase+loop)) ;
				}while(v1!=v2);
			}
		}
		break;
	case EZ4_4Chip:
	case EZ4_4ChipPsram:
		if( (blockAdd==0) || (blockAdd==0x1FC0000))
		{
			if(blockAdd==0)
				Address=0;
			else
				Address=0x7C0000;
			for(loop=0;loop<0x40000;loop+=0x8000)
			{
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+Address+loop)) = 0x30 ;
				
				*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+Address+loop+0x2000)) = 0x30 ;
				
				*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x2555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+Address+loop+0x4000)) = 0x30 ;
				
				*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ; 
				*((vu16 *)(FlashBase+0x3555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+Address+loop+0x6000)) = 0x30 ;
				do
				{  
					
					v1 = *((vu16 *)(FlashBase+Address+loop)) ;
					v2 = *((vu16 *)(FlashBase+Address+loop)) ;
				}while(v1!=v2);
				do
				{
					
					v1 = *((vu16 *)(FlashBase+Address+loop+0x2000)) ;
					v2 = *((vu16 *)(FlashBase+Address+loop+0x2000)) ;
				}while(v1!=v2);
				do
				{
					
					v1 = *((vu16 *)(FlashBase+Address+loop+0x4000)) ;
					v2 = *((vu16 *)(FlashBase+Address+loop+0x4000)) ;
				}while(v1!=v2);
				do
				{
					
					v1 = *((vu16 *)(FlashBase+Address+loop+0x6000)) ;
					v2 = *((vu16 *)(FlashBase+Address+loop+0x6000)) ;
				}while(v1!=v2);
			}	
		}
		else
		{
			
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55;
			*((vu16 *)(FlashBase+Address)) = 0x30 ;
			
			
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+Address+0x2000)) = 0x30 ;
			
			
			*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x2555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+Address+0x4000)) = 0x30 ;
			
			
			*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x3555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+Address+0x6000)) = 0x30 ;
			do
			{
				v1 = *((vu16 *)(FlashBase+Address)) ;
				v2 = *((vu16 *)(FlashBase+Address)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+Address+0x2000)) ;
				v2 = *((vu16 *)(FlashBase+Address+0x2000)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+Address+0x4000)) ;
				v2 = *((vu16 *)(FlashBase+Address+0x4000)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+Address+0x6000)) ;
				v2 = *((vu16 *)(FlashBase+Address+0x6000)) ;
			}while(v1!=v2);
		}
		break;
	case EZ4_2Chip:
		*((vu16 *)(FlashBase+0x555*2)) = 0xF0 ;
		*((vu16 *)(FlashBase+0x1555*2)) = 0xF0 ;
		if(blockAdd==0||blockAdd==0x2000000)
		{
			for(loop=0;loop<0x20000;loop+=0x4000)
			{
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+loop)) = 0x30 ;
				
				*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+loop+0x2000)) = 0x30;
				do
				{  
					
					v1 = *((vu16 *)(FlashBase+loop)) ;
					v2 = *((vu16 *)(FlashBase+loop)) ;
				}while(v1!=v2);
				do
				{
					
					v1 = *((vu16 *)(FlashBase+loop+0x2000)) ;
					v2 = *((vu16 *)(FlashBase+loop+0x2000)) ;
				}while(v1!=v2);
			}	
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x20000)) = 0x30 ;

			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x20000+0x2000)) = 0x30 ;
			do
			{  
				
				v1 = *((vu16 *)(FlashBase+0x20000)) ;
				v2 = *((vu16 *)(FlashBase+0x20000)) ;
			}while(v1!=v2);
			do
			{
				
				v1 = *((vu16 *)(FlashBase+loop+0x20000+0x2000)) ;
				v2 = *((vu16 *)(FlashBase+loop+0x20000+0x2000)) ;
			}while(v1!=v2);			
		}
		else if( (blockAdd==0x1FC0000)|| (blockAdd==0x2FC0000))
		{
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x7C0000)) = 0x30 ;
			
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x7C0000+0x2000)) = 0x30;
			do
			{  
				
				v1 = *((vu16 *)(FlashBase+0x7C0000)) ;
				v2 = *((vu16 *)(FlashBase+0x7C0000)) ;
			}while(v1!=v2);
			do
			{
				
				v1 = *((vu16 *)(FlashBase+0x7C0000+0x2000)) ;
				v2 = *((vu16 *)(FlashBase+0x7C0000+0x2000)) ;
			}while(v1!=v2);
			
			for(loop=0;loop<0x20000;loop+=0x4000)
			{
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x7C0000+0x20000+loop)) = 0x30 ;

				*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
				*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x7C0000+0x20000+0x2000+loop)) = 0x30 ;
				do
				{  
					
					v1 = *((vu16 *)(FlashBase+0x7C0000+0x20000+loop)) ;
					v2 = *((vu16 *)(FlashBase+0x7C0000+0x20000+loop)) ;
				}while(v1!=v2);
				do
				{
					
					v1 = *((vu16 *)(FlashBase+0x7C0000+0x20000+0x2000+loop)) ;
					v2 = *((vu16 *)(FlashBase+0x7C0000+0x20000+0x2000+loop)) ;
				}while(v1!=v2);			
			}
		}
		else
		{
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55;
			*((vu16 *)(FlashBase+Address)) = 0x30 ;
			
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+Address+0x2000)) = 0x30 ;
			
			do
			{
				v1 = *((vu16 *)(FlashBase+Address)) ;
				v2 = *((vu16 *)(FlashBase+Address)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+Address+0x2000)) ;
				v2 = *((vu16 *)(FlashBase+Address+0x2000)) ;
			}while(v1!=v2);
			
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55;
			*((vu16 *)(FlashBase+Address+0x20000)) = 0x30 ;
			
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0x80 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+Address+0x2000+0x20000)) = 0x30 ;
		
			do
			{
				v1 = *((vu16 *)(FlashBase+Address+0x20000)) ;
				v2 = *((vu16 *)(FlashBase+Address+0x20000)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+Address+0x2000+0x20000)) ;
				v2 = *((vu16 *)(FlashBase+Address+0x2000+0x20000)) ;
			}while(v1!=v2);	
		}
		break;
	default:
		break;
	}
	SetRompage(gl_currentpage);
}

void WriteEZ4Flash(u32 address,u8 *buffer,u32 size)
{
	vu16 page,v1,v2;
	register u32 loopwrite ;
	vu16* buf = (vu16*)buffer ;
	u32 size2,lop;
	u32 mapaddress;
	u32 j;
	page=gl_currentpage;
	while(address>=0x800000)
	{
		address-=0x800000;
		page+=0x1000;
	}
	SetRompage(page);
	v1=0;v2=1;
	switch(g_EZ4CardType)
	{	
	case EZ4_1Chip:
		for(loopwrite=0;loopwrite<(size/2);loopwrite++)
		{
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0xA0 ;
			*((vu16 *)(FlashBase+address+loopwrite*2)) = buf[loopwrite];
			do
			{
				v1 = *((vu16 *)(FlashBase+address+loopwrite*2)) ;
				v2 = *((vu16 *)(FlashBase+address+loopwrite*2)) ;
			}while(v1!=v2);
		}
		break;
	case EZ4_4Chip:
	case EZ4_4ChipPsram:
		for(loopwrite=0;loopwrite<(size/8);loopwrite++)
		{
			*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x555*2)) = 0xA0 ;
			*((vu16 *)(FlashBase+address+loopwrite*2)) = buf[loopwrite];
			
			*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x1555*2)) = 0xA0 ;			
			*((vu16 *)(FlashBase+address+0x2000+loopwrite*2)) = buf[0x1000+loopwrite];

			*((vu16 *)(FlashBase+0x2555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x22AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x2555*2)) = 0xA0 ;			
			*((vu16 *)(FlashBase+address+0x4000+loopwrite*2)) = buf[0x2000+loopwrite];

			*((vu16 *)(FlashBase+0x3555*2)) = 0xAA ;
			*((vu16 *)(FlashBase+0x32AA*2)) = 0x55 ;
			*((vu16 *)(FlashBase+0x3555*2)) = 0xA0 ;			
			*((vu16 *)(FlashBase+address+0x6000+loopwrite*2)) = buf[0x3000+loopwrite];
			do
			{
				v1 = *((vu16 *)(FlashBase+address+loopwrite*2)) ;
				v2 = *((vu16 *)(FlashBase+address+loopwrite*2)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+address+0x2000+loopwrite*2)) ;
				v2 = *((vu16 *)(FlashBase+address+0x2000+loopwrite*2)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+address+0x4000+loopwrite*2)) ;
				v2 = *((vu16 *)(FlashBase+address+0x4000+loopwrite*2)) ;
			}while(v1!=v2);
			do
			{
				v1 = *((vu16 *)(FlashBase+address+0x6000+loopwrite*2)) ;
				v2 = *((vu16 *)(FlashBase+address+0x6000+loopwrite*2)) ;
			}while(v1!=v2);
			
		}
		break;
	case EZ4_2Chip:
//		*((vu16 *)(FlashBase+0x555*2)) = 0xF0 ;
//		*((vu16 *)(FlashBase+0x1555*2)) = 0xF0 ; 
		if(size>0x4000)
		{
			size2 = size >>1 ;
			lop = 2; 
		}
		else 
		{
			size2 = size  ;
			lop = 1;
		}
		mapaddress = address;
		for(j=0;j<lop;j++)
		{
			if(j!=0)
			{
				mapaddress += 0x4000;
				buf = (vu16*)(buffer+0x4000);
			}
			for(loopwrite=0;loopwrite<(size2>>2);loopwrite++)
			{
				*((vu16 *)(FlashBase+0x555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x2AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x555*2)) = 0xA0 ;
				*((vu16 *)(FlashBase+mapaddress+loopwrite*2)) = buf[loopwrite];
				
				*((vu16 *)(FlashBase+0x1555*2)) = 0xAA ;
				*((vu16 *)(FlashBase+0x12AA*2)) = 0x55 ;
				*((vu16 *)(FlashBase+0x1555*2)) = 0xA0 ;			
				*((vu16 *)(FlashBase+mapaddress+0x2000+loopwrite*2)) = buf[0x1000+loopwrite];
				do
				{
					v1 = *((vu16 *)(FlashBase+mapaddress+loopwrite*2)) ;
					v2 = *((vu16 *)(FlashBase+mapaddress+loopwrite*2)) ;
				}while(v1!=v2);
				do
				{
					v1 = *((vu16 *)(FlashBase+mapaddress+0x2000+loopwrite*2)) ;
					v2 = *((vu16 *)(FlashBase+mapaddress+0x2000+loopwrite*2)) ;
				}while(v1!=v2);
			}
		}
		break;
	default:
		break;
	}
	SetRompage(gl_currentpage);
}

void SetSerialMode()
{
	
	*(vu16 *)0x9fe0000 = 0xd200;
	*(vu16 *)0x8000000 = 0x1500;
	*(vu16 *)0x8020000 = 0xd200;
	*(vu16 *)0x8040000 = 0x1500;
	*(vu16 *)0x9A40000 = 0xe200;
	*(vu16 *)0x9fc0000 = 0x1500;
	
}