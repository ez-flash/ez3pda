#include "agb.h"
#include "stdarg.h"
#include "string.h"

#include "keypad.h"
#include "BGfunc.h"
#include "shell.h"
#include "jpeg.h"

#include "screenmode.h"
#include "global.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
//******************************************************************************
//**** 图片浏览器器主函数
#define OAMmem         (u32*)0x7000000
#define VideoBuffer    (u16*)0x6000000
#define OAMdata		   (u16*)0x6100000
#define BGPaletteMem   (u16*)0x5000000
#define OBJPaletteMem  (u16*)0x5000200
#define GAMEPAKMem	   (u8*) 0x8000000
#define Vcache         (u16*)0x2001000
#define IWRAM		   (u16*)0x3000000


int viewPic(u8 *fp)
{
	u16 *p = VideoBuffer;
	u8 *jpgp;
	u16 *vdata, blackdata[480];
	char name[MAX_PATH],needrefresh=1;
	u16 *w,*h,ww,hh, x,y,wpad;
	u16 px=0,py=0,phh=0,pww=0;												//位图显示起始位置

	keypad keyy ;
	keypad *keys = &keyy ;
	memset(blackdata,0,480);
	jpgp=fp ;//fp+HEADSIZE;
	vdata = (u16*)(jpgp + 8);				
	w = (u16*)(jpgp+4);	h = (u16*)(jpgp+6);
	pww = *w; phh = *h;
	if(pww%4!=0)
		wpad=4-(pww%4);

	hh = (phh>160)?160:phh;
	ww = (pww>240)?240:pww;

	//dbgPrint("图像: %dx%d, pxpy: %d/%d",pww,phh,px,py);
	//sleep(1000);
	while(1)
	{
		sleep(15);
		if(needrefresh)
		{
			for(y=0;y<hh;y++)
			{
				DMA_Copy(3,p+240*y,vdata+pww*(y+py)+px,ww,DMA_16NOW);
				if(ww<240)	//空白处补充黑色
					DMA_Copy(3,p+240*y+ww,blackdata,240-ww,DMA_16NOW);
			}
			if(hh<160)		//空白处补充黑色
				for(y=hh;y<160;y++)
				{
					DMA_Copy(3,p+240*y,blackdata,240,DMA_16NOW);
				}

			needrefresh = 0;
		}

		keys->update();
		if (keys->hold(KEY_RIGHT))											//移动鼠标
		{
			for(u8 i=0;i<4;i++)
				if(pww-px>240)
					px++;
			needrefresh = 1;
		}
		if (keys->hold(KEY_LEFT))											//移动鼠标
		{
			for(u8 i=0;i<4;i++)
				if(px>0)
					px--;
			needrefresh = 1;
		}
		if (keys->hold(KEY_DOWN))											//移动鼠标
		{
			for(u8 i=0;i<4;i++)
				if(phh-py>160)
					py++;
			needrefresh = 1;
		}
		if (keys->hold(KEY_UP))											//移动鼠标
		{
			for(u8 i=0;i<4;i++)
				if(py>0)
					py--;
			needrefresh = 1;
		}
		if (keys->release(KEY_B))
		{
			return 0;
		}


	}
}


//******************************************************************************
//**** JPEG图片显示主函数(暂停使用)
int viewJpeg(u8 *fp)
{
	u16 *p = VideoBuffer;
	u16 *v = Vcache;
	u8 *jpgp;
	char name[MAX_PATH],needrefresh=1;
	u16 *w,*h,ww,hh, x,y;
	u16 px=0,py=0,phh=0,pww=0;												//位图显示起始位置

	keypad keyy ;
	keypad *keys = &keyy ;

	jpgp=fp+HEADSIZE;
	JPEG_DecompressImage(jpgp,v,320,218);
	pww = 320; phh = 218;


	hh = (phh>160)?160:phh;
	ww = (pww>240)?240:pww;
	//hh = 218;ww=320;

	while(1)
	{
		sleep(15);
		if(needrefresh)
		{
			for(y=0;y<hh;y++)
				//DMA_Copy(3,p+240*y,v+pww*(y+py)+px+80,ww,DMA_16NOW);
				//for(x=0;x<ww;x++)
				//	p[240*y+x] = v[(y+py)*pww+px+x];
				
				if(y%2==0)
				{
					DMA_Copy(3,p+240*y,v+pww*(y+py)+px,ww,DMA_16NOW);
				}
				else
				{ 
					DMA_Copy(3,p+240*y,v+pww*(y+py)-80+px,ww,DMA_16NOW);
				}
			//syncVVram();
			needrefresh = 0;
		}

		keys->update();
		if (keys->hold(KEY_RIGHT))											//移动鼠标
		{
			if(pww-px>240)
				px++;
			if(pww-px>240)
				px++;
			needrefresh = 1;
		}
		if (keys->hold(KEY_LEFT))											//移动鼠标
		{
			if(px>0)
				px--;
			if(px>0)
				px--;
			needrefresh = 1;
		}
		if (keys->hold(KEY_DOWN))											//移动鼠标
		{
			if(phh-py>160)
				py++;
			if(phh-py>160)
				py++;
			needrefresh = 1;
		}
		if (keys->hold(KEY_UP))											//移动鼠标
		{
			if(py>0)
				py--;
			if(py>0)
				py--;
			needrefresh = 1;
		}
		if (keys->release(KEY_B))
		{
			return 0;
		}


	}
}
