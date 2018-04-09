#ifndef __INRAM_H
#define __INRAM_H

#include "agb.h"

void SetRampage(u16 page);
void SetRompage(u16 page);
void SetRompageWithSoftReset(u16 page);
void SetRompageWithHardReset(u16 page);
void SoftRest();
void HardRest();

void inramresetexec() ;
void	ldErase() ;
void ldWrite(u32 address,u8*buf,u32 size);
void ldRead(u8* buf,u32 size);
void minireset(u16 page);
void   IsSingleCard();
//≤‚ ‘
//void AATEST() ;

#endif
