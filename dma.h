#ifndef	DMA_HEADER
#define DMA_HEADER

#include "agb.h"


#define DMA_32NOW              32 
#define DMA_16NOW              16


//******************************************************************************
//**** DMA内存直接访问式复制数据
#define DMA_Copy(channel,dest,source,WordCount,mode) \
{\
		DmaCopy(channel,source,dest,(WordCount*(mode>>3)),mode) ; \
}
#endif

