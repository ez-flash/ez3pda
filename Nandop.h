#ifndef _NANDOP_H
#define _NANDOP_H

#include "Agb.h"

void		EnableNand8bit();
void		EnableNand16bit();
void		DisableNandbit();
u32  	NAND_ReadID();
void 	NAND_Erase(u32 BlockAddress);
u8 		NAND_ReadStatus();
u8		NAND_ReadBusy(); //1 busy . 0 not busy 
void 	NAND_Reset();
void 	Nand_Read16(u32 address ,u8* pdata,u32 size);
void		Nand_Read8(u32 address ,u8* pdata,u32 size);
void 	Nand_Write(u32 address, u8* pdata,u32 size);
bool		IsValidID(u32 id);	
#endif
