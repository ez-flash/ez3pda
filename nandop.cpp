#include "nandop.h"
#include "hard.h"
#include "inram.h"
#include "bgFunc.h"
#include "stdio.h"
/* leave here in case for use of later time.
//#pragma arm section rwdata = "foo", code ="foo"  
//name the rw section and code section
*/
#define	 pNandCommand (*((u8*)0x9FFFFE2))
#define	 pNandAddress (*((u8*)0x9FFFFE0))
#define	 p16NandData (*((u16*)0x9FFC000))
#define	 p8NandData (*((u8*)0x9FFC000))
#define  NADNPAGESIZE  2048 

extern  u16      ReadKey,Cont,Trg;           // Key input
u16		NANDSize ;
void  SetNandControl(u16  control)
{
	*(u16 *)0x9fe0000 = 0xd200;
	*(u16 *)0x8000000 = 0x1500;
	*(u16 *)0x8020000 = 0xd200;
	*(u16 *)0x8040000 = 0x1500;
	*(u16 *)0x9400000 = control;
	*(u16 *)0x9fc0000 = 0x1500;
}

void	EnableNand8bit()
{
	SetNandControl(1);
}

void	EnableNand16bit()
{
	SetNandControl(3);
}

void	DisableNandbit()
{
	SetNandControl(0);
}

u32  	NAND_ReadID()
{
	u8 id[4];
	pNandCommand = 0x90 ;
	pNandAddress = 0 ;
	id[0]=p8NandData;
	id[1]=p8NandData;
	id[2]=p8NandData;
	id[3]=p8NandData;
	NANDSize = 0 ;
	if((*(u32*)id == 0x1580daec)||(*(u32*)id == 0x1580daad))
		NANDSize = 2; 
	else if((*(u32*)id == 0x1580dcec)||(*(u32*)id == 0x1580dcad))
		NANDSize = 4; 
	else if((*(u32*)id==0x15C1dcec)||(*(u32*)id==0x15C1dcad)||(*(u32*)id==0x9551d3ec))
		NANDSize = 8 ;
	return *(u32*)id ;
}

bool IsValidID(u32 id)
{
	bool ret ;
	if((id == 0x1580f1ec)||(id == 0x1580f1ad))
		ret = true ;
	else if((id == 0x1580daec)||(id == 0x1580daad))
		ret = true ;
	else if((id == 0x1580dcec)||(id == 0x1580dcad))
		ret = true ;
	else if((id==0x15C1dcec)||(id==0x15C1dcad)||(*(u32*)id==0x9551d3ec))
		ret = true ;
	else return false ;
	return ret ;
}
void NAND_Erase(u32 BlockAdress)
{
	u32 a1,a2;
	u8 add1 = 0 , add2 = 0 , add3 = 0 , add4 = 0 , add5 = 0;
	a1 = (u16)(BlockAdress%NADNPAGESIZE)&0xFFFF; //低位
	a2 = (BlockAdress/NADNPAGESIZE); //高位
	add1 =a1&0xFF ;
	add2 =(a1>>8)&0xFF;
	add3 =a2&0xFF ;
	add4 =(a2>>8)&0xFF;
	add5 = (a2>>16)&0xFF ;
	pNandCommand = 0x60 ;
	pNandAddress = add3;
	pNandAddress = add4;
	if(NANDSize)
		pNandAddress = add5;
	pNandCommand = 0xd0 ;
	do
	{
		add1 =  NAND_ReadStatus();
	}
	while((add1&0xC1)!=0xc0);
}

u8		NAND_ReadBusy() //1 busy . 0 not busy 
{
	u8 st ;
	Clock_Enable();
	st = *(u16*)CLOCK_DATA;
	Clock_Disable();
	return !(st&0x8) ;
}

u8 NAND_ReadStatus()
{
	u8 dd ;
	pNandCommand = 0x70 ;
	dd=p8NandData;
	return dd ;
}

void NAND_Reset()
{
	pNandCommand = 0xFF ;
}

void Nand_Read16(u32 address ,u8* pdata,u32 size)
{
	u16 nop ;
	u32 a1,a2;
	u8 add1 = 0 , add2 = 0 , add3 = 0 , add4 = 0 , add5 =0 ;
	a1 = (u16)(address%NADNPAGESIZE)&0xFFFF; //低位
	a2 = (address/NADNPAGESIZE) ; //高位
	add1 =a1&0xFF ;
	add2 =(a1>>8)&0xFF;
	add3 =a2&0xFF ;
	add4 =(a2>>8)&0xFF;
	add5= (a2>>16)&0xFF ;
	Clock_Enable();
	pNandCommand = 0x00 ;
	pNandAddress = add1;
	pNandAddress = add2;
	pNandAddress = add3;
	pNandAddress = add4;
	if(NANDSize)
		pNandAddress = add5;
	pNandCommand = 0x30;
	
	do{
		a1 = *(u16*)CLOCK_DATA;
	}while(!(a1&0x8));
	/*
	*/
	DmaCopy(3,0x9FFC000,pdata,size,32);

	Clock_Disable();
}

void Nand_Read8(u32 address ,u8* pdata,u32 size)
{
	register u16 i;
	u16 nop ;
	u8 d1,d2;
	u32 a1,a2;
	u8 add1 = 0 , add2 = 0 , add3 = 0 , add4 = 0 , add5 = 0 ;
	a1 = (u16)(address%NADNPAGESIZE)&0xFFFF; //低位
	a2 = (address/NADNPAGESIZE); //高位
	add1 =a1&0xFF ;
	add2 =(a1>>8)&0xFF;
	add3 =a2&0xFF ;
	add4 =(a2>>8)&0xFF;
	add5= (a2>>16)&0xFF ;
	Clock_Enable();
	pNandCommand = 0x00 ;
	pNandAddress = add1;
	pNandAddress = add2;
	pNandAddress = add3;
	pNandAddress = add4;
	if(NANDSize)
		pNandAddress = add5;
	pNandCommand = 0x30;
	
	do{
		a1 = *(u16*)CLOCK_DATA;
	}while(!(a1&0x8));
	/*
	*/
	for(i=0;i<(size>>1);i++)
	{
		d1 = p8NandData ;
		d2 = p8NandData ;
		((u16*)pdata)[i] = (d2<<8)+d1 ;
	}
	Clock_Disable();
}

void Nand_Write(u32 address, u8* pdata,u32 size)
{
	register u16 i ;
	u16 nop ;
	u32 a1,a2;
	u8 add1 = 0 , add2 = 0 , add3 = 0 , add4 = 0 ,add5 = 0;
	a1 = (u16)(address%NADNPAGESIZE)&0xFFFF; //低位
	a2 = (address/NADNPAGESIZE); //高位
	add1 =a1&0xFF ;
	add2 =(a1>>8)&0xFF;
	add3 =a2&0xFF ;
	add4 =(a2>>8)&0xFF;
	add5= (a2>>16)&0xFF ;
	Clock_Enable();
	pNandCommand = 0x80 ;
	pNandAddress = add1;
	pNandAddress = add2;
	pNandAddress = add3;
	pNandAddress = add4;
	if(NANDSize)
		pNandAddress = add5;
	for(i=0;i<size;i++)
	{
		p8NandData = pdata[i] ;
	}
	pNandCommand = 0x10;
	NULL;
	NULL;
	do{
		a1 = *(u16*)CLOCK_DATA;
	}while(!(a1&0x8));
	
	do
	{
		add1 =  NAND_ReadStatus();
	}
	while((add1&0xC1)!=0xc0);
	Clock_Disable();

}
