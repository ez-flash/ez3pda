/********************************************************************/
/*          main.cpp                                                */
/*            Source of EZ3Loader                                   */
/*                                                                  */
/*          Copyright (C) 2003-2004            USTC Inc.            */
/********************************************************************/
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"

#include "agb.h"
#include "string.h"
#include "keypad.h"
#include "BGfunc.h"
#include "screenmode.h"
#include "cmddefine.h"
#include "global.h"

#include "shell.h"
#include "inram.h"
#include "viewtext.h"
#include "icons_obj.h"
#include "savermanage.h"
#include "lang.h"
#include "nandop.h"
#include "fat16.h"
#include "hard.h"  
/*-------------------- Global Variables ----------------------------*/
///**************中断处理部分的全局变量**********
u32     IntrMainBuf[0x200];         // Buffer for interrupt main routine
extern "C" void 	intr_main(void);
typedef void (*IntrFuncp)(void);
//const char ezload_ver[] = "Ver1.21A";
extern BYTE  SDorNand ;
extern u32 version ;
res_struct res;  
vu8* HZKLINE;

extern int gameMine();
//int appleBrick();
extern u8* DICTLINE;
//extern u32	 glTotalsize ;
//extern u32  glCursize ;
extern u16  glLoadsaver ;
extern int VblankCount;
extern char CurrentDisk[32];	//当前所在磁盘
extern char CurrentPath[128];		//当前路径
extern char CurrentFileName[128]; //当前选择文件名
extern u16  glExecFileTxt ;
extern u16	 CurrentIndexSelect ;		//所选文件的索引
extern u16	 CurrentIndexView ;			//本页面文件开始的索引
u32  		gl_currentpage ;
u32 		gl_norOffset;				//Nor空闲开始的地方
u32 		gl_norsize;					//NOR Flash 的大小
int 		gl_norgamecounter=0;
EZ4_CARTTYPE g_EZ4CardType;

//*==================================================================*/
//*                      timer1 中断处理例程                                */
//*==================================================================*/
void time1Intr(void)
{
   *(vu16 *)REG_IF	   = TIMER1_INTR_FLAG ;
//   DrawPic(progress[g_time1],40,56,160,48,0,0,1);
	
//	sprintf(tt,"当前进度显示  %08x/%x8x",glCursize,glTotalsize);
//	Clear(60+54,40+30,62,40,0x7FBB,1);
//	DrawHZTextRect(tt,0,60+54,40+30,62,40,14,0,2,1);	//写消息
/*
   g_time1 ++;
   if(g_time1 >= 11)
   	g_time1 = 0 ;
*/
}

#define TM_SELCONT 5 
void TimeSetting()
{
//画背景
	u8 bcreate = 1,brefresh=0,bchangenum=0;
	u8 x,y ;
	u32 timer = 0, rtimer = 0, ti = 0, holdt = 0;

	u16 *pCol =(u16*)(GetRomFileByName("\\.shell\\bmp\\","Time_title.bmp")+sizeof(RomFile)+8);
	CLOCK_TIME clocktime ;	
	char year[6],mon[4],day[4],hour[4],minute[4];
	u8  tm_sel = 0 , tm_selold = 99 ;
	
	keypad keys ;
	bcreate = 1 ;
	x=120;y=48 ;
	while(1)
	{
		VBlankIntrWait();                           // Complete V Blank interrupt
		ti++;
		rtimer=timer;
		if(bcreate)
		{//创建整个背景以及显示时间
			Clear(0,0,240,160,*pCol,0);
			DrawFilePic("Time_title.bmp",22,8,172,32,0,0x7FFF,0);
			DrawFilePic("Time_col.bmp",42,48,48,92,0,0x7FFF,0);
			DrawFilePic("ok.bmp",184,140,24,14,1,0x7FFF,0);	//画OK
			DrawFilePic("x.bmp",214,140,12,12,1,0x7FFF,0);	//画X
			GetTime(&clocktime);
			sprintf(year,"%4d",2000+clocktime.year);
			DrawASCText16(year,0, x, y ,0 , 0 );
			DrawASCText16(year,0, x+1, y+1 ,0 , 0 );
			sprintf(mon,"%02d",clocktime.month);
			DrawASCText16(mon,0, x+8, y+18 ,0 , 0 );
			DrawASCText16(mon,0, x+9, y+19 ,0 , 0 );
			sprintf(day,"%02d",clocktime.data);
			DrawASCText16(day,0, x+8, y+36 ,0 , 0 );
			DrawASCText16(day,0, x+9, y+37 ,0 , 0 );
			sprintf(hour,"%02d",clocktime.hour);
			DrawASCText16(hour,0, x+8, y+54 ,0 , 0 );
			DrawASCText16(hour,0, x+9, y+55 ,0 , 0 );
			sprintf(minute,"%02d",clocktime.minute);
			DrawASCText16(minute,0, x+8, y+72 ,0 , 0 );
			DrawASCText16(minute,0, x+9, y+73 ,0 , 0 );
			syncVVram();
			bcreate = 0 ;
			brefresh = 1 ;
			
		}
		if(brefresh) //清空上次选择，重画
		{
			if(tm_selold == tm_sel)
				continue ;
			else
			{
				if(tm_selold <TM_SELCONT)
				{//先清空
					Clear(x-20,y+18*tm_selold,82,18,*pCol,1);
					switch(tm_selold)
					{
					case 0 :
						sprintf(year,"%4d",2000+clocktime.year);
						DrawASCText16(year,0, x, y ,0 , 1 );
						DrawASCText16(year,0, x+1, y+1 ,0 , 1 );
						break;
					case 1:
						sprintf(mon,"%02d",clocktime.month);
						DrawASCText16(mon,0, x+8, y+18 ,0 , 1 );
						DrawASCText16(mon,0, x+9, y+19 ,0 , 1 );
						break;
					case 2:
						sprintf(day,"%02d",clocktime.data);
						DrawASCText16(day,0, x+8, y+36 ,0 , 1 );
						DrawASCText16(day,0, x+9, y+37 ,0 , 1 );
						break;
					case 3:
						sprintf(hour,"%02d",clocktime.hour);
						DrawASCText16(hour,0, x+8, y+54 ,0 , 1 );
						DrawASCText16(hour,0, x+9, y+55 ,0 , 1 );
						break;
					case 4:
						sprintf(minute,"%02d",clocktime.minute);
						DrawASCText16(minute,0, x+8, y+72 ,0 , 1 );
						DrawASCText16(minute,0, x+9, y+73 ,0 , 1 );
						break;
					}
				}
				//画连个指示图片
				DrawFilePic("arrowL.bmp",x-20,y+18*tm_sel,12,12,1,0x7FFF,1);	//画L
				DrawFilePic("arrowR.bmp",x+48,y+18*tm_sel,12,12,1,0x7FFF,1);	//画R
			}			
			brefresh = 0 ;
		}
		if(bchangenum)
		{
			Clear(x,y+18*tm_sel,40,18,*pCol,1);
			switch(tm_sel)
			{
			case 0 :
				sprintf(year,"%4d",2000+clocktime.year);
				DrawASCText16(year,0, x, y ,0 , 1 );
				DrawASCText16(year,0, x+1, y+1 ,0 , 1 );
				break;
			case 1:
				sprintf(mon,"%02d",clocktime.month);
				DrawASCText16(mon,0, x+8, y+18 ,0 , 1 );
				DrawASCText16(mon,0, x+9, y+19 ,0 , 1 );
				break;
			case 2:
				sprintf(day,"%02d",clocktime.data);
				DrawASCText16(day,0, x+8, y+36 ,0 , 1 );
				DrawASCText16(day,0, x+9, y+37 ,0 , 1 );
				break;
			case 3:
				sprintf(hour,"%02d",clocktime.hour);
				DrawASCText16(hour,0, x+8, y+54 ,0 , 1 );
				DrawASCText16(hour,0, x+9, y+55 ,0 , 1 );
				break;
			case 4:
				sprintf(minute,"%02d",clocktime.minute);
				DrawASCText16(minute,0, x+8, y+72 ,0 , 1 );
				DrawASCText16(minute,0, x+9, y+73 ,0 , 1 );
				break;
			}
			bchangenum = 0 ;
		}
		keys.update();
		if ( (keys.press(KEY_UP)&&(holdt=ti))|| (keys.hold(KEY_UP)&&(ti-holdt>50)) )//向上移动鼠标
		{
			if((tm_sel%TM_SELCONT)==0)
				continue;
			if((tm_sel%TM_SELCONT)>0)
			{
				tm_selold = tm_sel;
				tm_sel--; 
				brefresh = 1;
			}
			
		}
		if ( (keys.press(KEY_DOWN)&&(holdt=ti))|| (keys.hold(KEY_DOWN)&&(ti-holdt>50)) )//向下移动鼠标
		{
			if((tm_sel%TM_SELCONT)==TM_SELCONT-1)
				continue;
			if(tm_sel==TM_SELCONT-1)
				continue;
			if((tm_sel%TM_SELCONT)<TM_SELCONT-1)
			{
				tm_selold = tm_sel;
				tm_sel++; 
				brefresh = 1;
			}
		}		
		if ( (keys.press(KEY_LEFT)&&(holdt=ti))|| (keys.hold(KEY_LEFT)&&(ti-holdt>50)) )//向左移动鼠标
		{
			int sel = (tm_sel%TM_SELCONT);
			if(sel==0)
			{
				if(clocktime.year<=0)
					continue ;
				clocktime.year -- ;		
			}		
			else if(sel == 1)//month
			{
				if(clocktime.month<=1)
					continue ;
				clocktime.month -- ;				
			}
			else if(sel == 2)//day
			{
				if(clocktime.data<=1)
					continue ;
				clocktime.data -- ;				
			}
			else if(sel == 3)//hour
			{
				if(clocktime.hour<=0)
					continue ;
				clocktime.hour -- ;				
			}
			else if(sel == 4)//minute
			{
				if(clocktime.minute<=0)
					continue ;
				clocktime.minute -- ;				
			}
			bchangenum = 1 ;
		}
		if ( (keys.press(KEY_RIGHT)&&(holdt=ti))|| (keys.hold(KEY_RIGHT)&&(ti-holdt>50)) )//向右移动鼠标
		{
			int sel = (tm_sel%TM_SELCONT);
			if(sel==0)
			{
				if(clocktime.year>=99)
					continue ;
				clocktime.year ++ ;				
			}
			else if(sel==1)//month
			{
				if(clocktime.month>=12)
					continue;
				clocktime.month ++ ;				
			}
			else if(sel==2)//day
			{
				if(clocktime.data>=31)
					continue ;
				clocktime.data ++ ;				
			}
			else if(sel==3)//hour
			{
				if(clocktime.hour>=23)
					continue ;
				clocktime.hour ++ ;	
			}
			else if(sel==4)//minute
			{
				if(clocktime.minute>=59)
					continue ;
				clocktime.minute ++ ;
			}
			bchangenum = 1 ;
		}
		if(keys.release(KEY_B))
		{
			break;
		}
		if(keys.release(KEY_A))
		{
			Set24Hour();
			SetTime(&clocktime);
			break;
		}
	}
}

void LangSetting()
{
	int x,y,sel;
	keypad key1 ;
	x = 60;
	y = 40;
	DrawFilePic("itemdisc.bmp",x,y,128,80,1,0x7FFF,1);	//画窗口
    DrawIcon(x+13,y+26,7,1);					//大图标
	DrawFilePic("ok.bmp",x+7,y+58,24,14,1,0x7FFF,1);	//画OK
	DrawFilePic("x.bmp",x+34,y+58,12,12,1,0x7FFF,1);	//画X
	
	DrawHZTextRect("中文",0,x+64,y+26,62,56,14,0,2,1);	//写消息
	DrawHZTextRect("English",0,x+64,y+46,62,56,14,0,2,1);	//写消息
	//DrawHZTextRect("France",0,x+64,y+46,62,56,14,0,2,1);	//写消息
	
	DrawFilePic("arrowR.bmp",x+52,y+26,12,12,1,0x7FFF,1);	//画方向箭头
	sel=0;
	while(1)
	{
		VBlankIntrWait();                           // Complete V Blank interrupt
		key1.update();
		if (key1.press(KEY_UP))
		{
			DrawFilePic("arrowR.bmp",x+52,y+26,12,12,1,0x7FFF,1);	//画方向箭头
			Clear(x+52,y+46,12,12,0x7FBB,1);
			sel = 0 ;
		}
		if(key1.press(KEY_DOWN))
		{
			DrawFilePic("arrowR.bmp",x+52,y+46,12,12,1,0x7FFF,1);	//画方向箭头
			Clear(x+52,y+26,12,12,0x7FBB,1);
			sel =1 ;
		}
		if(key1.release(KEY_A))
		{//设定
			if(sel)
			{//英文
				u16 ulan=1252 ;
				LoadEnglish();
				WriteSram(0x0E0081F0,(u8*)&ulan,2);
			}
			else
			{//中文
				u16 ulan=936 ;
				LoadChinese();
				WriteSram(0x0E0081F0,(u8*)&ulan,2);
			}
			break;
		}
		if(key1.release(KEY_B))
		{
			break;
		}
	}
	
}

void SaverSetting()
{
	int x,y,sel;
	keypad key1 ;
	SAVEFILEINFO saverinfo ;
	u16 lan = CheckLanguage();

	SetRampage(0);	
	ReadSram(_FileInfoSaver,(u8*)&saverinfo,sizeof(SAVEFILEINFO));

	x = 60;
	y = 40;
	DrawFilePic("itemdisc.bmp",x,y,128,80,1,0x7FFF,1);	//画窗口
    	DrawIcon(x+13,y+26,7,1);					//大图标
	DrawFilePic("ok.bmp",x+7,y+58,24,14,1,0x7FFF,1);	//画OK
	DrawFilePic("x.bmp",x+34,y+58,12,12,1,0x7FFF,1);	//画X
	if(lan == 936)
	{
		DrawHZTextRect("自动存档",0,x+64,y+26,62,56,14,0,2,1);	//写消息
		DrawHZTextRect("强制存档",0,x+64,y+46,62,56,14,0,2,1);	//写消息
	}
	else
	{
		DrawHZTextRect("Auto Saving",0,x+64,y+26,62,56,14,0,2,1);	//写消息
		DrawHZTextRect("Obliged Saving",0,x+64,y+46,62,56,14,0,2,1);	//写消息
	}
	if(saverinfo.saverforce)
		sel=saverinfo.saverforce -1;
	else
	{
		sel = 0 ;
		saverinfo.saverforce = 1 ;
	}
	DrawFilePic("arrowR.bmp",x+52,y+26+20*sel,12,12,1,0x7FFF,1);	//画方向箭头
	while(1)
	{
		VBlankIntrWait();                           // Complete V Blank interrupt
		key1.update();
		if (key1.press(KEY_UP))
		{
			DrawFilePic("arrowR.bmp",x+52,y+26,12,12,1,0x7FFF,1);	//画方向箭头
			Clear(x+52,y+46,12,12,0x7FBB,1);
			sel = 0 ;
		}
		if(key1.press(KEY_DOWN))
		{
			DrawFilePic("arrowR.bmp",x+52,y+46,12,12,1,0x7FFF,1);	//画方向箭头
			Clear(x+52,y+26,12,12,0x7FBB,1);
			sel =1 ;
		}
		if(key1.release(KEY_A))
		{//设定
			saverinfo.saverforce = sel +1  ;
			WriteSram(_FileInfoSaver,(u8*)&saverinfo,sizeof(SAVEFILEINFO));
			
			break;
		}
		if(key1.release(KEY_B))
		{
			break;
		}
	}
	
}
//*==================================================================*/
//*                      Main Routine                                */
//*==================================================================*/
void InitialGBA(void)
{
	SetMode (MODE_3 | BG2_ENABLE | OBJ_MAP_1D | OBJ_ENABLE);
//设置中断向量表，
    DmaCopy(3, intr_main, IntrMainBuf, sizeof(IntrMainBuf), 32);// Interrupt main routine set
    *(vu32 *)INTR_VECTOR_BUF = (vu32 )IntrMainBuf;
//设置显示中断
    *(vu16 *)REG_IME   = 1;                           // Set IME
    *(vu16 *)REG_IE    = V_BLANK_INTR_FLAG            // Permit V blank interrupt
                       | CASSETTE_INTR_FLAG	          // Permit cassette interrupt
                       | TIMER1_INTR_FLAG ;
                       
   	*(vu16 *)REG_IF	   = TIMER1_INTR_FLAG ;                    
    *(vu16 *)REG_STAT  = STAT_V_BLANK_IF_ENABLE;
    //*(vu16 *)REG_WAITCNT = CST_ROM0_1ST_3WAIT|CST_ROM0_2ND_1WAIT;
    //*(vu16 *)REG_WAITCNT = CST_ROM0_1ST_3WAIT|CST_ROM0_2ND_1WAIT|CST_PREFETCH_ENABLE|CST_SRAM_2WAIT;
    *(vu16 *)REG_WAITCNT = CST_PREFETCH_ENABLE|CST_SRAM_2WAIT;
    
    //还原page
    SetRampage(0);
	
}

void ShowTimeRightbotom(u8 direct)
{
	char time[32];
	CLOCK_TIME clocktime ;	
	if(SDorNand)
		sprintf(time,"[SD]V%x.%03x",(version&0xFFFF)>>12,(version&0xFFF));
	else
		sprintf(time,"[ND]V%x.%03x",(version&0xFFFF)>>12,(version&0xFFF));
	
	DrawHZText12(time,0,167,3,0,direct);	//写消息
	DrawHZText12(time,0,166,2,0x7FFF,direct);	//写消息

/*
	ClearWithBG(178,135,62,25,res.res_DESKBG,direct);
	GetTime_Orignal(&clocktime);
	sprintf(time,"20%01d%01d-%01d%01d-%01d%01d",
				(clocktime.year>>4),(clocktime.year&0xF),
				(clocktime.month>>4),(clocktime.month&0xF),
				(clocktime.data>>4),(clocktime.data&0xF));
	DrawHZText12(time,0,180,136,0,direct);	//写消息
	DrawHZText12(time,0,179,135,0x7FFF,direct);	//写消息
	sprintf(time,"%01d%01d:%01d%01d:%01d%01d",						
				(clocktime.hour>>4),(clocktime.hour&0xF),
				(clocktime.minute>>4),(clocktime.minute&0xF),
				(clocktime.second>>4),(clocktime.second&0xF));
	DrawHZText12(time,0,186,148,0,direct);	//写消息
	DrawHZText12(time,0,185,147,0x7FFF,direct);	//写消息
	*/
}
//
int ShowDesktop()
{
	u32 timer = 0, rtimer = 0, ti = 0, holdt = 0;
	u32 desktop_flag = DESKTOP_FLAG_CREAT;
	u8 desktop_icon_i=0, desktop_icon_i_old=99;
	u8 needRefresh = 1, showhint = 0, needfade=1;
	u16 desktop_act_t=1;//用于控制桌面动态效果的时间变量
	u16 ax=0,ay=0;
	
	keypad keys ;
	
	FadeInOut(1,0,90);//	渐渐消雾
	
	while(1) 
	{
		VBlankIntrWait();                           // Complete V Blank interrupt
		ti++;
		rtimer=timer;
		if(desktop_flag == DESKTOP_FLAG_CREAT)
		{
			//显示桌面
			DrawBG((u16*)res.res_DESKBG,0);

			for(u8 i=0;i<ICON_COUNT;i++)
    			DrawIcon(DESK_ICON_X0+(i/ICON_FLAG_YCOUNT)*DESK_ICON_DX,DESK_ICON_Y0+DESK_ICON_DY*(i%ICON_FLAG_YCOUNT),desk_icon_list[i],0);
            desktop_flag = DESKTOP_FLAG_UPD;
			needRefresh = 1;
		}
        if(needRefresh)
        {
   			ShowTimeRightbotom(0);
            syncVVram();
            needRefresh = 0;
        }
		if(desktop_flag == DESKTOP_FLAG_UPD)		//正常的桌面显示操作、刷新
		{
			if(showhint == 0)//如果没有提示窗口，显示左箭头
			{
					DrawFilePic("arrowL.bmp",DESK_ICON_X0 + 23 + (desktop_icon_i/ICON_FLAG_YCOUNT)*DESK_ICON_DX,DESK_ICON_Y0+5+(desktop_icon_i%ICON_FLAG_YCOUNT)*DESK_ICON_DY,12,12,1,0x7FFF,1);	//画方向箭头
			}
			else			//如果设置了提示窗口，显示右箭头
			{
				desktop_act_t = 1;
				desktop_icon_i_old = desktop_icon_i;
				ax = DESK_ICON_X0+24+16 + (desktop_icon_i/ICON_FLAG_YCOUNT)*DESK_ICON_DX;	//提示窗口x
				ay = DESK_ICON_Y0+2+(desktop_icon_i%ICON_FLAG_YCOUNT)*DESK_ICON_DY;			//提示窗口y
				if(ay>78)
					ay = 78;

				DrawFilePic("arrowR.bmp",ax-17,DESK_ICON_Y0+5+(desktop_icon_i%ICON_FLAG_YCOUNT)*DESK_ICON_DY,12,12,1,0x7FFF,1);	//画方向箭头
				CreateWindow((char*)gl_hint_title[desktop_icon_i],(char*)gl_hint_content[desktop_icon_i],ax,ay,desk_icon_list[desktop_icon_i]);
				desktop_act_t = 1;		
			}
			desktop_act_t = 0;
			desktop_flag = DESKTOP_FLAG_NULL;
		}
		if(needfade)
		{
			FadeInOut(1,0,50);
			needfade=0;
		}
		keys.update();
		if ( (keys.press(KEY_UP)&&(holdt=ti))|| (keys.hold(KEY_UP)&&(ti-holdt>50)) )//向上移动鼠标
		{
			if((desktop_icon_i%ICON_FLAG_YCOUNT)==0)
				continue;
			if((desktop_icon_i%ICON_FLAG_YCOUNT)>0)
			{
				desktop_icon_i_old = desktop_icon_i;
				desktop_icon_i--; 
				desktop_flag = DESKTOP_FLAG_UPD;
				needRefresh = 1;
			}
			
		}
		if ( (keys.press(KEY_DOWN)&&(holdt=ti))|| (keys.hold(KEY_DOWN)&&(ti-holdt>50)) )//向下移动鼠标
		{
			if((desktop_icon_i%ICON_FLAG_YCOUNT)==ICON_FLAG_YCOUNT-1)
				continue;
			if(desktop_icon_i==ICON_COUNT-1)
				continue;
			if((desktop_icon_i%ICON_FLAG_YCOUNT)<ICON_FLAG_YCOUNT-1)
			{
				desktop_icon_i_old = desktop_icon_i;
				desktop_icon_i++; 
				desktop_flag = DESKTOP_FLAG_UPD;
				needRefresh = 1;
			}
		}
		if ( (keys.press(KEY_LEFT)&&(holdt=ti))|| (keys.hold(KEY_LEFT)&&(ti-holdt>50)) )//向左移动鼠标
		{
			if(desktop_icon_i>ICON_FLAG_YCOUNT-1)
			{
				desktop_icon_i_old = desktop_icon_i;
				desktop_icon_i-=ICON_FLAG_YCOUNT; 
				desktop_flag = DESKTOP_FLAG_UPD;
				needRefresh = 1;
			}
			
		}
		if ( (keys.press(KEY_RIGHT)&&(holdt=ti))|| (keys.hold(KEY_RIGHT)&&(ti-holdt>50)) )//向右移动鼠标
		{
			if(desktop_icon_i+ICON_FLAG_YCOUNT<ICON_COUNT)
			{
				desktop_icon_i_old = desktop_icon_i;
				desktop_icon_i+=ICON_FLAG_YCOUNT; 
				desktop_flag = DESKTOP_FLAG_UPD;
				needRefresh = 1;
			}
			
		}
		if ( keys.hold(KEY_L) && keys.hold(KEY_R)) 
		{
			ScreenSaver("ezpdalogo.bmp");
			desktop_flag = DESKTOP_FLAG_CREAT;
			needfade = 1;
		}
		if (keys.release(KEY_A))
		{	
			if(showhint == 1)
			{
				//////////////////////////////////////////////桌面消息处理////////////////
				//////////////////////////////////////////////桌面消息处理////////////////
				if(desktop_icon_i == 0)//资源管理器
				{
					return PROC_ENTERDISKSHELL;
				}
				if(desktop_icon_i == 1)//存盘点继续
				{
					char tmp[64];
					int isset=0,selsaver=0,get=0;
					FILE_SAVE_INFO fsi;
					u8 *p;
					RomFile* rp;
					//进入下载界面
					
stillinsaver:
					isset = EnterSaver(0,isset) ;
					switch(isset)
					{
					case 0:
						selsaver = SAVETXTPOS ;
						break;
					case 1:
						selsaver = _txtSaver1 ;
						break;
					case 2:
						selsaver = _txtSaver2 ;
						break;
					case 3:
						selsaver = _txtSaver3 ;
						break;
					case 4:
						selsaver = _txtSaver4 ;
						break;
					default :
						selsaver = SAVETXTPOS ;						
					}
					if(isset>=0)
					{
						load(selsaver,(u8*)&fsi,sizeof(FILE_SAVE_INFO));
						if(fsi.flg!='eztx')
							goto stillinsaver ;
					}
					else
					{						
						needfade = 1;
						desktop_flag = DESKTOP_FLAG_CREAT;
						needRefresh = 1;
						continue ;
					}
					
					load(selsaver,(u8*)&fsi,sizeof(FILE_SAVE_INFO));
					//dbgPrint("pos:%d; pi:%d",fsi.pos,fsi.pos2);
					//sleep(1500);	
					sprintf(CurrentDisk,"EZ-Disk");	//当前所在磁盘
					sprintf(CurrentPath,"%s",fsi.path);		//当前路径
					sprintf(CurrentFileName,"%s",fsi.fname); //当前选择文件名
					//搜索是否有当前的文件，如果有继续，没有出现错误。
					sprintf(tmp,"%s%s",fsi.path,fsi.fname);
					fat_init(1);
					get = fat_open(tmp);
					if(get<0)
					{
						showhint = 0;
						desktop_flag = DESKTOP_FLAG_CREAT;
						needRefresh = 1;
						continue ;
					}
					fat_close_withouwrit(get);
					
					CurrentIndexSelect = fsi.fi ;
					CurrentIndexView = fsi.viewfi ;
					glExecFileTxt = EXEC_TXT ;
					if((CurrentPath[0]==0)||(CurrentFileName[0]==0))
					{
						showhint = 0;
						needRefresh = 1;
						continue ;
					}
					
					return PROC_ENTERDISKSHELL ;
				}
/*				if(desktop_icon_i == 2)	//英语学习
				{
					return PROC_EZWORD ;
				}*/
				if(desktop_icon_i == 2)	//扫雷游戏
				{
					u8 gameover=0;
					DrawFilePic("mineBG.bmp",0,0,240,160,0,0,1);	//画窗口
					while(gameover==0)
					{
						while(1){
							sleep(15);	keys.update();

							if(keys.press(KEY_START))
								break;
							if(keys.release(KEY_B))
							{
								gameover=1;			
								break;
							}
						}
						if(gameover)
						{
							needfade = 1;
							desktop_flag = DESKTOP_FLAG_CREAT;
							needRefresh = 1;
							FadeInOut(0,0,50);	//大雾
							break;
						}
						gameMine();
						FadeStatus(1,0);
						SetMode (MODE_3 | BG2_ENABLE | OBJ_MAP_1D | OBJ_ENABLE);
						DrawFilePic("mineBG.bmp",0,0,240,160,0,0,1);	//画窗口
						FadeInOut(1,0,50);
					}
				}
				/*
				if(desktop_icon_i == 4)	//砖块游戏
				{
					FadeInOut(0,0,50);	//大雾
					appleBrick();
					SetMode (MODE_3 | BG2_ENABLE | OBJ_MAP_1D | OBJ_ENABLE);
					needfade = 1;
					desktop_flag = DESKTOP_FLAG_CREAT;
					needRefresh = 1;
				}
				*/
				if(desktop_icon_i == 3)	//设置
				{
					int func_i ;
					while(1)
					{
						func_i = ShowWin_Menu("ezoption.bmp","","",(char**)gl_option_set,2,0);
						if(func_i<0)//
						{
							FadeInOut(0,1,30);//	渐渐雾大
							break;
						}
						else if(func_i==0)
						{//语言设置
							LangSetting();
						}
						else if(func_i>0)
						{//存档方式设置
							SaverSetting();
						}
					}
					needfade = 1;
					desktop_flag = DESKTOP_FLAG_CREAT;
					needRefresh = 1;
				}
				if(desktop_icon_i == 4)	//帮助
				{
					char path[MAX_PATH],fname[MAX_PATH];
					u8 *p;
					RomFile* rp;
					
					strcpy(path,"\\.shell\\");
					strcpy(fname,"help.txt");
					p = GetRomFileByName(path,fname);
					rp = (RomFile*)p;
					viewText(p+sizeof(RomFile),rp->size,0,0);
					FadeInOut(0,0,50);	//大雾
					needfade = 1;
					desktop_flag = DESKTOP_FLAG_CREAT;
				}
			}
			else
			{
				showhint = 1;
				desktop_flag = DESKTOP_FLAG_UPD;
				needRefresh = 1;
			}
		}
		if (keys.press(KEY_B))
		{
			showhint = 0;
			desktop_flag = DESKTOP_FLAG_UPD;
			needRefresh = 1;
		}

		timer++;
		if(!(timer%20))
		{//显示时间
			ShowTimeRightbotom(1);
		}
	}
}

#include "sdopera.h"
/*==================================================================*/
/*                      Main Routine                                */
/*==================================================================*/

int main(void)
{
	u32 timer = 0, rtimer = 0, ti = 0, holdt = 0;
	int shell_return = 0 ;
	int k = 0 , skip = 1 ;
	bool	  ret = 0 ;
	char *pLoader = (char*)0x08040000 ;
	gl_currentpage = 0x8002 ;	//temp lichuan old value 0x8002;


	//open the PSRAM write
	OpenRamWrite();

	InitialGBA();
	initResource();
	
	keypad keyy ;
	keypad *keys = &keyy ;
	
	
	glLoadsaver = 0 ; //检查是否已经加载了存档
	HZKLINE =  GetRomFileByName("\\.shell\\","hzk.dat")+sizeof(RomFile);
	//一定要调用，以确定是什么卡带

	//SD_Test();
	//while(1);
//SD测试
ezcartridge:
	OpenWrite();
	CheckEz4Card();
	EnableNand8bit();
	NAND_Reset();
	k = NAND_ReadID();
	ret = IsValidID(k);
	DisableNandbit();
	CloseWrite();
	SDorNand = 0 ;
	
	if(!ret)
	{
		SD_Enable();
		ret = SD_initial() ;
		SD_Disable();
		if(ret)
		{
			SDorNand = 1 ;
		}
	}
	if(!ret)
	{
		CreateWindow("info","can't find Disk",60,40,1);
		while(1);
	}
	
	//查找升级文件
	k = fat_init(1); //开始升级文件失败

	{
		//CreateWindow("info","fat_init OK",60,40,1);
	}
	
	if(k)	goto welcome ;
	k=fat_open("\\saver\\passme2.sav");	
	if(k>=0)
	{//正常文件,查找到升级文件，准备拷贝ram,passme2
		_stat stat;
		int filesize ;
		u8* pcache = (u8*)0x02020000 ;
		fat_read(k,pcache,0x20000);
		fat_get_stat("\\saver\\passme2.sav",&stat);
		filesize =  stat.FileSize ;
		CreateWindow("info","add passme2.sav >>>",60,40,1);
		fat_read(k,pcache,filesize);	
		//写入存档
		SetRampage(0);
		WriteSram(_SRAMSaver,pcache,filesize);
		fat_close_withouwrit(k);
		CreateWindow("info","add passme2.sav OK",60,40,1);
	}
	
welcome:	//欢迎画面
	sleep(1000);
	DrawFilePic("ezpdalogo.bmp",0,0,240,160,0,0,1);

	FadeInOut(1,0,90);//	渐渐消雾
	for(k=0;k<10;k++)
		VBlankIntrWait();                           // Complete V Blank interrupt
		
	
//语言	
	skip = Checkaladdin();
	u16 lan = CheckLanguage();
	if(lan == 936)//中文
	{
		LoadChinese();
	}
	else if(lan == 1252 )//英文
	{
		LoadEnglish();
	}
	else
	{//选择语言
		LangSetting();
	}
	
	SAVEFILEINFO saverinfo ;
	SetRampage(0);	
	ReadSram(_FileInfoSaver,(u8*)&saverinfo,sizeof(SAVEFILEINFO));
	if(saverinfo.saverforce ==2)
	{
			CreateWindow("info",gl_checkingsaver,60,40,1);
			k = CheckSram(); //需要备份
			if(k)
			{
				SaveCurrentSaverToFlash();
			}
	}
	else
	{
		CreateWindow("info","press L",60,40,1);
		
		if(!skip)
		{
			skip = 1 ;
			CreateWindow(gl_tip,gl_lskip,60,40,1);
			for(k=0 ; k<80 ; k++) 
			{
				VBlankIntrWait();                           // Complete V Blank interrupt
				keys->update();
				if(keys->hold(KEY_L))
				{
					skip = 0 ;
					break;
				}
			}
		}
		if(skip)
		{
			k = CheckSram(); //需要备份
			if(k)
			{
				SaveCurrentSaverToFlash();
			}
		}
	}
	shell_return = PROC_DESKTOP ;
	while(1) 
	{
		sleep(15);
		ti++;
		rtimer=timer; 
		switch (shell_return)
		{
		case PROC_DESKTOP:
			shell_return = ShowDesktop() ;
			break;
		case PROC_ENTERDISKSHELL:
	        	shell_return = runEz3DiskShell();
	        	break;
	        case PROC_GOLDENSELECT :
	        	shell_return = EnterCheatCodeSelect();
	        	break;
	        case PROC_SOFTRESET:
	        	softrest();
	        	break;
	        case PROC_HARDRESET:
	        	hardrest();
	        	break;
	        case PROC_LOADSAVER:	
	        	shell_return = LoadSaver();
	        	break;
	        case PROC_WRITESAVEREX :
	        	shell_return = WriteSaverEx();
	        	break;
	        case PROC_WRITE2NORFLASH :
	        	shell_return = EnterWriteGBAtoNor() ;
	        	break;
	        case PROC_FORMATNORFLASH:
	        	shell_return = FormatNor() ;
	        	break;
	         default:
			;
		}//runShell();

		timer++;

	////////////////////////////////////////////////////////////////////////////////
	}//while
	
}

