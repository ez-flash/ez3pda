#include "agbmemorymap.h"
#include "agbdefine.h"

#define     vl                      volatile
typedef     vl unsigned short       vu16;
typedef     vl unsigned int       	vu32;

void IntrDummy(void);
void VBlankIntr(void);
typedef void (*IntrFuncp)(void);

IntrFuncp IntrTable[13] = {
    VBlankIntr,     // V Blank interrupt
    IntrDummy,      // H Blank interrupt
    IntrDummy,      // V Counter interrupt
    IntrDummy,      // Timer 0 interrupt
    IntrDummy,      // Timer 1 interrupt
    IntrDummy,      // Timer 2 interrupt
    IntrDummy,      // Timer 3 interrupt
    IntrDummy,      // Serial communication interrupt
    IntrDummy,      // DMA 0 interrupt
    IntrDummy,      // DMA 1 interrupt
    IntrDummy,      // DMA 2 interrupt
    IntrDummy,      // DMA 3 interrupt
    IntrDummy,      // Key interrupt
};

typedef struct tagOAMEntry
{
	unsigned int  attr0;
	unsigned int  attr1;
}OAMEntry,*pOAMEntry;


int VblankCount;
/*==================================================================*/
/*                      Interrupt Routine                           */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                      V Blank Process                             */
/*------------------------------------------------------------------*/

void VBlankIntr(void)
{
    *(vu16*)INTR_CHECK_BUF = V_BLANK_INTR_FLAG;     // Set V Blank interrupt check
   	VblankCount ++ ;
}

/*------------------------------------------------------------------*/
/*                      Interrupt Dummy Routine                     */
/*------------------------------------------------------------------*/

void IntrDummy(void)
{
}

/* 例子中断处理程序
void IRQHandler(void)
{
	u16 Int_Flag; 

	REG_IME = 0x00;			// Disable interrupts 
	Int_Flag = REG_IF;		// Read the interrupt flags

	//If it is the H-BLANK interrupt, draw a line
	if((REG_IF & BIT01) == BIT01)
	{
		//Make a line with Bresenham's algorithm
		LineRGB(0, REG_VCOUNT+1, 240, REG_VCOUNT+1, rand()%31, rand()%31, rand()%31);
	}

	REG_IF = Int_Flag;		// Write back the interrupt flags 
	REG_IME = BIT00;		// Re-Enable interrups 
}
*/
