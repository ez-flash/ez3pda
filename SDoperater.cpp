#include "SDOpera.h"

#include <AGB.h>
#include "BGFunc.h"
#include "stdio.h"
#include "string.h"
#include "bgfunc.h"

u32 SDadd ; //此全局变量为沟通的SD地址
SD_CSD csd ;

u8 Get_TAAC()
{
	//return csd.Taac ;
}

u8 Get_NSAC()
{
	return csd.byNsac ;
}
/*
void Outputline(char *p)
{
	DmaClear(3, 0,VRAM,240*2*12,32);
	DrawHZText12(p,48,0,0,0x7FFF,1 );
}
*/
//**功能描述:	用于产生多项式为X^16+X^12+X^5+1的CRC码
//**用于快速产生CRC16的结果
const unsigned short wCRCTalbeAbs[] =
{
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
};

unsigned short CRC16_2( unsigned char* pchMsg, unsigned short wDataLen)
{
        unsigned short wCRC = 0xFFFF;
        unsigned short i;
        unsigned char chChar;

        for (i = 0; i < wDataLen; i++)
        {
                chChar = *pchMsg++;
                wCRC = wCRCTalbeAbs[(chChar ^ wCRC) & 15] ^ (wCRC >> 4);
                wCRC = wCRCTalbeAbs[((chChar >> 4) ^ wCRC) & 15] ^ (wCRC >> 4);
        }

        return wCRC;
}

//**功能描述:	用于产生多项式为X^16+X^12+X^5+1的CRC码
unsigned int cal_crc_CCITT(unsigned char *ptr,unsigned char len)
{
	unsigned char i;
	unsigned int  crc=0;
	while(len--!=0)
	{
		for(i=0x80;i!=0;i/=2)
		{
			if((crc & 0x8000)!=0)
			{
				crc*=2;
				crc^=0x1021;
			}		
			else
			{
				crc*=2;
			}
			if((*ptr & i)!=0)
				crc ^= 0x1021;
		}
		ptr++;
	}
	return crc;
}

//**功能描述:	用于产生多项式为X^7+X^3+1的CRC码
unsigned char cal_crc_730(unsigned char *ptr,unsigned char len)
{
	return  crc_730(ptr,len);
/*	unsigned char i;
	unsigned char  crc=0;
	while(len--!=0)
	{
		for(i=0x80;i!=0;i/=2)
		{
			if((crc & 0x40)!=0)
			{
				crc*=2;
				crc^=0x09;
			}		
			else
			{
				crc*=2;
			}
			if((*ptr & i)!=0)
				crc ^= 0x09;
		}
		ptr++;
	}*/
/*	
	unsigned char data1 ;
	while(len--!=0)
	{
		data1 = *ptr ;
		for (i=0;i<8;i++)
		{
			if ((data1&1)^(crc&0x40)==0) 
			{
				crc<<=1;
			} 
			else
			{
				crc=crc^0x4; 
				crc<<=1;
				crc|=1;
			}
			data1>>=1;
		}
		ptr++;
	}
	*/
	//return crc;
}

void  SD_SendCommand (int type , unsigned int param )
{
	unsigned char ppbuf[8];
	unsigned char crc = 0 ;
	ppbuf[0] = 0x40 | (type&0xFF) ;
	ppbuf[1] = (param>>24)&0xFF ;
	ppbuf[2] = (param>>16)&0xFF ;
	ppbuf[3] = (param>>8)&0xFF ;
	ppbuf[4] = (param)&0xFF ;
	crc = cal_crc_730(ppbuf,5);
	ppbuf[5] = (crc<<1)|0x1 ;
	
/*	if(type==17)
	{
		char pmsg[32] ;
		sprintf(pmsg,"[r]%x,%x",crc,ppbuf[5]);
		DrawHZText12(pmsg,0, 2, 144 ,0xFFFF, 1 );		
	}*/	
	SD_WriteBufferToLine(ppbuf,6);
}

bool 	SD_R16Response(unsigned char *ppbuf, int wait)
{
	return SD_ReadResponse(ppbuf,6,0x10000);
}

bool 	SD_R2Response(unsigned char *ppbuf, int wait)
{
	return SD_ReadResponse(ppbuf,17,0x10000);
}

bool		SD_R3Response(unsigned char *ppbuf, int wait)
{
	bool ret ;
	ret = SD_ReadResponse(ppbuf,6,0x10000);
	if(ret)
	{
		if((ppbuf[0]!=0x3F)&&(ppbuf[5]!=0xFF))
			return false ;
		return true ;
	}
	return false ;
}

bool 	SD_ReadSingleBlock(unsigned int address , unsigned char *ppbuf, int len)
{
	bool ret ;
	SD_SendCommand(17,address); // single block  read , parm = address
	ret = SD_ReadData(ppbuf,len,0x100000);
	SD_ReadLoop(8);

	return ret ;
	

}

bool 	SD_ReadMultiBlock(unsigned int address , unsigned char *ppbuf, int len)
{
	bool ret ;
//char pmsg[32] ;
	int off = 0 ;
	unsigned char p[8];
	//SD_ReadLoop(8);
	SD_SendCommand(18,address); // single block  read , parm = address
	do
	{
		ret = SD_ReadData(ppbuf+off,520,0x100000);
		off += 512 ;
	}
	while((off<len)&&ret);
	
	SD_ReadLoop(8);
	SD_SendCommand(12,0);	//写结束，发送Stop命令
	SD_R16Response(p);	
	SD_ReadLoop(8);

//sprintf(pmsg,"[ret:%x]%x|%x,%x|%x,%x|%x,%x|%x,%x|",p[0],address,ppbuf[0],ppbuf[1],ppbuf[512+0],ppbuf[512+1],ppbuf[1024+0],ppbuf[1024+1],ppbuf[1536+0],ppbuf[1536+1]);
//DmaClear(2, 0,VRAM,240*2*12,32);
//DrawHZText12(pmsg,0, 2, 0 ,0xFFFF, 1 );		
//while(*(vu16*)0x04000130 == 0x3FF );
//while(*(vu16*)0x04000130 != 0x3FF );

	return ret ;

}


bool		SD_WaitDataline(int loop)
{
	unsigned short sk=0xf ;
	if(loop==0)
	{
		do
		{
			sk = 0xF00&(*(vu16*)0x9FEA000) ;
		}
		while(!sk);
		if(sk) return true ;
		else   return false ;
	}
	else
	{
		do
		{
			sk = 0xF00&(*(vu16*)0x9FEA000) ;
			loop -- ;
		}while(loop & (!sk)) ;
		if(!loop)	return false ;
		if(sk) return true ;
	}
	return true;
}

bool		SD_EraseBlock(unsigned int stblock,unsigned int edblock,unsigned char * ppbuf, int len)
{
	SD_SendCommand(32,stblock);	//start block
	SD_R16Response(ppbuf);
	SD_SendCommand(33,edblock);   // end block address
	SD_R16Response(ppbuf);
	SD_SendCommand(38,0);   // end block address
	SD_R16Response(ppbuf);
	return true ;	
}

bool		SD_WriteSingleBlock(unsigned int address , unsigned char *ppbuf, int len)
{
	unsigned short w1,w2,w3,w4 ;
	unsigned short b1,b2,b3,b4 ;
	unsigned short rw1,rw2,rw3,rw4 ;
	unsigned char ptmp[144];
	unsigned char pbuf[1024+64] ;
	register unsigned int i ;
	
	memcpy(pbuf,ppbuf,512);
	for(i=0;i<512;i+=4)
		ptmp[i>>2] = (pbuf[i]&0x80) + ((pbuf[i+1]&0x80)>>2)+ ((pbuf[i+2]&0x80)>>4) + ((pbuf[i+3]&0x80)>>6) 
				+ ((pbuf[i]&0x8)<<3) + ((pbuf[i+1]&0x8)<<1)+ ((pbuf[i+2]&0x8)>>1) + ((pbuf[i+3]&0x8)>>3) ;
	b1 = cal_crc_CCITT(ptmp,128);
	for(i=0;i<512;i+=4)
		ptmp[i>>2] = ((pbuf[i]&0x40)<<1) + ((pbuf[i+1]&0x40)>>1)+ ((pbuf[i+2]&0x40)>>3) + ((pbuf[i+3]&0x40)>>5) 
				+ ((pbuf[i]&0x4)<<4) + ((pbuf[i+1]&0x4)<<2)+ ((pbuf[i+2]&0x4)) + ((pbuf[i+3]&0x4)>>2) ;
	b2 = cal_crc_CCITT(ptmp,128);
	for(i=0;i<512;i+=4)
		ptmp[i>>2] =  ((pbuf[i]&0x20)<<2) + ((pbuf[i+1]&0x20))+ ((pbuf[i+2]&0x20)>>2) + ((pbuf[i+3]&0x20)>>4) 
				+ ((pbuf[i]&0x2)<<5) + ((pbuf[i+1]&0x2)<<3)+ ((pbuf[i+2]&0x2)<<1) + ((pbuf[i+3]&0x2)>>1) ;
	b3 = cal_crc_CCITT(ptmp,128);
	for(i=0;i<512;i+=4)
		ptmp[i>>2] = ((pbuf[i]&0x10)<<3) + ((pbuf[i+1]&0x10)<<1)+ ((pbuf[i+2]&0x10)>>1) + ((pbuf[i+3]&0x10)>>3) 
				+ ((pbuf[i]&0x1)<<6) + ((pbuf[i+1]&0x1)<<4)+ ((pbuf[i+2]&0x1)<<2) + ((pbuf[i+3]&0x1)) ;
	b4 = cal_crc_CCITT(ptmp,128);
	w1=b1>>8 ;
	w2=b2>>8 ;
	w3=b3>>8 ;
	w4=b4>>8 ;	
	rw1 = ((w1&0x80))+((w2&0x80)>>1)+((w3&0x80)>>2)+((w4&0x80)>>3)
		+((w1&0x40)>>3)+((w2&0x40)>>4)+((w3&0x40)>>5)+((w4&0x40)>>6) ;
	rw2 = ((w1&0x20)<<2)+((w2&0x20)<<1)+((w3&0x20))+((w4&0x20)>>1)
		+((w1&0x10)>>1)+((w2&0x10)>>2)+((w3&0x10)>>3)+((w4&0x10)>>4) ;
	rw3 = ((w1&0x8)<<4)+((w2&0x8)<<3)+((w3&0x8)<<2)+((w4&0x8)<<1)
		+((w1&0x4)<<1)+((w2&0x4))+((w3&0x4)>>1)+((w4&0x4)>>2) ;
 	rw4 = ((w1&0x2)<<6)+((w2&0x2)<<5)+((w3&0x2)<<4)+((w4&0x2)<<3)
		+((w1&0x1)<<3)+((w2&0x1)<<2)+((w3&0x1)<<1)+((w4&0x1)) ;
	pbuf[512] = rw1 ;
	pbuf[513] = rw2 ;
	pbuf[514] = rw3 ;
	pbuf[515] = rw4 ;
	w1=b1 ;
	w2=b2 ;
	w3=b3 ;
	w4=b4 ;	
	rw1 = ((w1&0x80))+((w2&0x80)>>1)+((w3&0x80)>>2)+((w4&0x80)>>3)
		+((w1&0x40)>>3)+((w2&0x40)>>4)+((w3&0x40)>>5)+((w4&0x40)>>6) ;
	rw2 = ((w1&0x20)<<2)+((w2&0x20)<<1)+((w3&0x20))+((w4&0x20)>>1)
		+((w1&0x10)>>1)+((w2&0x10)>>2)+((w3&0x10)>>3)+((w4&0x10)>>4) ;
	rw3 = ((w1&0x8)<<4)+((w2&0x8)<<3)+((w3&0x8)<<2)+((w4&0x8)<<1)
		+((w1&0x4)<<1)+((w2&0x4))+((w3&0x4)>>1)+((w4&0x4)>>2) ;
 	rw4 = ((w1&0x2)<<6)+((w2&0x2)<<5)+((w3&0x2)<<4)+((w4&0x2)<<3)
		+((w1&0x1)<<3)+((w2&0x1)<<2)+((w3&0x1)<<1)+((w4&0x1)) ;
	//应该做四次循环
	pbuf[516] = rw1 ;
	pbuf[517] = rw2 ;
	pbuf[518] = rw3 ;
	pbuf[519] = rw4 ;

	unsigned char p[20];
	SD_ReadLoop(8);
	SD_SendCommand(24,address);
	SD_R16Response(p);
	for(i=0;i<8;i++)
		rw1 =  *(u16*)0x9fea000 ;
	SD_WriteData(pbuf,520,0x10000);


	u16 k[16];
	for(i=0;i<16;i++)
		k[i]= *(u16*)0x9fea000 ;
		
	//sprintf((char*)ptmp,"[%x]%x,%x,%x,%x,%x,%x",address,k[0],k[1],k[2],k[3],k[4],k[5]);
	//DrawHZText12((char*)ptmp,0, 2, 132 ,0xFFFF, 1 );
	//while(1);
	
	if(((k[3]&0x100)!=0x0)||((k[4]&0x100)!=0x100)||((k[5]&0x100)!=0x0))
		return false ;
	//后面必须读出来，使写入进行下去
	for(i=0;i<8;i++)	
		k[i]= *(u16*)0x9fea000 ;
	return true ;
}

void		SD_ReadStatus()
{
	unsigned char p[32];
	SD_SendCommand(13,(SDadd<<16)) ;	//读状态
	SD_R16Response(p);
}

bool		SD_WriteMultiBlock(unsigned int address , unsigned char *ppbuf, int len)
{
	unsigned short w1,w2,w3,w4 ;
	unsigned short b1,b2,b3,b4 ;
	unsigned short rw1,rw2,rw3,rw4 ;
	unsigned char ptmp[144];
	unsigned char pbuf[536] ;
	unsigned char p[20];
	unsigned int  off = 0 ;
	register unsigned int i ;
	do
	{
		memcpy(pbuf,ppbuf+off,512);
		for(i=0;i<512;i+=4)
			ptmp[i>>2] = (pbuf[i]&0x80) + ((pbuf[i+1]&0x80)>>2)+ ((pbuf[i+2]&0x80)>>4) + ((pbuf[i+3]&0x80)>>6) 
					+ ((pbuf[i]&0x8)<<3) + ((pbuf[i+1]&0x8)<<1)+ ((pbuf[i+2]&0x8)>>1) + ((pbuf[i+3]&0x8)>>3) ;
		b1 = cal_crc_CCITT(ptmp,128);
		for(i=0;i<512;i+=4)
			ptmp[i>>2] = ((pbuf[i]&0x40)<<1) + ((pbuf[i+1]&0x40)>>1)+ ((pbuf[i+2]&0x40)>>3) + ((pbuf[i+3]&0x40)>>5) 
					+ ((pbuf[i]&0x4)<<4) + ((pbuf[i+1]&0x4)<<2)+ ((pbuf[i+2]&0x4)) + ((pbuf[i+3]&0x4)>>2) ;
		b2 = cal_crc_CCITT(ptmp,128);
		for(i=0;i<512;i+=4)
			ptmp[i>>2] =  ((pbuf[i]&0x20)<<2) + ((pbuf[i+1]&0x20))+ ((pbuf[i+2]&0x20)>>2) + ((pbuf[i+3]&0x20)>>4) 
					+ ((pbuf[i]&0x2)<<5) + ((pbuf[i+1]&0x2)<<3)+ ((pbuf[i+2]&0x2)<<1) + ((pbuf[i+3]&0x2)>>1) ;
		b3 = cal_crc_CCITT(ptmp,128);
		for(i=0;i<512;i+=4)
			ptmp[i>>2] = ((pbuf[i]&0x10)<<3) + ((pbuf[i+1]&0x10)<<1)+ ((pbuf[i+2]&0x10)>>1) + ((pbuf[i+3]&0x10)>>3) 
					+ ((pbuf[i]&0x1)<<6) + ((pbuf[i+1]&0x1)<<4)+ ((pbuf[i+2]&0x1)<<2) + ((pbuf[i+3]&0x1)) ;
		b4 = cal_crc_CCITT(ptmp,128);
		w1=b1>>8 ;
		w2=b2>>8 ;
		w3=b3>>8 ;
		w4=b4>>8 ;	
		rw1 = ((w1&0x80))+((w2&0x80)>>1)+((w3&0x80)>>2)+((w4&0x80)>>3)
			+((w1&0x40)>>3)+((w2&0x40)>>4)+((w3&0x40)>>5)+((w4&0x40)>>6) ;
		rw2 = ((w1&0x20)<<2)+((w2&0x20)<<1)+((w3&0x20))+((w4&0x20)>>1)
			+((w1&0x10)>>1)+((w2&0x10)>>2)+((w3&0x10)>>3)+((w4&0x10)>>4) ;
		rw3 = ((w1&0x8)<<4)+((w2&0x8)<<3)+((w3&0x8)<<2)+((w4&0x8)<<1)
			+((w1&0x4)<<1)+((w2&0x4))+((w3&0x4)>>1)+((w4&0x4)>>2) ;
	 	rw4 = ((w1&0x2)<<6)+((w2&0x2)<<5)+((w3&0x2)<<4)+((w4&0x2)<<3)
			+((w1&0x1)<<3)+((w2&0x1)<<2)+((w3&0x1)<<1)+((w4&0x1)) ;
		pbuf[512] = rw1 ;
		pbuf[513] = rw2 ;
		pbuf[514] = rw3 ;
		pbuf[515] = rw4 ;
		w1=b1 ;
		w2=b2 ;
		w3=b3 ;
		w4=b4 ;	
		rw1 = ((w1&0x80))+((w2&0x80)>>1)+((w3&0x80)>>2)+((w4&0x80)>>3)
			+((w1&0x40)>>3)+((w2&0x40)>>4)+((w3&0x40)>>5)+((w4&0x40)>>6) ;
		rw2 = ((w1&0x20)<<2)+((w2&0x20)<<1)+((w3&0x20))+((w4&0x20)>>1)
			+((w1&0x10)>>1)+((w2&0x10)>>2)+((w3&0x10)>>3)+((w4&0x10)>>4) ;
		rw3 = ((w1&0x8)<<4)+((w2&0x8)<<3)+((w3&0x8)<<2)+((w4&0x8)<<1)
			+((w1&0x4)<<1)+((w2&0x4))+((w3&0x4)>>1)+((w4&0x4)>>2) ;
	 	rw4 = ((w1&0x2)<<6)+((w2&0x2)<<5)+((w3&0x2)<<4)+((w4&0x2)<<3)
			+((w1&0x1)<<3)+((w2&0x1)<<2)+((w3&0x1)<<1)+((w4&0x1)) ;
		//应该做四次循环
		pbuf[516] = rw1 ;
		pbuf[517] = rw2 ;
		pbuf[518] = rw3 ;
		pbuf[519] = rw4 ;
	//这里等待有个好处，就是减少等待的时间		
		if(off==0)
		{
			SD_ReadLoop(8);
			SD_SendCommand(25,address);
			SD_R16Response(p);
			//sprintf((char*)ptmp,"[25]%x,%x,%x,%x,%x,%x",p[0],p[1],p[2],p[3],p[4],p[5]);
			//DrawHZText12((char*)ptmp,0, 2, 120 ,0xFFFF, 1 );
		for(i=0;i<4;i++)
			rw1 = *(u16*)0x9fea000 ;
		for(i=0;i<4;i++)
			*(u16*)0x9fea000  = 0xFFFF ;
		}
		else if(off > 512)
		{
			SD_WriteWaitDataline(0);	
		}
		SD_WriteData(pbuf,520,0x10000);

		u16 k[12];
		for(i=0;i<10;i++)
			k[i]= *(u16*)0x9fea000 ;
		if(((k[3]&0x100)!=0x0)||((k[4]&0x100)!=0x100)||((k[5]&0x100)!=0x0))
		{
			sprintf((char*)ptmp,"[e]%x,%x,%x,%x,%x,%x",k[0],k[1],k[2],k[3],k[4],k[5]);
			DrawHZText12((char*)ptmp,0, 2, 132 ,0xFFFF, 1 );
			return false ;
		}
		
		SD_SendCommand(13,address);
		SD_R16Response(p);
		//sprintf((char*)ptmp,"[25]%x,%x,%x,%x,%x,%x",p[0],p[1],p[2],p[3],p[4],p[5]);
		//DrawHZText12((char*)ptmp,0, 2, 144 ,0xFFFF, 1 );
		//后面必须读出来，使写入进行下去
		//for(i=0;i<8;i++)	
		//	k[i]= *(u16*)0x9fea000 ;
			
		off += 512 ;
	}while(off<len);
	SD_WriteWaitDataline(0x10000);
	SD_SendCommand(12,0);	//写结束，发送Stop命令
	SD_R16Response(p);
	return true ;
	
}

bool SD_WriteWaitDataline(int loop)
{
	unsigned short sk=0xf ;
	
	//SD_SendCommand(13,SDadd<<16);	//写结束，发送Stop命令
	//SD_R16Response(p);	
	//sprintf(pmsg,"[13]%x,%x,%x,%x,%x,%x",p[0],p[1],p[2],p[3],p[4],p[5]);
	//DrawHZText12(pmsg,0, 2, 132 ,0xFFFF, 1 );
	if(loop==0)
	{
		do
		{
			sk = 0x100&(*(vu16*)0x9FEA000) ;
		}
		while(!sk);

		if(sk) return true ;
		else   return false ;
	}
	else
	{
		do
		{
			sk = 0x100&(*(vu16*)0x9FEA000) ;
			loop -- ;
		}while(loop && (!sk)) ;
		if(!loop)	return false ;
		if(sk) return true ;
	}
	return true;
}

void		SD_ReadLoop(int lp)
{
	int i ,k;
	for(i=0;i<lp;i++)
		k = *(u16*)0x9FFFF40 ;
}

bool		SD_GetCSDStruct(unsigned char * ppbuf , int len , SD_CSD &csd)
{
	//const float fay[]={0 , 1.0 , 1.2 , 1.3 , 1.5 , 2.0 , 2.5 , 3.0 , 3.5 , 4.0 , 4.5 , 5.0 , 5.5 , 6.0 , 7.0 , 8.0 };
	if(!ppbuf) return false;
	if(len!=17) return false ;
	SD_CSDREAL *preal ;
	preal = (SD_CSDREAL *)&ppbuf[1] ;
	csd.CSDStruct = preal->CSD_STRUCTURE ;
	csd.Taac.TimeUnit = (CSD_TAAC_TIMEUNIT)((preal->TAAC)&0x7);
	csd.Taac.fTimeValue = (preal->TAAC&0x7F)>>3 ; 
	csd.byNsac = preal->NSAC ;
	csd.TranSpeed.transfer_rate = (CSD_TRAN_SPEEDUNIT)((preal->TRAN_SPEED)&0x7) ; 
	csd.TranSpeed.fTimeValue = (preal->TRAN_SPEED&0x7F)>>3 ;
	csd.wCCC = (preal->CCC_H)*0x10 +  (preal->CCC_L) ;
	csd.bRead_Bl_Len = preal->READ_BL_LEN ;
	csd.bRead_Bl_Partial = preal->READ_BL_PARTIAL ;
	csd.bWrite_Blk_Misalign = preal->WRITE_BLK_MISALIGN ;
	csd.bRead_Blk_Misalign  = preal->READ_BLK_MISALIGN ;
	csd.bDsr_Imp = preal->DSR_IMP ;
	csd.wC_Size = ((preal->C_SIZE_H2)<<10)+((preal->C_SIZE_H1)<<2)+preal->C_SIZE_L ;
	csd.byVdd_R_Curr_Min = preal->VDD_R_CURR_MIN ;
	csd.byVdd_R_Curr_Max = preal->VDD_R_CURR_MAX ;
	csd.byVdd_W_Curr_Min = preal->VDD_W_CURR_MIN;
	csd.byVdd_W_Curr_Max = preal->VDD_W_CURR_MAX ;
	BYTE s = ((preal->C_SIZE_MULT_H)<<2) + preal->C_SIZE_MULT_L ;
	csd.wC_Size_Mult = 1<<(s+2) ;
	csd.bErase_Blk_En = preal->ERASE_BLK_EN ;
	csd.bySector_Size = preal->SECTOR_SIZE_H*2 + preal->SECTOR_SIZE_L;
	csd.byWp_Grp_Size = preal->WP_GRP_SIZE ;
	csd.bWp_Grp_Enable = preal->WP_GRP_ENABLE ;
	csd.byR2w_Factor = preal->R2W_FACTOR ;
	csd.byWrite_Bl_Len = preal->WRITE_BL_LEN_H * 4 + preal->WRITE_BL_LEN_L ;
	csd.bWrite_Bl_Partial = preal->WRITE_BL_PARTIA ;
	csd.byFile_Format_Grp = preal->FILE_FORMAT_GRP ;
	csd.bCopy = preal->COPY ;
	csd.byPerm_Write_Protect = preal->PERM_WRITE_PROTECT ;
	csd.byTmp_Write_Protect = preal->TMP_WRITE_PROTECT ;
	csd.File_Format = (CSD_FILE_FORMAT)preal->FILE_FORMAT ;
	return true ;
}

bool		SD_GetCIDStruct(unsigned char * ppbuf , int len , SD_CID &cid)
{
	BYTE *p;
	if(!ppbuf)	return false ;
	if(len != 17) return false ;
	memset(&cid,0,sizeof(SD_CID));
	p = &ppbuf[0];
	cid.byManufacturerID = p[0];
	cid.wOemID = *((WORD *) &p[1]);
	memcpy(cid.szProductName,&p[3],5);
	cid.byProductReision = p[8];
	cid.dwProductSn = *((DWORD *) &p[9]);
	cid.wData = *((WORD *) &p[13]);
	cid.byCRC = (p[15] & 0xFE);
	//this is just a test 
	return true;
	
}

#include "hard.h"
void	 SD_Enable()
{
	OpenWrite();
	EnableNand8bit();
}

void	SD_Disable()
{
	DisableNandbit();
	CloseWrite();
}

bool SD_initial()
{//如果返回真表示存在SD卡，假表示不存在SD卡
	bool  ret ;
	unsigned char pres[40] ;
	SD_CID	cid ;	
	
	SD_ReadLoop(147);
	SD_SendCommand(0,0);
	SD_ReadLoop(8);
	
	SD_SendCommand(55,0);   // 指示下一个命令是app命令
	ret = SD_R16Response(pres);	
	if(pres[0]!=55)
		return false; 
	if(ret)
	{
		SD_SendCommand(41,0x0); //检测电压范围
		ret = SD_R16Response(pres);
		do
		{
			SD_SendCommand(55,0);
			ret = SD_R16Response(pres);
			SD_SendCommand(41,0xFC0000);
			ret = SD_R3Response(pres);
		}while(pres[1]!=0x80) ;
	}
	
	if(ret)
	{
		SD_SendCommand(2,0);	//Get CID 
		ret = SD_R2Response(pres);
		SD_GetCIDStruct(pres,17,cid);
	}
	if(ret)
	{
		do
		{
			SD_SendCommand(3,0);		//进入stand by 状态，并得到sd卡状态
			ret = SD_R16Response(pres);
		}while((pres[3]&0x1E) != 0x6); //stand by state
	}
	
	if(ret)
	{
		SDadd = pres[1]*0x100 + pres[2] ;
		SD_SendCommand(9,(SDadd<<16));//send_csd
		ret = SD_R2Response(pres);
		SD_GetCSDStruct(pres,17,csd);
	}
	if(ret)
	{
		SD_SendCommand(7,SDadd<<16);	//select card
		ret = SD_R16Response(pres);	
	}
	if(ret)
	{
		SD_SendCommand(55,SDadd<<16); //app command
		ret = SD_R16Response(pres);
		SD_SendCommand(6,2);		//00, 1 bit , 10  4 bit 
		ret = SD_R16Response(pres);
	}
	if(ret)
	{
		SD_SendCommand(16,0x200) ;	//设定一个block为512大小
		ret = SD_R16Response(pres);
	}
	return ret ;
}

#include "AGB.h"
void SD_Test()
{
	unsigned char pmem[1024+64] ;
	unsigned char pres[40] ;
	char pmsg[64] ;
	u32 kk ;
	
	//first must initial 
	OpenWrite();
	EnableNand8bit();
	SD_ReadLoop(147);
	SD_SendCommand(0,0);
	SD_ReadLoop(8);

	SD_SendCommand(55,0);   // 指示下一个命令是app命令
	SD_R16Response(pres);	
	sprintf(pmsg,"[55]%x,%x,%x,%x,%x,%x \0",pres[0],pres[1],pres[2],pres[3],pres[4],pres[5]);
	DrawHZText12(pmsg,0, 2, 0 ,0xFFFF, 1 );

	SD_SendCommand(41,0x0); //检测电压范围
	SD_R16Response(pres);
	do
	{
		SD_SendCommand(55,0);
		SD_R16Response(pres);
		SD_SendCommand(41,0xFC0000);
		SD_R3Response(pres);
	}while(pres[1]!=0x80) ;
	sprintf(pmsg,"[41]%x,%x,%x,%x,%x,%x \0",pres[0],pres[1],pres[2],pres[3],pres[4],pres[5]);
	DrawHZText12(pmsg,0, 2, 12 ,0xFFFF, 1 );
	
	SD_CID	cid ;	
	SD_SendCommand(2,0);	//Get CID 
	SD_R2Response(pres);
	SD_GetCIDStruct(pres,17,cid);
	sprintf(pmsg,"[2]%x,%x,%s %x,%x,%x",pres[0],pres[1],cid.szProductName,cid.byManufacturerID,cid.wOemID,cid.dwProductSn);
	DrawHZText12(pmsg,0, 2, 24 ,0xFFFF, 1 );
	
	do
	{
		SD_SendCommand(3,0);		//进入stand by 状态，并得到sd卡状态
		SD_R16Response(pres);
	}while((pres[3]&0x1E) != 0x6); //stand by state
	sprintf(pmsg,"[3]%x,%x,%x,%x,%x,%x,%x,%x",pres[0],pres[1],pres[2],pres[3],pres[4],pres[5],pres[6],pres[7]);
	DrawHZText12(pmsg,0, 2, 36,0xFFFF, 1 );
	
	SDadd = pres[1]*0x100 + pres[2] ;
	SD_CSD csd ;
	SD_SendCommand(9,(SDadd<<16));//send_csd
	SD_R2Response(pres);
	SD_GetCSDStruct(pres,17,csd);
	sprintf(pmsg,"[9]%x,%x,%x,%x,%x,%x,%x,%x",pres[0],csd.bWrite_Bl_Partial,csd.byWrite_Bl_Len,pres[3],pres[4],pres[5],pres[6],pres[7]);
	DrawHZText12(pmsg,0, 2, 48,0xFFFF, 1 );
	
	
	SD_SendCommand(7,SDadd<<16);	//select card
	SD_R16Response(pres);	
	sprintf(pmsg,"[7]%x,%x,%x,%x,%x,%x,%x,%x",pres[0],pres[1],pres[2],pres[3],pres[4],pres[5],pres[6],pres[7]);
	DrawHZText12(pmsg,0, 2, 60,0xFFFF, 1 );

	
	SD_SendCommand(55,SDadd<<16); //app command
	SD_R16Response(pres);
	SD_SendCommand(6,2);		//00, 1 bit , 10  4 bit 
	SD_R16Response(pres);
	sprintf(pmsg,"[6]%x,%x,%x,%x,%x,%x,%x,%x",pres[0],pres[1],pres[2],pres[3],pres[4],pres[5],pres[6],pres[7]);
	DrawHZText12(pmsg,0, 2, 72 ,0xFFFF, 1 );
	u32 off =0 ;
	bool ret ;
//	do
	{
		DmaClear(3,0,0x6000000+240*2*84,240*12*2,32);
		SD_ReadSingleBlock(off+512,pmem,536);
		sprintf(pmsg,"[da]%x,%x,%x,%x,%x,%x,%x,%x",pmem[0],pmem[1],pmem[2],pmem[3],pmem[4],pmem[5],pmem[6],pmem[7]);
		DrawHZText12(pmsg,0, 2, 84 ,0xFFFF, 1 );

		*(vu16*)0x8000408 = 0x7766 ;

	//	SD_EraseBlock(0,0x20000,pres,20);
	//	u16  r = SD_WaitDataline(0) ;
	//	sprintf(pmsg,"[38]%x,%x,%x,%x,%x,%x,%x,%x",pres[0],pres[1],pres[2],pres[3],pres[4],pres[5],pres[6],r);
	//	DrawHZText12(pmsg,0, 2, 108 ,0xFFFF, 1 );

		while(*(vu16*)0x04000130==0x3FF)
			VBlankIntrWait();
		while(*(vu16*)0x04000130!=0x3FF)
			VBlankIntrWait();
		//off += 512 ;
	}//while(1);
	for(kk=0;kk<1024;kk++)
	{
		pmem[kk] = 1117- kk ;
	}
	SD_WriteSingleBlock(512,pmem,512);
	SD_WriteWaitDataline(0);
	//SD_WriteMultiBlock(0,pmem,1024) ;
	
	SD_SendCommand(13,SDadd<<16);	//select card
	SD_R16Response(pres);	
	sprintf(pmsg,"[13]%x,%x,%x,%x,%x,%x,%x,%x",pres[0],pres[1],pres[2],pres[3],pres[4],pres[5],pres[6],pres[7]);
	DrawHZText12(pmsg,0, 2, 108,0xFFFF, 1 );

		ret = SD_ReadSingleBlock(off+512,pmem,536);
		sprintf(pmsg,"[ret]%x,%x,%x,%x,%x,%x,%x,%x",ret,pmem[0],pmem[1],pmem[2],pmem[3],pmem[4],pmem[5],pmem[6],pmem[7]);
		DrawHZText12(pmsg,0, 2, 96 ,0xFFFF, 1 );

}