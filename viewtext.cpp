#include "agb.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
//#include "math.h"
#include "stdlib.h"

#include "keypad.h"
#include "BGfunc.h"
#include "shell.h"
#include "screenmode.h"
#include "global.h"
#include "savermanage.h"

extern res_struct res;


///////////////////////////////////////////////////////////////////////////////////////////////////////
//******************************************************************************
//**** 准备在指定区域中打印汉字段落，自动换行：先得到总行数
/*
void GetLineNum(char *str, u32 len, u16 w, u32 *totalLineNum)
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

*/
//******************************************************************************
//**** 准备在指定区域中打印汉字段落，自动换行：得到每行字数的数组
void GetLinebuf(char *str, u32 len, u16 w, u16 *linenumbuf, u32 *totalLineNum)
{
    u32 i,hiden,intl, l, rownum,colmaxnum,colnum;
	u32 pi;
	//u16 li=0;

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

		linenumbuf[rownum]=colnum+hiden;
		rownum++;
		pi+=colnum+hiden;
	}//while
	*totalLineNum=rownum;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
//******************************************************************************
//**** 文本浏览器主函数
int viewText(u8 *txtp,u32 size,u32 startline,u32 pi)
{
	u32 i,hiden,intl, l,colnum;
	u32 tmpi;	//pi 是当前文件中的指针
	u8 needrefresh=1,bcreate=1,bscroll=0,bchgpage=1,needfade=0;
	u32 linenum,txtpos,txtnewpos;
	u32 txt_scr_totalH,txt_scr_H,txt_scr_Y,scrY,scrH;
	u16 *linebuf;
	
	keypad keyy ;
	keypad *keys = &keyy ;

	//GetLineNum((char*)txtp,size,TXT_W,&linenum);//获得文件行数
	//linebuf = (u16*)malloc(linenum);
	linebuf = (u16*)(_UnusedEram+0x1000) ;
	GetLinebuf((char*)txtp,size,TXT_W,linebuf,&linenum);//获得每行字节数的数组
	

	while(1)
	{
		VBlankIntrWait(); 
		if(bcreate)
		{
			txtpos=startline;
			txtnewpos=startline;
			//计算拉条
			txt_scr_totalH = (160-16)*100;
			if(linenum>TXT_MSN)
				txt_scr_H=txt_scr_totalH*TXT_MSN/linenum;
			else
				txt_scr_H=txt_scr_totalH;

			if(txt_scr_H<100)
				txt_scr_H=100;

			bcreate = 0;
			bchgpage = 1;
			DrawBG((u16*)res.res_TXTBG,0);
		}
		if(bchgpage)
		{
			tmpi = pi;
			//DrawBG((u16*)res.res_TXTBG,0);
			for(u8 ii=0;ii<TXT_MSN;ii++)
			{
				u8 li = (txtnewpos+ii)%TXT_MSN;
			  //DrawPic((u16*)res.res_TXTBG+li*TXT_ROW_H*240,0,TXT_T + TXT_ROW_H*(TXT_MSN-1),240,TXT_ROW_H+2,0,0,0);
				DrawPic((u16*)res.res_TXTBG+li*TXT_ROW_H*240,0,TXT_T + TXT_ROW_H*(ii),240,TXT_ROW_H,0,0,0);
			    if(txtnewpos+ii>linenum-1)
					break;
				DrawHZText12((char*)(txtp+tmpi),linebuf[txtnewpos+ii],TXT_L, TXT_T + TXT_ROW_H*ii,0,0);
				tmpi+=linebuf[txtnewpos+ii];
			}
			txtpos = txtnewpos;
			bchgpage=0;
			needrefresh = 1;
		}


		if(bscroll)//////////////////////////////////////////////////////////////
		{
			tmpi = pi;
			u8 li;

			if(txtnewpos>txtpos)//下翻
			{
				for(u8 ii=0;ii<TXT_MSN-1;ii++)
				{
					tmpi+=linebuf[txtnewpos+ii];
				}
				for(u8 ii=1;ii<TXT_MSN;ii++)
				{
					moveScreen(TXT_T + TXT_ROW_H*(ii),TXT_T + TXT_ROW_H*(ii-1),TXT_ROW_H,0);
				}
				li = (txtnewpos+TXT_MSN-1)%TXT_MSN;
				DrawPic((u16*)res.res_TXTBG+li*TXT_ROW_H*240,0,TXT_T + TXT_ROW_H*(TXT_MSN-1),240,TXT_ROW_H+2,0,0,0);
				DrawHZText12((char*)(txtp+tmpi),linebuf[txtnewpos+(TXT_MSN-1)],TXT_L, TXT_T + TXT_ROW_H*(TXT_MSN-1),0,0);	
			}
			else				//上翻
			{
				for(int ii=TXT_MSN-2;ii>=0;ii--)
				{
					moveScreen(TXT_T + TXT_ROW_H*(ii),TXT_T + TXT_ROW_H*(ii+1),TXT_ROW_H,0);
				}
				li = (txtnewpos)%TXT_MSN;
				DrawPic((u16*)res.res_TXTBG+li*TXT_ROW_H*240,0,TXT_T,240,TXT_ROW_H,0,0,0);
				DrawHZText12((char*)(txtp+tmpi),linebuf[txtnewpos],TXT_L, TXT_T,0,0);	
			}
			txtpos = txtnewpos;
			bscroll=0;
			needrefresh = 1;
		}
		if(needrefresh)
		{

			txt_scr_Y=100 + txt_scr_totalH*txtnewpos/linenum;
			for(i=0;i<144;i++)
				DrawPic(res.res_TXTSCBACK,232,8+i,8,1,0,0,0);

			for(i=0;i<txt_scr_H/100;i++)
				DrawPic(res.res_TXTSCSIGN,232,7+i+txt_scr_Y/100,8,1,0,0,0);
			DrawPic(res.res_TXTSCUP,232,0,8,8,0,0,0);
			DrawPic(res.res_TXTSCDOWN,232,151,8,8,0,0,0);
			//DrawPic(res.res_TXTSCSIGN,232,40,8,6,0,0,0);
			
			syncVVram();
			{//自动存档
				FILE_SAVE_INFO fsi;
				load(SAVETXTPOS,(u8*)&fsi,sizeof(FILE_SAVE_INFO));
				fsi.pos = txtnewpos;
				fsi.pos2 = pi;
				save(SAVETXTPOS,(u8*)&fsi,sizeof(FILE_SAVE_INFO));			
			}
			needrefresh = 0;
		}
		if(needfade)
		{
			if(needfade)
			{
				FadeInOut(1,0,50);//消雾
				needfade=0;
			}
		}


		//--------------------------------------------------------------------------------
		//--------------------------------------------------------------------------------
		//--------------------------------------------------------------------------------
		keys->update();
		if ( keys->hold(KEY_L) && keys->hold(KEY_R)) 
		{
			ScreenSaver("ezpdalogo.bmp");
			needfade = 1;
			bchgpage=1;
		}
		if (keys->hold(KEY_START))											//移动鼠标
		{
			FILE_SAVE_INFO fsi,fsi2;
			int selsaver ;
savefile:			
			int sel = 0 ;
			sel =  EnterSaver(1,sel);
			if(sel<0)
			{//取消
				bchgpage = 1;
				continue ;
			}
			switch(sel)
			{
			case 0:
				selsaver = _txtSaver1 ;
				break;
			case 1:
				selsaver = _txtSaver2 ;
				break;
			case 2:
				selsaver = _txtSaver3 ;
				break;
			case 3:
				selsaver = _txtSaver4 ;
				break;
			default :
				selsaver = _txtSaver4 ;						
			}
			load(SAVETXTPOS,(u8*)&fsi,sizeof(FILE_SAVE_INFO));
			load(selsaver,(u8*)&fsi2,sizeof(FILE_SAVE_INFO));
			
			if(fsi2.flg=='eztx')
			{//原有存档，需要提示
				CreateWindow("","OverWrite ?",60,40,0);
				while(1)
				{
					VBlankIntrWait(); 
					keys->update();
					if(keys->release(KEY_A))
					{
						break;
					}
					if(keys->release(KEY_B))
					{
						goto savefile ;
					}
				}		
			}
			fsi.flg = 'eztx' ;
			fsi.pos = txtnewpos;
			fsi.pos2 = pi;
			save(selsaver,(u8*)&fsi,sizeof(FILE_SAVE_INFO));
			syncVVram();
			VBlankIntrWait(); 			
			bchgpage = 1;
		}
		if (keys->press(KEY_RIGHT)||keys->hold(KEY_RIGHT))											//移动鼠标
		{
			if(txtpos<linenum-2*TXT_MSN+1)
			{
				for(u16 ii=0;ii<TXT_MSN;ii++)
				{
					pi+=linebuf[txtpos+ii];
				}
				txtnewpos+=TXT_MSN;
				bchgpage = 1;
			}
			else if((txtpos>linenum-2*TXT_MSN)&&(txtpos<linenum-TXT_MSN))
			{
				txtnewpos=linenum-TXT_MSN;
				for(u16 ii=txtpos;ii<txtnewpos;ii++)
				{
					pi+=linebuf[ii];
				}
				bchgpage = 1;
			}
			else
				;
		}
		if (keys->press(KEY_LEFT)||keys->hold(KEY_LEFT))											//移动鼠标
		{
			if(txtpos>TXT_MSN-1)
			{
				txtnewpos-=TXT_MSN;
				for(u8 ii=0;ii<TXT_MSN;ii++)
				{
					pi-=linebuf[txtnewpos+ii];
				}
				bchgpage = 1;
			}
			else if((txtpos>0)&&(txtpos<TXT_MSN))
			{
				pi=0;
				txtnewpos=0;
				bchgpage = 1;
			}
			else
				;
		}
		if (keys->hold(KEY_DOWN))											//移动鼠标
		{
			if(txtpos<linenum-TXT_MSN)
			{
				pi+=linebuf[txtpos];
				txtnewpos++;
				bscroll = 1;
			}
		}
		if (keys->hold(KEY_UP))											//移动鼠标
		{
			if(txtpos>0)
			{
				txtnewpos--;
				pi-=linebuf[txtnewpos];
				bscroll = 1;
			}
		}
		if (keys->release(KEY_B))
		{
			free(linebuf);
			return 0;
		}
	}
}
