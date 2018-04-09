#include "stdio.h"
#include "stdarg.h"
#include "string.h"
//#include "math.h"
#include "stdlib.h"

#include "agb.h"
#include "global.h"
#include "BGfunc.h"
#include "keypad.h"
#include "screenmode.h"
#include "jpeg.h"
#include "asc1616.h"
#include "yinbiao.h"
#include "shell.h"
#include "savermanage.h"

extern res_struct res;

//diffrent from agb official 
#define REG_TM3D       *(u16*)0x400010C
#define REG_BLDMOD     *(u16*)0x4000050
#define REG_COLEV      *(u16*)0x4000052
#define REG_COLEY      *(u16*)0x4000054
#define OAMmem         (u32*)0x7000000
#define VideoBuffer    (u16*)0x6000000
#define OAMdata		   (u16*)0x6100000
#define BGPaletteMem   (u16*)0x5000000
#define OBJPaletteMem  (u16*)0x5000200
#define GAMEPAKMem	   (u8*) 0x8000000
#define Vcache         (u16*)0x2001000
#define IWRAM		   (u16*)0x3000000

const unsigned short picExtnum=11;	//识别的图像文件数量
const char picExt[11][4]={			//识别的图像文件扩展名列表
	"BMP",
	"GIF",
	"JPG",
	"ICO",
	"JPE", 
	"LWF",
	"PIC", 
	"PCX",
	"PSD",
	"TIF",
	"PNG"
};



//******************************************************************************
//**** 延时函数(毫秒级) 
void sleep(u16 ms)
{
	//Start the timer
	*(vu16*)REG_TM3CNT_H  = 1|2|128|0x80;

    //zero the timer
	REG_TM3D = 0; 

	while(ms--)
	{
		while(REG_TM3D <= 16){}                //wait
		REG_TM3D = 0;          //reset the timmer
	} 
	//Stop the timer
	*(vu16*)REG_TM3CNT_H  = 0;
	
} 


//******************************************************************************
//**** 在调试显示框中，打印调试信息(要求MODE_3)
void dbgPrint(char *str,...)			  
{
	va_list ptr;char *strr;
	strr=(char*)malloc(strlen(str)+2);
	va_start(ptr,str);
	vsprintf(strr,str,ptr);
	va_end(ptr);
	Clear(0,130,240,30,RGB(30,30,2),1);
	DrawHZTextRect(strr,120,1,131,238,29,14,0,RGB(3,3,31),1);
	free(strr);
}


//******************************************************************************
//**** 初始化资源，将系统资源结构体进行赋值
void initResource()
{
	res.res_ZKDATA = GetRomFileByName("\\.shell\\","hzk.dat")+sizeof(RomFile);
	res.res_ASCDATA = GetRomFileByName("\\.shell\\","asc.dat")+sizeof(RomFile);

	res.res_DESKBG =	(u16*)(GetRomFileByName("\\.shell\\bmp\\","desktop.bmp")+sizeof(RomFile)+8);
	res.res_DESKICON =	(u16*)(GetRomFileByName("\\.shell\\bmp\\","deskicon.bmp")+sizeof(RomFile)+8);
	res.res_FILEICON =	(u16*)(GetRomFileByName("\\.shell\\bmp\\","icons.bmp")+sizeof(RomFile)+8);
	res.res_FILEBG =	(u16*)(GetRomFileByName("\\.shell\\bmp\\","filemng.bmp")+sizeof(RomFile)+8);
	res.res_FILEBGHEAD =(u16*)(GetRomFileByName("\\.shell\\bmp\\","filemnghead.bmp")+sizeof(RomFile)+8);

	res.res_TXTBG =		(u16*)(GetRomFileByName("\\.shell\\bmp\\","txtbg.bmp")+sizeof(RomFile)+8);
	res.res_TXTSCUP   = (u16*)(GetRomFileByName("\\.shell\\bmp\\","TextScrollUp.BMP")+sizeof(RomFile)+8);
	res.res_TXTSCDOWN = (u16*)(GetRomFileByName("\\.shell\\bmp\\","TextScrollDown.BMP")+sizeof(RomFile)+8);
	res.res_TXTSCSIGN = (u16*)(GetRomFileByName("\\.shell\\bmp\\","TextScrollButtonSign.BMP")+sizeof(RomFile)+8);
	res.res_TXTSCBACK = (u16*)(GetRomFileByName("\\.shell\\bmp\\","TextScrollBack.BMP")+sizeof(RomFile)+8);
}


//******************************************************************************
//**** 设定当前淡入淡出状态
//**** status=1代表无效果，为0代表为效果最大；
//**** colorWhite=1代表白色效果，为0代表黑色效果
void FadeStatus(u8 status, u8 colorWhite)
{
	if(colorWhite==0)
		REG_BLDMOD = 0xFFFF;
	else
		REG_BLDMOD = 0xFFBF;
	if(status)
		REG_COLEY = 0x0800+16;
	else
		REG_COLEY = 0x0800;
}


//******************************************************************************
//**** 指定时间段的淡入淡出动画效果
//**** fadeIN=0代表淡出（雾化加重），否则淡入（雾化减轻）
//**** colorWhite=1代表白色效果，为0代表黑色效果
//**** time为动画渐变过程中，每帧经历的时间
void FadeInOut(u8 fadeIN, u8 colorWhite, u32 time)
{
	if(colorWhite==0)
		REG_BLDMOD = 0xFFFF;
	else
		REG_BLDMOD = 0xFFBF;
	for(u32 i=0;i<16+1;i++)
	{
		if(fadeIN==0)
			REG_COLEY = 0x0800+i;
		else
			REG_COLEY = 0x0800+16-i;
		sleep(time);
	}
}


//******************************************************************************
//**** ALPHA混合效果，指定两个GB索引，及各自的比例(二者之和为16)
void AlphaBG(u8 bg1index, u8 bg2index,u8 bg1per, u8 bg2per)
{
	REG_BLDMOD = (1<<bg1index)|(1<<(8+bg2index))|(1<<6);//|(1<<4)|(1<<(8+4));
	REG_COLEV = bg1per|(bg2per<<8);
}


//******************************************************************************
//**** 在屏幕显存/缓存上画满幅图像(240x160)
//**** isDrawDirect=1,直接写入显存；=0则写入二级缓存
void DrawBG(u16 *GFX, u8 isDrawDirect)
{
	u16 *p;
	if(isDrawDirect)
		p = VideoBuffer;
	else
		p = Vcache;

	DMA_Copy(3,p,GFX,38400,DMA_16NOW);
}


//******************************************************************************
//**** 在屏幕显存/缓存 x,y处画一个WxH规格的矩形图形
//**** isTrans为1代表是否过滤透明色，tcolor为透明色值
void DrawPic(u16 *GFX, u16 x, u16 y, u16 w, u16 h, u8 isTrans, u16 tcolor, u8 isDrawDirect)
{
	u16 *p,c;
	u16 xi,yi,ww,hh;

	if(isDrawDirect)
		p = VideoBuffer;
	else
		p = Vcache;
	
    hh = (y+h>159)?159:(y+h);
    ww  = (x+w>239)?(239-x):w;
	
	
	if(isTrans)//如果透明显示
	{
		for(yi=y; yi < hh; yi++)
			for(xi=x;xi<x+ww;xi++)
			{
				c = GFX[(yi-y)*w+(xi-x)];
				if(c!=tcolor)
					p[yi*240+xi] = c;
			}
	}
	else		//实心显示
	{
		for(yi=y; yi < hh; yi++)
			DMA_Copy(3,p+yi*240+x,GFX+(yi-y)*w,w,DMA_16NOW);
	}
}


//******************************************************************************
//**** 指定图像路径文件名，在屏幕显存/缓存 x,y处画一个WxH规格的矩形图形
//**** isTrans为1代表是否过滤透明色，tcolor为透明色值
void DrawFilePic(char *filename, u16 x, u16 y, u16 w, u16 h, u8 isTrans, u16 tcolor, u8 isDrawDirect)
{
	//u16 *p,c;
	//u16 xi,yi,ww,hh;
	u16 *GFX;

	GFX = (u16*)(GetRomFileByName("\\.shell\\bmp\\",filename)+sizeof(RomFile)+8);

	DrawPic(GFX,x,y,w,h,isTrans,tcolor,isDrawDirect);
}


//******************************************************************************
//**** 将二级缓存内容同步到显存
void syncVVram()
{
    u16 *p = (u16*)Vcache;
    u16 *v = VideoBuffer;
    DMA_Copy(3,v,p,38400,DMA_16NOW);
}


//******************************************************************************
//**** 用指定颜色清空显存/缓存的一个区域
void Clear(u16 x, u16 y, u16 w, u16 h, u16 c, u8 isDrawDirect)
{
	u16 *p;
	u16 yi,ww,hh;
    
	if(isDrawDirect)
		p = VideoBuffer;
	else
		p = Vcache;

    hh = (y+h>160)?160:(y+h);
    ww  = (x+w>240)?(240-x):w;

	u16 tmp[240];
	for(u8 i=0;i<ww;i++)
		tmp[i] = c;

	for(yi=y; yi < hh; yi++)
        DMA_Copy(3,p+yi*240+x,tmp,ww,DMA_16NOW);
}


//******************************************************************************
//**** 重现某区域的背景图像（使用背景图像相应位置填充某区域）
void ClearWithBG(u16 x, u16 y, u16 w, u16 h, u16* pbg, u8 isDrawDirect)
{
	u16 *p;
	u16 yi,ww,hh;
    
	if(isDrawDirect)
		p = VideoBuffer;
	else
		p = Vcache;

    hh = (y+h>160)?160:(y+h);
    ww  = (x+w>240)?(240-x):w;

	for(yi=y; yi < hh; yi++)
        DMA_Copy(3,p+yi*240+x,pbg+yi*240+x,ww,DMA_16NOW);
}


//******************************************************************************
//**** 二级缓存画点函数
void setPixel(int x, int y, u16 c)
{
	u16 *p = Vcache;
	p[x+y*240] = c;
}


//******************************************************************************
//**** 屏幕显存/缓存画点函数
void setPixel2(u16 *v, int x, int y, u16 c)
{
	v[x+y*240] = c;
}


//******************************************************************************
//**** 画16X8的ASC字串,长度限制为len，如len＝0则自动判断长度
void DrawASCText16(char *str, u32 len, u16 x, u16 y,  u16 c, u8 isDrawDirect)
{
    u32 i,j,k, l;
    
    int location;
	u32 hi=0;
	u8 *p;
	u16 *v;
	u16 *p1 = Vcache;
	u16 *p2 = VideoBuffer;

	if(isDrawDirect)
		v = p2;
	else
		v = p1;

	if(len==0)
		l=strlen(str);
	else
	{
		if(len>strlen(str))
			l=strlen(str);
		else
			l=len;
	}

    while(hi<l)
    {
		if(str[hi]<0x7F+1)  //ASCII code
		{
			location = str[hi]*32;
			//在字库里定位
			p=(u8*)&ASC1616_DATA[location];

			if(str[hi]==' ')
			{
				for(i=0;i<16;i++)
				for(k=0;k<8;k++) 
					setPixel2(v,x+k,y+i,c);
					//DrawHZPixel(x+k,y+i,0);
			}
			else
			{
				for(i=0;i<16;i++)
				for(j=0;j<2;j++)
				for(k=0;k<8;k++) 
				if(((p[i*2+j])>>(7-k))&0x1) 
					setPixel2(v,x+j*8+k,y+i,c);
					//DrawHZPixel(x+j*8+k,y+i,c);
			}
			//x+=16;
			x+=ascwidth[str[hi]];
			hi++;
		}
    }
}


//******************************************************************************
//**** 打印英语音标(12x6格式),长度限制为len，如len=0则自动判断长度
void DrawYinBiao12(char *str, u32 len, u16 x, u16 y, u16 c, u8 isDrawDirect)
{
    u32 i,j,k, l;
    
    int  location;
	u32 hi=0;
	u8 *p;
	u16 *v;
	u16 *p1 = Vcache;
	u16 *p2 = VideoBuffer;

	if(isDrawDirect)
		v = p2;
	else
		v = p1;


	if(len==0)
		l=strlen(str);
	else
	{
		if(len>strlen(str))
			l=strlen(str);
		else
			l=len;
	}

    while(hi<l)
    {
		if(str[hi]<0x7F+1)  //ASCII code
		{
			location = str[hi]*24;
			hi++;
			//在字库里定位
			p=(u8*)&YINBIAO_DATA[location];

			for(i=0;i<12;i++)	//不同字库应变动
			for(j=0;j<2;j++)	//固定为2
			for(k=0;k<8;k++)	//固定为8 
			if(((p[i*2+j])>>(7-k))&0x1)			//固定为7-k
				setPixel2(v,x+j*8+k,y+i,c);
				//DrawHZPixel(x+j*8+k,y+i,c);		//固定为j*8
			x+=10;
			continue;
		}
    }
}


//******************************************************************************
//**** 打印输出一行句子(规格12x12汉字),长度限制为len，如len=0则自动判断长度
//**** (本函数实现稍微复杂，主要是尽量利用加法实现乘法，加快运算速度)
void DrawHZText12(char *str, u16 len, u16 x, u16 y, u16 c, u8 isDrawDirect)
{
    u32 i,l,hi=0;
    u32 location;
	u8 cc,c1,c2;
	u16 *v;
	u16 *p1 = Vcache;
	u16 *p2 = VideoBuffer;
	u16 yy;

	if(isDrawDirect)
		v = p2;
	else
		v = p1;

	if(len==0)
		l=strlen(str);
	else
		if(len>strlen(str))
			l=strlen(str);
		else
			l=len;

	if((u16)(len*6)>(u16)(240-x))
		len=(240-x)/6;
    while(hi<l)
    {
		c1 = str[hi];
    	hi++;
    	if(c1<0x80)  //ASCII 英文单字节字符
    	{
			yy = 240*y;	//yy是显存起始点与该点的整行偏移量
    		location = c1*12;	//字库中偏移量
    		for(i=0;i<12;i++)
			{
				cc = res.res_ASCDATA[location+i];
				if(cc & 0x01)
					v[x+7+yy]=c;
				if(cc & 0x02)
					v[x+6+yy]=c;
				if(cc & 0x04)
					v[x+5+yy]=c;
				if(cc & 0x08)
					v[x+4+yy]=c;
				if(cc & 0x10)
					v[x+3+yy]=c;
				if(cc & 0x20)
					v[x+2+yy]=c;
				if(cc & 0x40)
					v[x+1+yy]=c;
				if(cc & 0x80)
					v[x+yy]=c;
				yy+=240;
			}		
    		x+=6;
    		continue;
    	}
		else	//双字节汉字
		{	
    		c2 = str[hi];
    		hi++;
    		if(c1<0xb0)										//如果是字库前部分的符号
    			location = ((c1-0xa1)*94+(c2-0xa1))*24;
    		else
    			location = (9*94+(c1-0xb0)*94+(c2-0xa1))*24;

			// 根据字符数据画出字符
			yy = 240*y;
			for(i=0;i<12;i++)	//不同字库应变动
			{				
				cc = res.res_ZKDATA[location+i*2]; //前半汉字
				if(cc & 0x01)
					v[x+7+yy]=c;
				if(cc & 0x02)
					v[x+6+yy]=c;
				if(cc & 0x04)
					v[x+5+yy]=c;
				if(cc & 0x08)
					v[x+4+yy]=c;
				if(cc & 0x10)
					v[x+3+yy]=c;
				if(cc & 0x20)
					v[x+2+yy]=c;
				if(cc & 0x40)
					v[x+1+yy]=c;
				if(cc & 0x80)
					v[x+yy]=c;
								
				cc = res.res_ZKDATA[location+i*2+1];//后半汉字
				if(cc & 0x01)
					v[x+15+yy]=c;
				if(cc & 0x02)
					v[x+14+yy]=c;
				if(cc & 0x04)
					v[x+13+yy]=c;
				if(cc & 0x08)
					v[x+12+yy]=c;
				if(cc & 0x10)
					v[x+11+yy]=c;
				if(cc & 0x20)
					v[x+10+yy]=c;
				if(cc & 0x40)
					v[x+9+yy]=c;
				if(cc & 0x80)
					v[x+8+yy]=c;
				yy+=240;
			}
			x+=12;
		}
	}
}


//******************************************************************************
//**** 准备在指定区域中打印汉字段落（自动换行），本函数得到文本行数
//**** 长度限制为len，如len=0则自动判断长度
void DrawHZTextGetLineNum(char *str, u32 len, u16 w, u16 *totalLineNum)
{
    u32 i,hiden,intl, l, rownum,colmaxnum,colnum;
	u32 pi;
	    
	if(len==0)
		l=strlen(str);
	else
		l=len;

	colmaxnum=w/6;

	pi=0;
	rownum=0;
	while(pi<l)
	{
		hiden=0;
		colnum=colmaxnum;
		for(i=0;i<colmaxnum;i++)
		{
			if(pi+i>l-1)
			{
				colnum=i;
				break;
			}
			if(str[pi+i]==0x0D)	//DOS换行 0x0D,0x0A
			{
				if(pi+i<l-1)
					if(str[pi+i+1]==0x0A)
					{
						colnum=i;
						hiden=2;
						break;
					}
			}
			if(str[pi+i]==0x0A)	//DOS换行 0x0D,0x0A
			{
				colnum=i;
				hiden=1;
				break;
			}
		}

		intl=0;//是否截断汉字
		if(colnum==colmaxnum)//自然换行时，判断是否截断汉字
		{
			for(i=0;i<colnum;i++)
			{
				if(str[pi+i]<0x7F)
					intl=0;
				else
				{
					if(intl==0)	//如果上次是整，这次则是半
						intl=1;
					else
						intl=0; //如果上次是半，这次则是整
				}
			}
		}
		if(intl==1)
			colnum--;

		rownum++;
		pi+=colnum+hiden;
	}//while
	*totalLineNum=rownum;
}


//******************************************************************************
//**** 简单的TXT文件阅读器(暂停使用)
void DrawTXTviewer(char *data, u32 size, u32 start, u16 spcLine, u16 scrH, u16 scrY , u16 mode)
{
	u32 i,hiden,intl, l,rownum,colmaxnum,colnum;
	u32 pi;

	//刷新全屏
	if(mode&FM_TXT_LINES)
		DrawBG((u16*)res.res_TXTBG,0);//清屏
			    
	l=size;
	colmaxnum=(240-FM_SCR_W)/6;

	ClearWithBG(240-FM_SCR_W,0,FM_SCR_W,160,res.res_TXTBG,0);
	Clear(240-FM_SCR_W,scrY,FM_SCR_W,scrH,FM_CL_GRAY,0);
	DrawRect(240-FM_SCR_W,0,240-1,160-1,0x4);

	pi=0;
	rownum=0;
	//for(j=0;j<TXT_MSN;j++)
	while(pi<l)
	{
		intl=0;
		hiden=0;
		colnum=colmaxnum;
		for(i=0;i<colmaxnum;i++)//为显示某行文字，做数据准备
		{
			if(pi+i>l-1)
			{
				colnum=i;
				break;
			}
			if(data[pi+i]==0x0D)	//DOS换行 0x0D,0x0A
			{
				if(pi+i<l-1)
					if(data[pi+i+1]==0x0A)
					{
						colnum=i;
						hiden=2;
						break;
					}
			}
			if(data[pi+i]==0x0A)	//DOS换行 0x0D,0x0A
			{
				colnum=i;
				hiden=1;
				break;
			}

			if(data[pi+i]<0x7F)
				intl=0;
			else
			{
				if(intl==0)	//如果上次是整，这次则是半
					intl=1;
				else
					intl=0; //如果上次是半，这次则是整
			}
		}

		if(intl==1)
			colnum--;

		if(rownum+1>start)//从指定行数开始读取显示
		{
			//刷新特殊行
			if(mode&FM_TXT_SPCLINE)
			{	
				if(spcLine==rownum-start)
				{
					Clear(0,spcLine*TXT_ROW_H,240-FM_SCR_W,TXT_ROW_H,FM_CL_BG,0);
					if(colnum>0)
						DrawHZText12(data+pi,colnum,1,1+TXT_ROW_H*(rownum-start),FM_CL_BLACK,0);//打印字串
					return;
				}
			}
			else	//刷新清屏
				if(colnum>0)
					DrawHZText12(data+pi,colnum,1,1+TXT_ROW_H*(rownum-start),FM_CL_BLACK,0);//打印字串
		}

		pi+=colnum+hiden;
		rownum++;
		if(rownum>start+TXT_MSN-1)
			break;
	}//while
}


//******************************************************************************
//**** 在指定区域中打印文本段落，自动换行
//**** 指定每行的高度lineH(推荐14-16)，li指定从哪行开始显示，文本颜色为c
void DrawHZTextRect(char *str, u32 len, u16 x, u16 y, u16 w, u16 h, u16 lineH, u16 li, u16 c, u8 isDrawDirect)
{
    u16 i,hiden,intl, l,rowmaxnum,rownum,colmaxnum,colnum;
	u16 pi;
	    
	if(len==0)
		l=strlen(str);
	else
		l=len;

	rowmaxnum=h/lineH;
	colmaxnum=w/6;

	if(rowmaxnum<2)//如果不能显示多行，则起始显示行为0
		li=0;


	pi=0;
	rownum=0;

	while(pi<l)
	{
		hiden=0;
		colnum=colmaxnum;
		for(i=0;i<colmaxnum;i++)
		{
			if(pi+i>l-1)
			{
				colnum=i;
				break;
			}
			if(str[pi+i]==0x0D)	//DOS换行 0x0D,0x0A
			{
				if(pi+i<l-1)
					if(str[pi+i+1]==0x0A)
					{
						colnum=i;
						hiden=2;
						break;
					}
			}
			if(str[pi+i]==0x0A)	//DOS换行 0x0D,0x0A
			{
				colnum=i;
				hiden=1;
				break;
			}
		}

		intl=0;//是否截断汉字
		if(colnum==colmaxnum)//自然换行时，判断是否截断汉字
		{
			for(i=0;i<colnum;i++)
			{
				if(str[pi+i]<0x7F)
					intl=0;
				else
				{
					if(intl==0)	//如果上次是整，这次则是半
						intl=1;
					else
						intl=0; //如果上次是半，这次则是整
				}
			}
		}
		if(intl==1)
			colnum--;

		if(rownum+1>li)
			if(colnum>0)
				DrawHZText12(str+pi,colnum,x+1,y+1+lineH*(rownum-li),c,isDrawDirect);
		pi+=colnum+hiden;
		rownum++;
		if(rownum>TXT_MSN+li-1)
			break;
		if(rownum>h/TXT_ROW_H+li-1)
			break;
	}//for
}


//******************************************************************************
//**** 根据文件名,返回该文件类型
u32 GetFileType(char *rfname)
{
	int i,pos=1000,el;
	char ext[MAX_PATH];
	char ep[4];
	int l=(int)strlen(rfname);

	for(i=0;i<l;i++)
		if(rfname[i]=='.')
			pos=i;
	if(pos==1000)
		return 0;

	el = l-pos-1;	//扩展名长度
	memcpy(ext,rfname+pos+1,el);
	ext[el]=0;
	for(i=0;i<el;i++)
	{
		ext[i]=(ext[i]>'a'-1)?ext[i]-('a'-'A'):ext[i];
	}
	if(strcmp("TXT",ext)==0)
		return PROC_TXT_VIEW;
	if(strcmp("INI",ext)==0)
		return PROC_TXT_VIEW;
	if(strcmp("INF",ext)==0)
		return PROC_TXT_VIEW;
	if(strcmp("C",ext)==0)
		return PROC_C_VIEW;
	if(strcmp("CPP",ext)==0)
		return PROC_C_VIEW;
	if(strcmp("H",ext)==0)
		return PROC_H_VIEW;
	if(strcmp("HTM",ext)==0)
		return PROC_HTM_VIEW;
	if(strcmp("HTML",ext)==0)
		return PROC_HTM_VIEW;
	if(strcmp("GBA",ext)==0)
		return PROC_GBA_VIEW;
	if(strcmp("BIN",ext)==0)
		return PROC_GBA_VIEW;
	if(strcmp("LZ7",ext)==0)
		return PROC_ZIPGBA_VIEW;
	if(strcmp("SAV",ext)==0)
		return PROC_SAVER_VIEW;
	for(i=0;i<100;i++)
	{
		sprintf(ep,"S%02d",i);
		if(strcmp(ep,ext)==0)
			return PROC_SAVER_VIEW;
	
	}
	for(i=0;i<picExtnum;i++)
	{
		if(strcmp(picExt[i],ext)==0)
			return PROC_JPG_VIEW;		
	}

	return PROC_UNKNOWN;
}


//******************************************************************************
//**** 画线函数，颜色为c
void DrawLine(u16 x, u16 y, u16 x2, u16 y2, u16 c)
{
	u16 i;

    if(x2==x)//竖线
	{
		if(y==y2)
		{
			setPixel(x,y,c);
			return;
		}
    	for(i=0;i<abs(y2-y)+1;i++)
    		setPixel(x,y+i*(abs(y2-y)/(y2-y)),c);

		return;
	}
	if(y2==y)//横线
	{
    	for(i=0;i<abs(x2-x)+1;i++)
    		setPixel(x+i*((x2-x)/abs(x2-x)),y,c);
		return;
	}
	if(abs(x2-x)>abs(y2-y)) //横向斜线
	{
		if(x2>x)
    		for(i=x;i<x2+1;i++)
    			setPixel(i,y+((i-x)*(y2-y))/(x2-x),c);
		else
    		for(i=x;i>x2+1;i--)
    			setPixel(i,y+((i-x)*(y2-y))/(x2-x),c);
	}
	else				//纵向斜线
	{
		if(y2>y)
    		for(i=y;i<y2+1;i++)
    			setPixel(x+((i-y)*(x2-x))/(y2-y),i,c);
		else
    		for(i=y;i>y2+1;i--)
    			setPixel(x+((i-y)*(x2-x))/(y2-y),i,c);
	}
}


//******************************************************************************
//**** 画一个填充四边形
void DrawRect(u16 x, u16 y, u16 x2, u16 y2, u16 c)
{
	DrawLine(x,y,x2,y,c);
	DrawLine(x,y,x,y2,c);
	DrawLine(x2,y2,x2,y,c);
	DrawLine(x2,y2,x,y2,c);
}


//******************************************************************************
//**** 创建带图标的消息窗口，参数分别为标题、消息内容、X、Y、图标集的索引
void CreateWindow(char* caption, char* msg, u16 x, u16 y, u8 iconIndex)
{
	DrawFilePic("itemdisc.bmp",x,y,128,80,1,0x7FFF,1);	//画窗口
    DrawIcon(x+13,y+26,iconIndex,1);					//大图标
	DrawFilePic("ok.bmp",x+7,y+58,24,14,1,0x7FFF,1);	//画OK
	DrawFilePic("x.bmp",x+34,y+58,12,12,1,0x7FFF,1);	//画X
	if(strcmp(caption,"")==0)//空标题，则上移消息数据，负责消息在下面显示
		DrawHZTextRect(msg,0,x+54,y+16,62,56,14,0,2,1);	//写消息
	else
	{
		DrawHZText12(caption,0,x+54,y+16, 3,1);	//粗体标题
		DrawHZText12(caption,0,x+55,y+16, 3,1);
		DrawHZTextRect(msg,0,x+54,y+30,62,40,14,0,2,1);	//写消息
	}
}


//******************************************************************************
//**** 画窗口(暂停使用)
void DrawFrame(char *caption)
{
	//标题
	Clear(0,0,240,FM_TITLE_H,FM_CL_TITLE,0);
	DrawHZText12(caption,240/6, 1,1, FM_CL_BG,0);
	DrawHZText12(caption,240/6, 2,1, FM_CL_BG,0);

	//空白区域
	Clear(0,FM_TITLE_H,240,160-FM_TITLE_H-FM_FOOT_H,FM_CL_BG,0);

	//状态栏
	Clear(0,160-FM_FOOT_H,240,FM_FOOT_H,FM_CL_GRAY,0);
}


//******************************************************************************
//**** 画文件管理器窗口(暂停使用)
void DrawFileMngr(char *caption,char* footmsg, FM_MD_FILE* fmf, u16 oldFi, u16 newFi, u16 scrH, u16 scrY , u16 mode)
{
	u16 i,num;

	//刷新浏览器头
	if(mode&FM_MD_HEAD)
	{
		ClearWithBG(0,0,240,14,res.res_FILEBG,0);
		DrawHZText12(caption,240/6, 1,1, FM_CL_BG,0);
		DrawHZText12(caption,240/6, 2,1, FM_CL_BG,0);
	}

	//刷新浏览器尾
	if(mode&FM_MD_FOOT)
	{
		ClearWithBG(0,160-FM_FOOT_H,240,FM_FOOT_H,res.res_FILEBG,0);
		DrawHZText12(footmsg,240/6, 1,160-FM_FOOT_H+1, FM_CL_BLACK,0);
	}

	//刷新文件区
	if(mode&FM_MD_FILES)
	{
		char FileStr[128];
		ClearWithBG(0,FM_TITLE_H,240-FM_SCR_W,160-FM_TITLE_H-FM_FOOT_H,res.res_FILEBG,0);
		num=(fmf->sectnum>FM_MSN)?FM_MSN:fmf->sectnum;
		for(i=0;i<num;i++)
		{
			DrawHZText12(fmf->sect[i].filename,FM_F_NAME_W/6, FM_F_NAME_X, FM_TITLE_H+2+i*FM_ROW_H, FM_CL_BLACK,0);
			if(fmf->sect[i].isDir)
				DrawFileIcon(3, FM_TITLE_H+2+i*FM_ROW_H,PROC_DIR_VIEW,0);
			else
				DrawFileIcon(3, FM_TITLE_H+2+i*FM_ROW_H,GetFileType(fmf->sect[i].filename),0);

			if(fmf->sect[i].isDir==0)
			{
				if(fmf->sect[i].filesize>999*1024)
					sprintf(FileStr,"%d,%03d KB",(fmf->sect[i].filesize/1024)/1000,(fmf->sect[i].filesize/1024)%1000);
				else if(fmf->sect[i].filesize>9999)
					sprintf(FileStr,"%d KB",fmf->sect[i].filesize/1024);
				else if(fmf->sect[i].filesize>999)
					sprintf(FileStr,"%d,%03d",fmf->sect[i].filesize/1000,fmf->sect[i].filesize%1000);
				else
					sprintf(FileStr,"%d",fmf->sect[i].filesize);
				DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr), FM_TITLE_H+2+i*FM_ROW_H, FM_CL_BLACK,0);
			}
		}
	}

	//刷新选中文件
	if(mode&FM_MD_FOCUS)
	{
		////取消高亮文件
		char FileStr[128];
		u16 w = strlen(fmf->sect[oldFi].filename)*6+2;
		ClearWithBG(FM_F_NAME_X,FM_TITLE_H+1+oldFi*FM_ROW_H,240-FM_SCR_W-FM_F_NAME_X,FM_ROW_H-2,res.res_FILEBG,0);

		DrawHZText12(fmf->sect[oldFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
		if(fmf->sect[oldFi].isDir==0)
		{
			if(fmf->sect[oldFi].filesize>999*1024)
				sprintf(FileStr,"%d,%03d KB",(fmf->sect[oldFi].filesize/1024)/1000,(fmf->sect[oldFi].filesize/1024)%1000);
			else if(fmf->sect[oldFi].filesize>9999)
				sprintf(FileStr,"%d KB",fmf->sect[oldFi].filesize/1024);
			else if(fmf->sect[oldFi].filesize>999)
				sprintf(FileStr,"%d,%03d",fmf->sect[oldFi].filesize/1000,fmf->sect[oldFi].filesize%1000);
			else
				sprintf(FileStr,"%d",fmf->sect[oldFi].filesize);
			DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
		}

		////高亮显示选中文件
		w = strlen(fmf->sect[newFi].filename)*6+2;
		if(fmf->sect[newFi].isDir)
			DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,PROC_DIR_VIEW,0);
		else
			DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,GetFileType(fmf->sect[newFi].filename),0);
		Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,w,FM_ROW_H-2,RGB(3,20,20),0);

		DrawHZText12(fmf->sect[newFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);
		if(fmf->sect[newFi].isDir==0)
		{
			if(fmf->sect[newFi].filesize>999*1024)
				sprintf(FileStr,"%d,%03d KB",(fmf->sect[newFi].filesize/1024)/1000,(fmf->sect[newFi].filesize/1024)%1000);
			else if(fmf->sect[newFi].filesize>9999)
				sprintf(FileStr,"%d KB",fmf->sect[newFi].filesize/1024);
			else if(fmf->sect[newFi].filesize>999)
				sprintf(FileStr,"%d,%03d",fmf->sect[newFi].filesize/1000,fmf->sect[newFi].filesize%1000);
			else
				sprintf(FileStr,"%d",fmf->sect[newFi].filesize);
			DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BLACK,0);
		}
	}

	//刷新拖动条
	if(mode&FM_MD_SCR)
	{
		ClearWithBG(240-FM_SCR_W,FM_TITLE_H,FM_SCR_W,160-FM_TITLE_H-FM_FOOT_H,res.res_FILEBG,0);
		ClearWithBG(240-FM_SCR_W,scrY,FM_SCR_W,scrH,res.res_FILEBG,0);
		DrawRect(240-FM_SCR_W,FM_TITLE_H,240-1,160-FM_FOOT_H-1,0x4);
	}	
}


//******************************************************************************
//**** 移动y1处一条高h的屏幕数据，到y2处(注意数据移动中的重叠)
void moveScreen(u16 y1,u16 y2,u16 h,u8 isDrawDirect)
{
	u16 *p;
	if(isDrawDirect)
		p = VideoBuffer;
	else
		p = Vcache;
	DMA_Copy(3,p+240*y2,p+240*y1,240*h,DMA_16NOW);
}


//******************************************************************************
//**** 在文件管理器中打印一行文件名及大小信息(暂停使用)
void DrawFileRow(u16 rowIndex, char* name, u32 size)
{
	char FileStr[128];
	DrawHZText12(name,FM_F_NAME_W/6, FM_F_NAME_X, FM_TITLE_H+2+rowIndex*FM_ROW_H, 0,0);
	if(size>999*1024)
		sprintf(FileStr,"%d,%03d KB",(size/1024)/1000,(size/1024)%1000);
	else if(size>9999)
		sprintf(FileStr,"%d KB",size/1024);
	else if(size>999)
		sprintf(FileStr,"%d,%03d",size/1000,size%1000);
	else
		sprintf(FileStr,"%d",size);
	DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+rowIndex*FM_ROW_H, 0,0);
}


//******************************************************************************
//**** 在屏幕上显示JPEG文件(须MODE_3模式)
void ShowJPEG(u16 x, u16 y, u8* data)
{
	u16 *p = Vcache;
	JPEG_DecompressImage(data,p,240,160);
}


//******************************************************************************
//**** 指定图标索引，在桌面上指定位置画图标
void DrawIcon(u16 x, u16 y, u8 Ii, u8 isDrawDirect)
{
	if(Ii == PROC_ZIPGBA_VIEW)
		Ii = PROC_GBA_VIEW ;
	DrawPic((u16*)res.res_DESKICON+Ii*24*24,
		x , 
		y , 
		24,
		24,
		1,
		0x7FFF,
		isDrawDirect);
}


//******************************************************************************
//**** 指定小图标索引，在文件管理器指定位置画小图标
void DrawFileIcon(u16 x, u16 y, u8 Ii, u8 isDrawDirect)
{
	if(Ii == PROC_ZIPGBA_VIEW)
		Ii = PROC_GBA_VIEW ;
	DrawPic((u16*)res.res_FILEICON+Ii*16*14,
		x, 
		y, 
		16,
		14,
		1,
		0x7FFF,
		isDrawDirect);
}


//******************************************************************************
//**** 向指定偏移量位置写存盘数据
void save(u32 offset, u8 *data, u16 len)
{
	//最终版提供
	return WriteSram(offset,data,len);
}


//******************************************************************************
//**** 从指定偏移量位置读存盘数据
void load(u32 offset, u8 *data, u16 len)
{
	//最终版提供
	return ReadSram(offset,data,len);
}


//******************************************************************************
//**** 简单解密
void Dec(unsigned char *Indata, int IndataLen, unsigned char *Outdata, int *OutdataLen)
{
	//最终版提供
	return;
}


//******************************************************************************
//**** 屏幕保护函数
void ScreenSaver(char *bg)
{
	//最终版提供
	return;
}

