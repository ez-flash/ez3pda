#include "inram.h"

#include "hard.h"
//#pragma arm section rodata="_inram"
extern u32 gl_currentpage ;
extern EZ4_CARTTYPE g_EZ4CardType;
 
void SetRampage(u16 page)
{
	*(vu16 *)0x9fe0000 = 0xd200;
	*(vu16 *)0x8000000 = 0x1500;
	*(vu16 *)0x8020000 = 0xd200;
	*(vu16 *)0x8040000 = 0x1500;
	*(vu16 *)0x9c00000 = page;
	*(vu16 *)0x9fc0000 = 0x1500;
}

void SetRompage(u16 page)
{
	*(vu16 *)0x9fe0000 = 0xd200;
	*(vu16 *)0x8000000 = 0x1500;
	*(vu16 *)0x8020000 = 0xd200;
	*(vu16 *)0x8040000 = 0x1500;
	*(vu16 *)0x9880000 = page;
	*(vu16 *)0x9fc0000 = 0x1500;
}

void HardRest()
{
    __asm
    {

		mov		r3, #0x04000208
		mov		r2, #0
		str		r2, [r3, #0]
		mov		r3, #0x03007FFA
		mov		r2, #0xFD
		str		r2, [r3, #0]
		swi		0x1
		mov		r6, #0x30007F00
		str		r5, [r6]
		mov		r6, #0x03007FFA
		mov		r7, #0x0
		str		r7, [r6, #0]
		swi		0
    }

}

void SoftRest()
{
    __asm
    {
    	swi 0x26
   	}
}

void SetRompageWithSoftReset(u16 page)
{
	SetRompage(page);
	SoftRest();
}

void SetRompageWithHardReset(u16 page)
{
	SetRompage(page);
	HardRest();
}

void inramresetexec()
{
	SetRompage(0x0);
	*(u16*)(0x8000000) = 0xFF ;
	*(u16*)(0x8000000+0x100) = 0xFF ;
	*(u16*)(0x8000000+0x200) = 0xFF ;
	*(u16*)(0x8000000+0x300) = 0xFF ;
	*(u16*)(0x8800000) = 0xFF ;
	*(u16*)(0x8800000+0x100) = 0xFF ;
	*(u16*)(0x8800000+0x200) = 0xFF ;
	*(u16*)(0x8800000+0x300) = 0xFF ;
	SetRompage(gl_currentpage)	;
}

void minireset(u16 page)
{
	SetRompage(page);
	__asm
	{
		mov		r3, #0x03007FFA
		mov		r2, #0x0
		strh	r2, [r3, #0]
		mov		r0, 0xFC
		swi		0x1
		swi 0
	}
}

void	ldErase()
{
	u16 i ;
	u32 eraseAdd = 0x83E0000;//0x08200000 ;
	 *(vu16 *)REG_IME   = 0 ;                           // Set IME
	*(vu16*)(eraseAdd) = 0x50 ;
	*(vu16*)(eraseAdd) = 0xFF ;
	*(vu16*)(eraseAdd) = 0x20 ;
	*(vu16*)(eraseAdd) = 0xD0 ;
	while( (*(vu16*)(eraseAdd)) != 0x80);	
	*(vu16*)(eraseAdd) = 0xFF ;
	 *(vu16 *)REG_IME   = 1 ;                           // Set IME

	/*
	for(i=0;i<32;i++)
	{
		*(u16*)(eraseAdd+i*0x8000) = 0x20 ;
		*(u16*)(eraseAdd+i*0x8000) = 0xd0 ;
		while( (*(u16*)(eraseAdd+i*0x8000)) != 0x80);
	}
	*/
//	*(u16*)(0x08200000) = 0xFF ;
}

void  ldRead(u8* buf,u32 size)
{
	register u32 ss;
	u8 *pSave=(u8*)0x0E000000;
	
	SetRampage(0xA0);
	for(ss=0;ss<size;ss++)
	{
		buf[ss] = pSave[ss]  ;
	}
	SetRampage(0x00);
	/*
	u32 eraseAdd = 0x83E0000;//0x08200000 ;
	u32 i;
	
	 *(vu16 *)REG_IME   = 0 ;                           // Set IME
	*(vu16*)(eraseAdd) = 0x50 ;
	*(vu16*)(eraseAdd) = 0xFF ;
	for(i=0;i<size;i+=2)
		*((u16 *)(buf+i)) = *(vu16*)(eraseAdd+i);
	 *(vu16 *)REG_IME   = 1 ;              
	 */
}
void  ldWrite(u32 address,u8*buf,u32 size)
{
	register  u32 ss;
	u8 *pSave=(u8*)0x0E000000;
	
	SetRampage(0xA0);
	for(ss=0;ss<size;ss++)
	{
		pSave[ss] = buf[ss];
	}
	SetRampage(0x00);
	
	/*
	vu16* pWord = (vu16 *)buf ;
	u32  wsize = size/2 ;
	u32 ss = 0,e=0;

	for(ss=0;ss<wsize;ss+=128)
	{
		*(vu16*)(address+(ss<<1)) = 0x41;
		for(e=0;e<128;e++)
		{
			*(vu16 *)(address+(ss<<1)+e*2) = pWord[ss+e];
		}
		*(vu16*)(address+(ss<<1)) = 0x0e;
		*(vu16*)(address+(ss<<1)) = 0xd0;
		while((*(vu16*)(address+(ss<<1)) != 0x80));

		*(vu16*)(address+(ss<<1)) = 0xFF;
	}
	*(vu16*)(0x83E0000) = 0xFF ;
	*/
}

void IsSingleCard()
{	
	u32 address = 0x8000000;
	vu16 id1,id2;
	*(u16*)(address+0xAAA) = 0xF0 ;
	*(u16*)(address+0xAAA) = 0xAA ;
	*(u16*)(address+0x554) = 0x55 ;
	*(u16*)(address+0xAAA) = 0x90 ;
	id1 = *(vu16*)(address+2);
	id2 = *(vu16*)(address+0xE*2);
	if(id1==0x227E && id2==0x2202)
	{
		*(u16*)(address+0xAAA) = 0xAA ;
		*(u16*)(address+0x554) = 0x55 ;
		*(u16*)(address+0xAAA) = 0xF0 ;
		g_EZ4CardType = EZ4_1Chip;
	}
}
