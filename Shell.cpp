// Donald Peeke-Vout
// bg.cpp
//  My first adventure with tile backgrounds! Yay!  Looks like a bad
//  case of parallax scrolling to me...


#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"

#include "md5.h"
#include "keypad.h"
#include "BGfunc.h"
#include "shell.h"
#include "viewtext.h"
#include "viewPic.h"

#include "screenmode.h"
#include "cmddefine.h"
#include "global.h"
#include "inram.h"
#include "hard.h"
#include "fat16.h"
#include "savermanage.h"
#include "lang.h"
#include "nandop.h"
#include "SDOpera.h"

extern res_struct res;
char CurrentDisk[32] = {0,0};	//当前所在磁盘
char CurrentPath[maxNameLen*2] = {0,0};		//当前路径
char CurrentFileName[maxNameLen] = {0,0}; //当前选择文件名
u16	 CurrentIndexSelect ;		//所选文件的索引
u16	 CurrentIndexView ;			//本页面文件开始的索引
FM_NOR_FS  glNorFS ;			//保留其信息在软复位时使用
u32	 glTotalsize ;
u32  glCursize ;
u16  glhard ;
u16  glLoadsaver ;
u16  glExecFileTxt ;

extern u32  gl_norsize;
extern u32 gl_currentpage ;
extern u32 gl_norOffset;
extern EZ4_CARTTYPE g_EZ4CardType;
extern int gl_norgamecounter;

//******************************************************************************
//**** 返回一个路径中的斜杠数目
u16 getDirLayerNum(char* path)
{
	u16 num=0,i;
	for(i=0;i<strlen(path);i++)
	{
		if(path[i]=='\\')
			num++;
	}
	return num;
}


//******************************************************************************
//**** 返回一个路径中的文件名
void getDirFileName(char* path, char* filename)
{
	int i=0;
	u16 len;
	len=(u16)strlen(path);
	for(i=len-2;i>=0;i--)
	{
		if(path[i]=='\\')
			break;
	}
	strcpy(filename,path+i+1);
	return;
}


//******************************************************************************
//**** 返回当前目录中ROM文件总数
u16 GetDirFileCount(char *path)
{
	u8 *fsBase,*fsline;
	char fname[MAX_PATH];
	fsBase = GBAPAKLINE; //文件系统起始地址
	RomFile *rf;
	u16 count=0;


	fsline=fsBase;
	rf=(RomFile*)fsline;
	while(rf->FileName[0]!=0)//未到文件末尾
	{
		u16 m,n;
		rf=(RomFile*)fsline;
		fsline=fsline + HEADSIZE + rf->size + rf->padsize;
		m = getDirLayerNum(rf->FileName);
		n = getDirLayerNum(path);
		getDirFileName(rf->FileName,fname);

		if((m==1)&&(n==1))
		{
			if(fname[0]!='.')	//如果目录文件名非 .* 隐藏模式
				count++;
			continue;
		}
		if((n>1)&&(m == n))
		{
			if(memcmp(rf->FileName,path,strlen(path))==0)
				if(fname[0]!='.')	//如果目录文件名非 .* 隐藏模式
					count++;
			continue;
		}
	}
	return count;
}


//******************************************************************************
//**** 根据文件索引号，在指定DIR的ROM中检索某文件，返回该文件地址
u8* GetCurRomFile(char *path,u16 index,u8 isgoon)
{
	static u8* lastpRom=NULL;
	u8 *fsBase,*fsline;
	char fname[MAX_PATH];
	fsBase = GBAPAKLINE; //文件系统起始地址
	RomFile *rf;
	u16 count=0;
	u8 seek1stGoon=0;	

	if((isgoon!=0)&&(lastpRom==NULL))//连续模式的第一次循环
	{
		seek1stGoon=1;
	}

	if(lastpRom==NULL)
        lastpRom=fsBase;

	if(isgoon==0)//非连续读取形式
        lastpRom=fsBase;
	
	fsline=lastpRom;

	rf=(RomFile*)fsline;
	while(rf->FileName[0]!=0)//未到文件末尾
	{
		u16 m,n;

		n = getDirLayerNum(path);
		m = getDirLayerNum(rf->FileName);
		getDirFileName(rf->FileName,fname);

		if((m==1)&&(n==1))
		{
			if(((isgoon==0)&&(count==index)) || (seek1stGoon==0)&&(isgoon==1) ||(seek1stGoon==1)&&(count==index) )
			{
				if(isgoon==1)//连续读取形式
					lastpRom=fsline+HEADSIZE + rf->size + rf->padsize;
				else
					lastpRom=NULL;
				if(fname[0]!='.')	//如果目录文件名非 .* 隐藏模式
					return fsline;
			}
			if(fname[0]!='.')		//如果目录文件名非 .* 隐藏模式
				count++;
		}
		else if((n>1)&&(m == n))
		{

			if(memcmp(rf->FileName,path,strlen(path))==0)
			{


				if(((isgoon==0)&&(count==index)) || (seek1stGoon==0)&&(isgoon==1) ||(seek1stGoon==1)&&(count==index) )
				{
					if(isgoon==1)//连续读取形式
						lastpRom=fsline+HEADSIZE + rf->size + rf->padsize;
					else
						lastpRom=NULL;
					if(fname[0]!='.')
						return fsline;
				}
				if(fname[0]!='.')
					count++;
			}
		}		
		fsline += HEADSIZE + rf->size + rf->padsize;
		rf=(RomFile*)fsline;
	}

	if(isgoon==1)//连续读取形式
		lastpRom=fsline+HEADSIZE + rf->size + rf->padsize;
	else
		lastpRom=NULL;

	//"系统没有找到任何文件!
	//dbgPrint("未找到: %s, i=%d",path,index);
	//exit(1);
	return NULL;
}


//******************************************************************************
//**** 根据文件名和路径，确定并返回该文件指针
u8* GetRomFileByName(char *path, char *Name)
{
	u8 *fsBase,*fsline;
	fsBase = GBAPAKLINE; //文件系统起始地址
	RomFile *rf;
	char tmpbuf[MAX_PATH];

	fsline=fsBase;
	sprintf(tmpbuf,"%s%s",path,Name);
	rf=(RomFile*)fsline;
	
	while(rf->FileName[0]!=0)//未到文件末尾
	{
		if(strcmp(tmpbuf,rf->FileName)==0)
			return fsline;
		fsline += HEADSIZE + rf->size + rf->padsize;
		rf=(RomFile*)fsline;
	}

	//dbgPrint("未找到:%s",Name);
	//exit(1);
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
//******************************************************************************
//**** EZ3磁盘管理器主函数 ** 管理磁盘目录下的文件。
int runEz3DiskShell(void)
{
	u32  shift =0 , orgtt = 123455, dwName = 0 ;
	char msg[maxNameLen],path[maxNameLen*2];
	char ext[6];
	u8 execgo ;
	u8 bcreate=1,bchgpage=1,bscroll=0,bchg=0,needfade=0;
	u16 shell_scr_totalH=0,shell_scr_H=0,shell_scr_Y=0;
	int fi=0,viewfi=0,newfi=0,rfcount=0;
	FM_NOR_FS *pNorFS = (FM_NOR_FS *)_UnusedVram ;
	FM_NOR_FS tmpNorFS ;
	FM_MD_FILE fmf;
	int get,ret;
	_stat stat;
	register int i ;
	u16 count=0;
	int  oldFi,newFi;
	u32 ti = 0 , holdt = 0;
	keypad keys;
	int ctDir=0,ctFile=0 ;
	u32 j;
	u8* pReadCache = (u8*)_ramFatBlockCache ;		
//比较当前保存的目录，以确定搜索地址
	if(strcmp(CurrentDisk,"EZ-Disk"))
	{		
	//首先得到文件列表.
		rfcount = GetFileListFromNor() ;
		gl_norgamecounter = rfcount-1;
	//设定路径
		sprintf(CurrentDisk,"Game Disk");
		CurrentPath[0] = 0 ;
		if(CurrentFileName[0]==0)
		{
		//计算数据显示
			CurrentIndexSelect = 0 ;
			CurrentIndexView = 0;
		}
	}
	else
	{//nand flash 的文件
		FM_NOR_FS *pFile ;
		
		if(CurrentPath[0]==0) //根目录,或者刚刚进入Nand
		{
			CurrentIndexSelect = 0 ;
			CurrentIndexView = 0;
			sprintf(CurrentPath,"\\");
		}
		get = fat_init(0); //不能使用erternal ram
		if(get)
		{
			CreateWindow(gl_warning,gl_faterror,60,40,1);
			while(1){
				sleep(15);	keys.update();
				if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
			}
			sprintf(CurrentDisk,"Game Disk");
			CurrentPath[0] = 0 ;
			return PROC_ENTERDISKSHELL;//返回上级目录
		}
		count = 0 ;
		ctDir = 0 ;
		ctFile = 0;
		pFile = (FM_NOR_FS*)_UnusedEram;
		sprintf(path,"%s",CurrentPath);

		get = fat_getfirst(path,msg);
		while(!get)
		{
		//过滤nds文件
			memcpy(ext,msg+strlen(msg)-4,4);
			ext[4]=0;
/*
/////**************测试得到的文件******************开始
CreateWindow(ext,msg, 10, 10, 1) ;
		while(*(vu16*)0x04000130 == 0x3FF );
		while(*(vu16*)0x04000130 != 0x3FF );
/////**************测试得到的文件 ******************结束*/
			if(!stricmp(ext,".nds"))
			{
				get = fat_getnext(msg);
				continue ;
			}
			if(!strcmp(CurrentPath,"\\"))
				sprintf(path,"%s%s",CurrentPath,msg);
			else
				sprintf(path,"%s\\%s",CurrentPath,msg);
			ret = fat_get_stat(path,&stat);

			strcpy(tmpNorFS.filename,msg);
			//sprintf(tmpNorFS.filename,"%s",msg);
			tmpNorFS.rompage = 0 ;
			tmpNorFS.saversize = 0 ;
			if(ret)
			{
				tmpNorFS.filesize = 0;
				tmpNorFS.savertype = 0x5AA5 ;
				DmaCopy(3,&tmpNorFS,&pNorFS[ctDir],sizeof(FM_NOR_FS),32);			
				ctDir++;
			}
			else
			{
				tmpNorFS.filesize = stat.FileSize ;
				if((stat.Attr == ATTR_DIRECTORY))
				{
					tmpNorFS.savertype = 0x5AA5 ;
					DmaCopy(3,&tmpNorFS,&pNorFS[ctDir],sizeof(FM_NOR_FS),32);			
					ctDir++;
				}
				else
				{
					tmpNorFS.savertype = 0xA55A ;
					DmaCopy(3,&tmpNorFS,&pFile[ctFile],sizeof(FM_NOR_FS),32);			
					ctFile++;
				}
			}
			count ++ ;
			get = fat_getnext(msg);
			if(count>=336)
			break;
		}
	//排序，找到的文件数据需要排序，仅仅文件排序
		if(ctFile>1)
		{
			for(ret=0;ret<ctFile-1;ret++)
			{
				for(i=0;i<ctFile-ret-1;i++)
				{
					get = strcmp(pFile[i].filename,pFile[i+1].filename) ;
					if(get>0)
					{
						DmaCopy(3,&pFile[i+1],&tmpNorFS,sizeof(FM_NOR_FS),32);
						DmaCopy(3,&pFile[i],&pFile[i+1],sizeof(FM_NOR_FS),32);
						DmaCopy(3,&tmpNorFS,&pFile[i],sizeof(FM_NOR_FS),32);					
					}
				}
			}
		}	
	   // 需要整理数据，文件夹放在前面，文件放在后面
        if(ctFile)
		    DmaCopy(3,(u8*)pFile,(u8*)&pNorFS[ctDir],sizeof(FM_NOR_FS)*ctFile,32);
	    rfcount = count ; //count 至少为1
		
	}
	while(1)
	{
		ti++;
        VBlankIntrWait();                           // Complete V Blank interrupt
    		shift ++ ;
		if(shift >16)
		{ //循环显示
			int namelen = strlen(fmf.sect[newFi].filename);
			if(namelen >( FM_F_NAME_W/6 + 8) ) 
			{
				u32  tt = ((shift-16)/8)% namelen;
				if(orgtt!= tt )
				{
					orgtt = tt ;
					sprintf(msg,"%s    ",fmf.sect[newFi].filename + tt);
					strncpy(msg+strlen(msg) ,fmf.sect[newFi].filename , 256 - strlen(msg) );
					msg[255] = 0 ;
					if(fmf.sect[newFi].filename[tt] > 0x80)
					{
					
						if(dwName)
						{
							msg[0] = 0x20 ;
							dwName = 0 ;
						}
						else
							dwName = 1 ;
					}
					else
						dwName = 0;
					Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,220-FM_F_NAME_X,FM_ROW_H-2,RGB(3,20,20),1);
					DrawHZText12(msg,FM_F_NAME_W/6 + 8, FM_F_NAME_X, FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_WHITE,1);
				}
			}
		}

        
		//chsh初始化
		count = 0 ;
		if(bcreate)//////////////////////////////////////////////////////////////
		{
			newfi = CurrentIndexSelect ;
			viewfi = CurrentIndexView ;
		//头部信息

			if(!strcmp(CurrentPath,"\\"))
				sprintf(path,"%s%s",CurrentPath,CurrentFileName);
			else
				sprintf(path,"%s\\%s",CurrentPath,CurrentFileName);
			sprintf(msg,"%s",path);
			if(strlen(msg)>29) //每行最多显示29个字母
			{
				msg[29]=0; msg[28]='.'; msg[27]='.'; msg[26]='.';
			}
			//清空结构体数组
			for(i=0;i<FM_MSN;i++)
			{
				fmf.sectnum=0;
				strcpy(fmf.sect[i].filename,"\0");
				fmf.sect[i].isDir=0;
				fmf.sect[i].filesize=0;
			}

			//逐条设置文件信息
			if(strcmp(CurrentDisk,"EZ-Disk"))
			{//nor
				for(i=viewfi;i<viewfi+FM_MSN;i++)
				{
					if(i>rfcount-1)
						break;
					DmaCopy(3,&pNorFS[i],&tmpNorFS,sizeof(FM_NOR_FS),32);
					sprintf(fmf.sect[i-viewfi].filename,"%s",tmpNorFS.filename);
					fmf.sect[i-viewfi].filesize=tmpNorFS.filesize ;
					if(i==0)
						fmf.sect[i-viewfi].isDir = 1;
					else
						fmf.sect[i-viewfi].isDir = 0;
					count++;
				}
			}
			else
			{//nand flash
			
				for(i=viewfi;i<viewfi+FM_MSN;i++)
				{
					if(i>rfcount-1)
						break;
					DmaCopy(3,&pNorFS[i],&tmpNorFS,sizeof(FM_NOR_FS),32);
					sprintf(fmf.sect[i-viewfi].filename,"%s",tmpNorFS.filename);
					fmf.sect[i-viewfi].filesize=tmpNorFS.filesize ;
					if(tmpNorFS.savertype==0x5AA5)
						fmf.sect[i-viewfi].isDir = 1;
					else if(tmpNorFS.savertype==0xA55A)
						fmf.sect[i-viewfi].isDir = 0;
					count++;
				}
			
			}
			fmf.sectnum=count;
			//计算拉条数据
			shell_scr_totalH = (160-FM_TITLE_H-FM_FOOT_H-3)*100;
			if(rfcount>FM_MSN)
				shell_scr_H=shell_scr_totalH*FM_MSN/rfcount;
			else
				shell_scr_H=shell_scr_totalH;
			
			if(rfcount/FM_MSN == newfi/FM_MSN)
				shell_scr_Y=(FM_TITLE_H+1)*100 + shell_scr_totalH*viewfi/rfcount + 200;
			else
				shell_scr_Y=(FM_TITLE_H+1)*100 + shell_scr_totalH*viewfi/rfcount;

			bcreate = 0;
			bchgpage = 1;
		}
		if(bchgpage)///////////////////换页//////////////////////////////////////
		{
			//刷新浏览器头
			DrawPic(res.res_FILEBGHEAD,0,0,240,FM_TITLE_H,0,0,0);
			//DrawHZText12(msg,0, 60,3, FM_CL_BG,0);
			//画出背景图案
			char FileStr[128];
			u8 num=(fmf.sectnum>FM_MSN)?FM_MSN:fmf.sectnum;
			for(u8 i=0;i<FM_MSN;i++)
			{
				DrawPic((u16*)res.res_FILEBG+((viewfi+i)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+(i)*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			//DrawPic(res.res_FILEBG,0,FM_TITLE_H,240,(FM_ROW_H<<3),0,0,0);
			//画icon,以及文件大小和文件名
			for(u8 i=0;i<num;i++)
			{
				DrawHZText12(fmf.sect[i].filename,FM_F_NAME_W/6, FM_F_NAME_X, FM_TITLE_H+2+i*FM_ROW_H, FM_CL_BLACK,0);
				if(fmf.sect[i].isDir)
					DrawFileIcon(3, FM_TITLE_H+2+i*FM_ROW_H,PROC_DIR_VIEW,0);
				else
					DrawFileIcon(3, FM_TITLE_H+2+i*FM_ROW_H,GetFileType(fmf.sect[i].filename),0);

				if(fmf.sect[i].isDir==0)
				{
					if(fmf.sect[i].filesize>999*1024)
						sprintf(FileStr,"%d,%03d KB",(fmf.sect[i].filesize/1024)/1000,(fmf.sect[i].filesize/1024)%1000);
					else if(fmf.sect[i].filesize>9999)
						sprintf(FileStr,"%d KB",fmf.sect[i].filesize/1024);
					else if(fmf.sect[i].filesize>999)
						sprintf(FileStr,"%d,%03d",fmf.sect[i].filesize/1000,fmf.sect[i].filesize%1000);
					else
						sprintf(FileStr,"%d",fmf.sect[i].filesize);
					DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr), FM_TITLE_H+2+i*FM_ROW_H, FM_CL_BLACK,0);
				}
			}
			//刷新选中文件

			oldFi = fi-viewfi;
			newFi = newfi - viewfi;

			////高亮显示选中文件
			//u16 w = strlen(fmf.sect[newFi].filename)*6+2;
			if(fmf.sect[newFi].isDir)
				DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,PROC_DIR_VIEW,0);
			else
				DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,GetFileType(fmf.sect[newFi].filename),0);
			Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,220-FM_F_NAME_X,FM_ROW_H-2,RGB(3,20,20),0);
			DrawHZText12(fmf.sect[newFi].filename,FM_F_NAME_W/6+8, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);
			/*
			if(fmf.sect[newFi].isDir==0)
			{
				if(fmf.sect[newFi].filesize>999*1024)
					sprintf(FileStr,"%d,%03d KB",(fmf.sect[newFi].filesize/1024)/1000,(fmf.sect[newFi].filesize/1024)%1000);
				else if(fmf.sect[newFi].filesize>9999)
					sprintf(FileStr,"%d KB",fmf.sect[newFi].filesize/1024);
				else if(fmf.sect[newFi].filesize>999)
					sprintf(FileStr,"%d,%03d",fmf.sect[newFi].filesize/1000,fmf.sect[newFi].filesize%1000);
				else
					sprintf(FileStr,"%d",fmf.sect[newFi].filesize);
				DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BLACK,0);
			}
			*/
			syncVVram();
			bchgpage = 0;
		}
		if(bscroll)//////////////////////////////////////////////////////////////
		{
			//刷新文件区
			char FileStr[128];
			u16 count = 0;
			oldFi = fi-viewfi;
			newFi = newfi - viewfi;
			if(newFi == 0)//向上选择，屏幕下滚
			{
				for(u8 ii=FM_MSN-1;ii>0;ii--)
				{
					moveScreen(FM_TITLE_H + FM_ROW_H*(ii-1),FM_TITLE_H + FM_ROW_H*(ii),FM_ROW_H,0);
				}
				DrawPic((u16*)res.res_FILEBG+((viewfi)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+newFi*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			else			//向下选择，屏幕上滚
			{
				for(u8 ii=1;ii<FM_MSN;ii++)
				{
					moveScreen(FM_TITLE_H + FM_ROW_H*(ii),FM_TITLE_H + FM_ROW_H*(ii-1),FM_ROW_H,0);
				}
				DrawPic((u16*)res.res_FILEBG+((fi+1)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+newFi*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			//逐条设置文件信息
			for(u8 i=0;i<fmf.sectnum;i++)
			{
				if(i+viewfi>rfcount-1)
					continue;
				DmaCopy(3,&pNorFS[i+viewfi],&tmpNorFS,sizeof(FM_NOR_FS),32);
				sprintf(fmf.sect[i%FM_MSN].filename,"%s",tmpNorFS.filename);
				fmf.sect[i%FM_MSN].filesize=tmpNorFS.filesize ;
				fmf.sect[i%FM_MSN].isDir = 0;
			}

			//取消高亮文件
			u16 w = strlen(fmf.sect[oldFi].filename)*6+2;
			DrawPic((u16*)res.res_FILEBG+((fi)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+FM_ROW_H*oldFi,240,FM_ROW_H,0,0,0);
			if(fmf.sect[oldFi].isDir)
				DrawFileIcon(3, FM_TITLE_H+2+oldFi*FM_ROW_H,PROC_DIR_VIEW,0);
			else
				DrawFileIcon(3, FM_TITLE_H+2+oldFi*FM_ROW_H,GetFileType(fmf.sect[oldFi].filename),0);
			DrawHZText12(fmf.sect[oldFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
			if(fmf.sect[oldFi].isDir==0)
			{
				if(fmf.sect[oldFi].filesize>999*1024)
					sprintf(FileStr,"%d,%03d KB",(fmf.sect[oldFi].filesize/1024)/1000,(fmf.sect[oldFi].filesize/1024)%1000);
				else if(fmf.sect[oldFi].filesize>9999)
					sprintf(FileStr,"%d KB",fmf.sect[oldFi].filesize/1024);
				else if(fmf.sect[oldFi].filesize>999)
					sprintf(FileStr,"%d,%03d",fmf.sect[oldFi].filesize/1000,fmf.sect[oldFi].filesize%1000);
				else
					sprintf(FileStr,"%d",fmf.sect[oldFi].filesize);
				DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
			}

			//高亮显示选中文件
			//w = strlen(fmf.sect[newFi].filename)*6+2;
			//Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,w,FM_ROW_H-2,RGB(3,20,20),0);//画阴影
			Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,220-FM_F_NAME_X,FM_ROW_H-2,RGB(3,20,20),0);
			if(fmf.sect[newFi].isDir)
				DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,PROC_DIR_VIEW,0);
			else
				DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,GetFileType(fmf.sect[newFi].filename),0);

			DrawHZText12(fmf.sect[newFi].filename,FM_F_NAME_W/6+8, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);//写新背景
/*
			if(fmf.sect[newFi].isDir==0)
			{
				if(fmf.sect[newFi].filesize>999*1024)
					sprintf(FileStr,"%d,%03d KB",(fmf.sect[newFi].filesize/1024)/1000,(fmf.sect[newFi].filesize/1024)%1000);
				else if(fmf.sect[newFi].filesize>9999)
					sprintf(FileStr,"%d KB",fmf.sect[newFi].filesize/1024);
				else if(fmf.sect[newFi].filesize>999)
					sprintf(FileStr,"%d,%03d",fmf.sect[newFi].filesize/1000,fmf.sect[newFi].filesize%1000);
				else
					sprintf(FileStr,"%d",fmf.sect[newFi].filesize);
				DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BLACK,0);
			} */
			syncVVram();
			bscroll = 0;
		}
		if(bchg)//////////////////////////////////////////////////////////////
		{
			//刷新选中文件
			char FileStr[128];

			oldFi = fi-viewfi;
			newFi = newfi - viewfi;
			////取消高亮文件
			u16 w = strlen(fmf.sect[oldFi].filename)*6+2;
			DrawPic((u16*)res.res_FILEBG+((fi)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+FM_ROW_H*oldFi,240,FM_ROW_H,0,0,0);
			if(fmf.sect[oldFi].isDir)
				DrawFileIcon(3, FM_TITLE_H+2+oldFi*FM_ROW_H,PROC_DIR_VIEW,0);
			else
				DrawFileIcon(3, FM_TITLE_H+2+oldFi*FM_ROW_H,GetFileType(fmf.sect[oldFi].filename),0);

			DrawHZText12(fmf.sect[oldFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
			if(fmf.sect[oldFi].isDir==0)
			{
				if(fmf.sect[oldFi].filesize>999*1024)
					sprintf(FileStr,"%d,%03d KB",(fmf.sect[oldFi].filesize/1024)/1000,(fmf.sect[oldFi].filesize/1024)%1000);
				else if(fmf.sect[oldFi].filesize>9999)
					sprintf(FileStr,"%d KB",fmf.sect[oldFi].filesize/1024);
				else if(fmf.sect[oldFi].filesize>999)
					sprintf(FileStr,"%d,%03d",fmf.sect[oldFi].filesize/1000,fmf.sect[oldFi].filesize%1000);
				else
					sprintf(FileStr,"%d",fmf.sect[oldFi].filesize);
				DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
			}

			////高亮显示选中文件
			//w = strlen(fmf.sect[newFi].filename)*6+2;
			if(fmf.sect[newFi].isDir)
				DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,PROC_DIR_VIEW,0);
			else
				DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,GetFileType(fmf.sect[newFi].filename),0);
			Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,220-FM_F_NAME_X,FM_ROW_H-2,RGB(3,20,20),0);
			DrawHZText12(fmf.sect[newFi].filename,FM_F_NAME_W/6+8, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);
/*
			if(fmf.sect[newFi].isDir==0)
			{
				if(fmf.sect[newFi].filesize>999*1024)
					sprintf(FileStr,"%d,%03d KB",(fmf.sect[newFi].filesize/1024)/1000,(fmf.sect[newFi].filesize/1024)%1000);
				else if(fmf.sect[newFi].filesize>9999)
					sprintf(FileStr,"%d KB",fmf.sect[newFi].filesize/1024);
				else if(fmf.sect[newFi].filesize>999)
					sprintf(FileStr,"%d,%03d",fmf.sect[newFi].filesize/1000,fmf.sect[newFi].filesize%1000);
				else
					sprintf(FileStr,"%d",fmf.sect[newFi].filesize);
				DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BLACK,0);
			}		*/	
			bchg = 0;
			syncVVram();
		}
		fi=newfi;
		if(needfade)
		{
			FadeInOut(1,0,50);//消雾
			needfade=0;
		}

		//--------------------------------------------------------------------------------
		//--------------------------------------------------------------------------------
		//--------------------------------------------------------------------------------
		glhard = 0 ;
		keys.update();
/*		if (keys.hold(KEY_L)&&keys.hold(KEY_R)&&keys.hold(KEY_SELECT) )
		{
			return PROC_EZWORD ;
		}		
*/		
		if ( keys.hold(KEY_L) && keys.hold(KEY_R)) 
		{
			shift =0  ;
			return PROC_WRITESAVEREX ;

		/*
			FileIndexOfPath[layerIndex]=newfi;	//纪录文件焦点
			ViewIndexOfPath[layerIndex]=viewfi;	
			ScreenSaver("ezpdalogo.bmp");
			shellflag=SHELL_FLAG_SCR;
			bcreate = 1;
			needfade = 1;
			*/
		}
		if ( (keys.press(KEY_UP))&&(holdt=ti) || (keys.hold(KEY_UP)&&(ti-holdt>15)) )//向上移动鼠标
		{
			shift =0  ;
			if(fi>0)
			{
				newfi--;
				if(viewfi-newfi>0)
				{
					//翻屏上移一个文件
					viewfi--;
					bscroll = 1;
				}
				else
				{
					//本屏上移一个文件
					bchg = 1;
				}
			}
			CurrentIndexSelect = newfi ;
			CurrentIndexView = viewfi ;
		}
		if ( (keys.press(KEY_DOWN))&&(holdt=ti) || (keys.hold(KEY_DOWN)&&(ti-holdt>15)) )//向下移动鼠标
		{
			shift =0  ;
			if(fi<rfcount-1)
			{
				newfi++;
				if(newfi-viewfi>FM_MSN-1)
				{
					//翻屏下移一个文件
					viewfi++;
					bscroll = 1;
				}
				else
				{
					//本屏下移一个文件
					bchg = 1;
				}
			}
			CurrentIndexSelect = newfi ;
			CurrentIndexView = viewfi ;
		}
		if (keys.press(KEY_LEFT)&&(holdt=ti) )
		{//上一页
			shift =0  ;
			if(fi>0)
			{
				newfi -= FM_MSN;
				if(newfi <0)	newfi = 0 ;
				if(viewfi-newfi>0)
				{
					//翻屏上移一个文件
					viewfi -= FM_MSN;
					if(viewfi <0) viewfi = 0 ;
					bcreate = 1;
				}
				else
				{
					//本屏上移一个文件
					bcreate = 1;
				}
				CurrentIndexSelect = newfi ;
				CurrentIndexView = viewfi ;
			}
		}
		if (keys.press(KEY_RIGHT)&&(holdt=ti) )
		{//下一页
			shift =0  ;
			if(fi<rfcount-1)
			{
				newfi+= FM_MSN;
				if(newfi>rfcount-1) newfi = rfcount-1 ;
				if(newfi-viewfi>FM_MSN-1)
				{
					//翻屏下移一个文件
					viewfi+= FM_MSN;
					if(viewfi>rfcount -FM_MSN) viewfi = rfcount -FM_MSN ;
					bcreate = 1 ;
				}
				else
				{
					//本屏下移一个文件
					bcreate = 1;
				}
				CurrentIndexSelect = newfi ; 
				CurrentIndexView = viewfi ;
			}
		}
		if(keys.press(KEY_START)&&(holdt=ti) )
		{
			DmaCopy(3,&pNorFS[CurrentIndexSelect],&glNorFS,sizeof(FM_NOR_FS),32);
			return PROC_FORMATNORFLASH;
			/*
			if(strcmp(CurrentDisk,"EZ-Disk"))
			{
				CreateWindow("",gl_norformat,60,40,1);
				while(1){
					sleep(5);	keys.update();
					if(keys.press(KEY_A))
					{
						break;
					}
					if(keys.press(KEY_B))
					{
							sprintf(CurrentDisk,"Game Disk");
							CurrentPath[0] = 0 ;
							return PROC_ENTERDISKSHELL ;
					}
				}
				OpenWrite();  
				SetSerialMode();
				Block_Erase(0);
				CloseWrite();
				sprintf(CurrentDisk,"Game Disk");
				CurrentPath[0] = 0 ;
				return PROC_ENTERDISKSHELL ;
			}	
			*/		
		}
		if(keys.press(KEY_SELECT)&&(holdt=ti) )
		{
			DmaCopy(3,&pNorFS[CurrentIndexSelect],&glNorFS,sizeof(FM_NOR_FS),32);
			return  PROC_WRITE2NORFLASH ;
		}
		if ((keys.hold(KEY_A))&&(keys.hold(KEY_L)))
		{//硬复位
			shift =0  ;
			glhard = 1 ;
		}
		if (keys.release(KEY_A)||glhard||(glExecFileTxt==EXEC_TXT))
		{
		
			shift =0  ;
			DmaCopy(3,&pNorFS[CurrentIndexSelect],&tmpNorFS,sizeof(FM_NOR_FS),32);
			if(strcmp(CurrentDisk,"Game Disk"))
			{//不一样 ， 在nand中
				if(tmpNorFS.savertype == 0x5AA5)
				{//dir
					if(!strcmp(tmpNorFS.filename,"."))
					{
						continue ;
					}
					if(!strcmp(tmpNorFS.filename,".."))
					{
						if(strcmp(CurrentDisk,"Game Disk"))
						{//不一样 ， 在nand disk中 ,
						
							if(strcmp(CurrentPath,"\\"))
							{//回到上级目录
								char *pt = strrchr(CurrentPath,'\\');
								pt[0]=0;
								
								CurrentIndexSelect = 0 ;
								CurrentIndexView = 0 ;
								return PROC_ENTERDISKSHELL ;
							}
							else
							{
								sprintf(CurrentDisk,"Game Disk");
								CurrentPath[0] = 0 ;
								CurrentIndexSelect = 0 ;
								CurrentIndexView = 0 ;
								return PROC_ENTERDISKSHELL ;
							}
						}
					}
					if(strcmp(CurrentPath,"\\"))
						strcat(CurrentPath,"\\");
					strcat(CurrentPath,tmpNorFS.filename);

					CurrentFileName[0]=0;
					CurrentIndexSelect = 0 ;
					CurrentIndexView = 0 ;
					return PROC_ENTERDISKSHELL ;
				}
				else if((tmpNorFS.savertype == 0xA55A)||(glExecFileTxt==EXEC_TXT))
				{//文件操作
					u8 isopen , isset ;
					int hdfile,first ;
					u8* pReadCache = (u8*)_ramFatBlockCache ;
					u8* prom = (u8*)_Ez3PsRAM ;
					u32 head[2],pos[256],id , j;
					sprintf(CurrentFileName,tmpNorFS.filename);
					DmaCopy(3,&tmpNorFS,&glNorFS,sizeof(FM_NOR_FS),32);
					u32 prc=GetFileType(tmpNorFS.filename);
					//准备读文件部分
					get = fat_init(1);
					if(get)
					{
						CreateWindow(gl_warning,gl_faterror,60,40,1);
						while(1){
							sleep(15);	keys.update();
							if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
						}
						continue ;
					}
					if(!strcmp(CurrentPath,"\\"))
						sprintf(path,"%s%s",CurrentPath,tmpNorFS.filename);
					else
						sprintf(path,"%s\\%s",CurrentPath,tmpNorFS.filename);
					hdfile = fat_open(path);
					if(hdfile<0)
					{
						CreateWindow(gl_warning,gl_nofile,60,40,1);
						while(1){
							sleep(15);	keys.update();
							if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
						}
						bcreate = 1 ;
						continue ;
					}
					
					switch(prc)
					{
					case PROC_UNKNOWN: 					//执行 - 未知
						CreateWindow("info",gl_unkownType,60,40,1);
						while(1){
							sleep(5);	keys.update();
							if(keys.press(KEY_A))
							{
								isopen=1;
								break;
							}
							if(keys.release(KEY_B))
							{
								isopen=0;
								bcreate = 1;
								break;
							}
						}
						if(isopen==0)
							break;
						break;
					case PROC_TXT_VIEW:					//执行 - 文件阅读器
					case PROC_HTM_VIEW:					//执行 - 文件阅读器
					case PROC_C_VIEW:					//执行 - 文件阅读器
					case PROC_H_VIEW:					//执行 - 文件阅读器
						FILE_SAVE_INFO fsi;
						load(SAVETXTPOS,(u8*)&fsi,sizeof(FILE_SAVE_INFO));						
						isset = 1;
						fsi.flg = 'eztx' ;
						fsi.fi = newfi;
						fsi.viewfi = viewfi;
						sprintf(fsi.path,"%s",CurrentPath);
						sprintf(fsi.fname,"%s",CurrentFileName);						
						save(SAVEISSET,(u8*)&isset,2);
						save(SAVETXTPOS,(u8*)&fsi,sizeof(FILE_SAVE_INFO));

					//需要先打开文件然后读到psram中，显示
						CreateWindow(gl_tip,gl_waitread,60,40,1);

						for(i=0;i<tmpNorFS.filesize;i+=0x20000)
						{
							fat_read(hdfile,pReadCache,0x20000);
							OpenWrite();
							DmaCopy(3,pReadCache,prom+i,0x20000,32);
							CloseWrite();
						}
						if(glExecFileTxt==EXEC_TXT)
						{
							viewText(prom,tmpNorFS.filesize,fsi.pos,fsi.pos2);
							glExecFileTxt = 0 ;
						}
						else
						{
							viewText(prom,tmpNorFS.filesize,0,0);
							glExecFileTxt = 0 ;
						}
						sleep(5);
						keys.update();
						VBlankIntrWait();
						bcreate = 1;
						break;
					
					case PROC_JPG_VIEW: 				//执行 - JPG阅读器
					//需要先打开文件然后读到psram中，显示
						CreateWindow(gl_tip,gl_waitread,60,40,1);

						for(i=0;i<tmpNorFS.filesize;i+=0x20000)
						{
							fat_read(hdfile,pReadCache,0x20000);
							OpenWrite();
							DmaCopy(3,pReadCache,(prom+i),0x20000,32);
							CloseWrite();
						}
						viewPic(prom);
						keys.update();
						VBlankIntrWait();
						
						bcreate = 1;
						break;
						
					case PROC_ZIPGBA_VIEW:				//执行zip解压。
						//根据文件类型进行操作。
						fat_read(hdfile,head,8);
						if(head[0]!='lz77')  
						{
							CreateWindow(gl_warning,gl_fileerror,60,40,1);
							while(1){
								sleep(15);	keys.update();
								if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
							}
							bcreate = 1 ;
							break ;
						}
						glTotalsize = tmpNorFS.filesize ;

						StartProgress();
						fat_read(hdfile,pos,head[1]*4);
						first = (head[1]*4+8+15)/16*16 ;
						first = first - (head[1]*4+8) ;
						if(first)
							fat_read(hdfile,pReadCache ,first);
						first = head[1]*0x20000 ;
						
						if(first>0x1000000)
						{//写NOR
							CreateWindow(gl_warning,gl_flashwrite,60,40,1);			
							while(1)
							{
  								VBlankIntrWait();
								keys.update();
								if(keys.press(KEY_A))
								{
									execgo = 1 ;
									break;
								}
								if(keys.release(KEY_B))
								{
									execgo = 0 ;
									break;
								}
							}
							if(execgo)
							{//开始擦除
							
								CreateWindow(gl_warning,gl_waitread,60,40,1);	
								OpenWrite();
								chip_reset();
								id = chip_id();

								chip_erase(id);
								chip_reset();
								StartProgress();
								for(i=0;i<head[1];i++)
								{
									fat_read(hdfile,pReadCache,pos[i]);

									OpenWrite();
									if(pos[i]<0x20000)
									{
										DmaCopy(3,pReadCache,prom,0x20000,32);
										LZ77UnCompWram(prom,pReadCache);
									}
									if(i==0)
										pReadCache[0xBE] = 0xCF ;
									//write to Flash
									for(j=0;j<0x20000;j+=0x8000)
										WriteFlash(id,i*0x20000+j,pReadCache+j,0x8000);
									CloseWrite();
									
														
									glCursize += pos[i] ;
									sprintf(msg,"%s  %d%%",gl_showprog,glCursize*100/glTotalsize);
									Clear(60+54,40+16,62,40,0x7FBB,1);
									DrawHZTextRect(msg,0,60+54,40+16,62,40,14,0,2,1);	//写消息
								}
								OpenWrite();
								chip_reset();
								memset(pReadCache,0,256);
								ldWrite(_Ez3NorCNName,pReadCache,256);

								CloseWrite();
								sprintf(CurrentDisk,"Game Disk");
								return PROC_GOLDENSELECT ;
							}
							else
							{
								bchgpage = 1;
							}
							
						}
						else
						{
							for(i=0;i<head[1];i++)
							{
								fat_read(hdfile,pReadCache,pos[i]);

								OpenWrite();
								if(pos[i]<0x20000)
								{
									DmaCopy(3,pReadCache,(prom+i*0x20000),0x20000,32);
									LZ77UnCompWram((prom+i*0x20000),pReadCache);
									DmaCopy(3,pReadCache,(prom+i*0x20000),0x20000,32);
								}
								else
								{
									DmaCopy(3,pReadCache,(prom+i*0x20000),0x20000,32);
								}
								CloseWrite();
								
													
								glCursize += pos[i] ;
								sprintf(msg,"%s  %d%%",gl_showprog,glCursize*100/glTotalsize);
								Clear(60+54,40+16,62,40,0x7FBB,1);
								DrawHZTextRect(msg,0,60+54,40+16,62,40,14,0,2,1);	//写消息
							}
							StopProgress();	
							return PROC_GOLDENSELECT ;
						}
					case PROC_GBA_VIEW:				//执行拷贝工作。
						glTotalsize = tmpNorFS.filesize ;
						
						u32 lenPsram;
						if(gl_norsize==0x3000000)
							lenPsram = 0x2000000;
						else if(gl_norsize==0x2000000)
							lenPsram = 0x1000000;
						else if(gl_norsize==0x800000)
							lenPsram = 0;
						else
							  lenPsram=0;
						if(glTotalsize>lenPsram)
						{
							if(lenPsram==0)
								CreateWindow(gl_warning,gl_nosupport,60,40,1);	
							else
								CreateWindow(gl_warning,gl_psramout,60,40,1);			
							
							while(1){
								sleep(5);	keys.update();
								if(keys.press(KEY_B) /*|| keys.press(KEY_A)*/)
								{
									//如果选择不写回到当前目录
									CurrentFileName[0]=0;
									CurrentIndexSelect = 0 ;
									CurrentIndexView = 0 ;  
									return PROC_ENTERDISKSHELL ;
								}
							}		  	
						}  
						//如果GBA游戏大于128M,判断PSRAM大小,如果是256M,写入PSRAM
						/*
						if(glTotalsize>0x1000000)
						{
							CreateWindow(gl_warning,gl_flashwrite,60,40,1);			
							while(1)
							{
  								VBlankIntrWait(); 
								keys.update();
								if(keys.press(KEY_A))
								{
									execgo = 1 ;
									break;
								}
								if(keys.release(KEY_B))
								{
									execgo = 0 ;
									break;
								}
							}
							if(execgo)
							{//开始擦除
								CreateWindow(gl_warning,gl_waitread,60,40,1);	
								OpenWrite();
								chip_reset();
								id = chip_id();

								chip_erase(id);
								chip_reset();
								StartProgress();
								
								for(i=0;i<tmpNorFS.filesize;i+=0x8000)
								{
									fat_read(hdfile,pReadCache,0x8000);
									if(i==0)
										pReadCache[0xBE] = 0xCF ;
									OpenWrite();
									WriteFlash(id,i,pReadCache,0x8000);
									CloseWrite();
									glCursize += 0x8000 ;
									sprintf(msg,"%s  %d%%",gl_showprog,glCursize*100/glTotalsize);
									Clear(60+54,40+16,62,40,0x7FBB,1);
									DrawHZTextRect(msg,0,60+54,40+16,62,40,14,0,2,1);	//写消息
								}
								StopProgress();
								OpenWrite();
								chip_reset();
								memset(pReadCache,0,256);
								ldWrite(_Ez3NorCNName,pReadCache,256);

								CloseWrite();
								sprintf(CurrentDisk,"Game Disk");
								return PROC_GOLDENSELECT ;
							//开始写入游戏
							}
							else
							{
								bchgpage = 1;
							}
						}
						else
						*/
						{
						
							*(u8*)0x0E00FFFF = 0x5A ; //写 
							StartProgress();
							
							
							for(i=0;i<tmpNorFS.filesize;i+=0x20000)
							{
								
								fat_read(hdfile,pReadCache,0x20000);
								
								OpenWrite();
								if(i>=0x1000000)
								{
									SetRompage(gl_currentpage+0x800);
									DmaCopy(3,pReadCache,prom+i-0x1000000,0x20000,32);
								}
								else
								{
									DmaCopy(3,pReadCache,prom+i,0x20000,32);
								}
								if(i>=0x1000000)
									SetRompage(gl_currentpage);
								CloseWrite();
								glCursize += 0x20000 ;
								sprintf(msg,"%s  %d%%",gl_showprog,glCursize*100/glTotalsize);
								Clear(60+54,40+16,62,40,0x7FBB,1);
								DrawHZTextRect(msg,0,60+54,40+16,62,40,14,0,2,1);	//写消息
							}
							StopProgress();
							*(u8*)0x0E00FFFF = 0xA5 ; //读

						}						
						//根据文件类型进行操作。
//						return PROC_GOLDENSELECT ;
						return PROC_SOFTRESET;
					case PROC_SAVER_VIEW :
						return PROC_LOADSAVER ;
						
					default:
						break;
					}
				}
				else
				{
					CreateWindow(gl_warning,gl_op_error,60,40,1);			
					while(1){
						sleep(15);	keys.update();
						if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
					}
					bchg = 1;
				}			
			}
			else
			{//一样, 在NorFlash中
				if(newfi == 0)
				{ //进入子目录 Nand disk
					sprintf(CurrentDisk,"EZ-Disk");
					CurrentPath[0] = 0 ;
					CurrentIndexSelect = 0 ;
					CurrentIndexView = 0 ;
					return PROC_ENTERDISKSHELL ;
				}
				else
				{//硬件复位进入游戏
					//需要特别的参数，第几个游戏。										
					sprintf(CurrentFileName,tmpNorFS.filename);
					DmaCopy(3,&tmpNorFS,&glNorFS,sizeof(FM_NOR_FS),32);
					return PROC_GOLDENSELECT ;
				}
			}
		}
		if (keys.release(KEY_B))
		{
			shift =0  ;
			if(strcmp(CurrentDisk,"Game Disk"))
			{//不一样 ， 在nand disk中 ,
			
				if(strcmp(CurrentPath,"\\"))
				{//回到上级目录
					char *pt = strrchr(CurrentPath,'\\');
					pt[0]=0;
					
					CurrentIndexSelect = 0 ;
					CurrentIndexView = 0 ;
					return PROC_ENTERDISKSHELL ;
				}
				else
				{
					sprintf(CurrentDisk,"Game Disk");
					CurrentPath[0] = 0 ;
					CurrentIndexSelect = 0 ;
					CurrentIndexView = 0 ;
					return PROC_ENTERDISKSHELL ;
				}
			}
			
			else
			{//应该回到桌面
				//dbgPrint("OK, RETURN!");
				//sleep(1000);
				FadeInOut(0,0,50);
				return PROC_DESKTOP;
			}
			
		}
		if(keys.release(KEY_R))
		{//显示时间
			shift =0  ;
			ShowTime();
			keys.update();
			VBlankIntrWait();			
			bchg = 1;
			
		}
	}//while
	return 0 ;
}

//******************************************************************************
//**** EZ3磁盘管理器主函数 ** 管理磁盘目录下的文件。
//得到NorFlash文件并写入一个缓存中.
int  GetFileListFromNor()
{
	 *(vu16 *)REG_IME   = 0 ;       
	int page=0 ,count=0,i=0;
	unsigned int StartAddress = _Ez3NorRom;
	FM_NOR_FS *pNorFS = (FM_NOR_FS *)_UnusedVram ;
	FM_NOR_FS tmpNorFS ;
	char temp[256]; 
	vu16  Value;
	//设定第一个为Nand disk目录
	//SetRompage(gl_currentpage) ;
	sprintf(tmpNorFS.filename,"EZ-Disk");
	tmpNorFS.rompage = 0 ;
	tmpNorFS.saversize = 0 ;
	tmpNorFS.filesize = 0 ;
	tmpNorFS.savertype = 0 ;
	DmaCopy(3,&tmpNorFS,pNorFS,sizeof(FM_NOR_FS),32);
	count ++ ;
	gl_norOffset=0;
	
	u8* pReadCache = (u8*)_ramFatBlockCache ;
	ldRead(pReadCache,0x10000);  

//	OpenWrite();
//	CheckEz4Card();
//	CloseWrite();			
//	OpenWrite();
			
	Value = *(vu16 *)(StartAddress + 0xbe);
	while( ((Value&0xFF)==0xCE) || ((Value&0xFF)==0xCF)|| ((Value&0xFF)==0x00))
	{
		if(*(vu8 *)(StartAddress+0xb2) == 0x96)
		{
		
			memcpy(temp,(char*)(StartAddress+0xa0),12);
			temp[12] = 0 ;
			
			if(*(vu16*)(pReadCache+256*(count-1)) == 0 || *(vu16*)(pReadCache+256*(count-1)) == 0xFF)
			{
				for(i=0;i<12;i++)
					if(temp[i] == '.')
						temp[i] = '_';
				sprintf(tmpNorFS.filename,"%s.gba",temp);
			}
			else  
			{
				memcpy(temp,(char*)((pReadCache+256*(count-1))),256);
				temp[255]=0;
				sprintf(tmpNorFS.filename,"%s",temp);
			}	
			tmpNorFS.rompage = (StartAddress - 0x9400000+page*0x800) >> 17;
			tmpNorFS.saversize =(*(vu16 *)(StartAddress + 0xb8+2) & 0x00ff) ;
			tmpNorFS.filesize = ( (*(vu16 *)(StartAddress + 0xb6+2)) << 15) ;
			tmpNorFS.filesize = ((((tmpNorFS.filesize+0x3FFFF)/0x40000)*0x40000));
			gl_norOffset+=tmpNorFS.filesize;
			tmpNorFS.savertype = *(vu8 *)(StartAddress+0xbc) ;
			DmaCopy(3,&tmpNorFS,&pNorFS[count],sizeof(FM_NOR_FS),32);
			count ++ ;
			
		}
		else 
		{
			break;
		}
		
		//if((*(vu16 *)(StartAddress + 0xb6+2)==0)) //游戏大小为0
		//	break;
			

		StartAddress += tmpNorFS.filesize;//(*(vu16 *)(StartAddress + 0xb6+2) << 15);
		
		while(StartAddress>=_Ez3NorRomEnd)
		{
			page += 0x1000 ;
			if(gl_norsize==0x3000000)
			{
				if(page>0x5000) 
				{
					SetRompage(gl_currentpage);
					return count;
				}
			}
			else
			{
				if(page>0x3000) 
				{
					SetRompage(gl_currentpage);
					return count;
				}
			}
			SetRompage(gl_currentpage+page);
			StartAddress -= 0x800000 ;
		}
		if(count>250) break;
		Value = *(vu16 *)(StartAddress + 0xbe);
	}
	SetRompage(gl_currentpage);
	
	*(vu16 *)REG_IME   = 1 ;       
	return count ;
}

//******************************************************************************
//**** EZ3进度条的管理程序
void StartProgress()
{
//开始timer1中断计时，大概每秒钟一次，先画出frame
	glCursize = 0;
	DrawFilePic("itemdisc.bmp",60,40,128,80,1,0x7FFF,1);	//画窗口
	DrawHZText12("进度",0,60+7,40+20, 3,1);	//粗体标题
	DrawHZText12("进度",0,60+8,40+20, 3,1);	
}

void StopProgress()
{
//结束中断计时,主程序需要处理重新显示问题
//	*(vu32*)REG_TM1CNT = 0;
}

typedef struct
{
	char FileName[16];
} CHEATST;

u8   pCheat[256]; //为了保存结果
//******************************************************************************
//**** EZ3金手指项目选择函数。
int EnterCheatCodeSelect()
{//	
	char msg[256],name[256];
	char bk[16] ;
	const u32 cheatcode[] = {0x4640b5ff,0x485eb401,0x4a5e8801,0x2a00400a};
	int	 startpos[256];
	u8   cheatnum[256];
	u8 bcreate=1,bchgpage=1,bscroll=0,bchg=0,needfade=0;
	u16 shell_scr_totalH=0,shell_scr_H=0,shell_scr_Y=0;
	u16 fi=0,viewfi=0,newfi=0,rfcount=0;
 	u16*pPsram;
 	CHEATST *pVBack ;
 	u8* pPos ,*pbPos; //当前显示在那里
 	u32 filestart,skip;
	FM_MD_FILE fmf;
	register int i ;
	u16 count=0;
	u16 oldFi,newFi;
	u32 ti = 0 , holdt = 0 , page , j;
	u32 cheatsize = 0;
	keypad keys;
	pVBack = (CHEATST *)_UnusedVram ;
//比较当前保存的目录，以确定搜索地址                      
	//统计所有的选项
	if(strcmp(CurrentDisk,"EZ-Disk"))
	{//norflash 中
 		pPsram = (u16*)_Ez3PsRAM ; //psram开始的地方
		page = glNorFS.rompage ;
		page = (page>>6)<<12 ;
		
		*(vu16 *)0x9fe0000 = 0x55AA;
		
		SetRompage(gl_currentpage+page); //游戏开始的地方就是
		skip  = (glNorFS.rompage  - (page>>6))<<17 ; //游戏开始的偏移地址
		pPos = (u8*)_Ez3NorRom ;
		filestart = (pPos+skip)[0xB5] + ((pPos+skip)[0xb6]<<8) + ((pPos+skip)[0xb7]<<16) ;
		filestart = filestart<<4 ;
		
		if(filestart == 0)
		{
			memset(name,0,256);
			WriteSram(_GoldenSaver,(u8*)name,256);
			if(glhard)
				return PROC_HARDRESET ;
			else
				return PROC_SOFTRESET;       
		}
		if((filestart + skip )> 0x800000)
		{
			*(vu16 *)0x9fe0000 = 0x5501;
			SetRompage(gl_currentpage+page+((skip+filestart)>>23<<12)); //游戏开始的地方就是

			i = (filestart + skip )%0x800000 ;
			if((i +0x10000)>0x800000) // 游戏数据.
			{
				j=(i +0x10000) - 0x800000 ;
				OpenWrite();
				OpenRamWrite();
				DmaCopy(3,pPos+i,0x2020000,0x10000-j,32);
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				SetRompage(gl_currentpage); 
				DmaCopy(3,0x2020000,pPsram,0x10000-j,32);
				CloseWrite();
				SetRompage(gl_currentpage+page+((skip+filestart)>>23<<12)+0x1000); //游戏开始的地方就是

				OpenWrite();
				OpenRamWrite();
				DmaCopy(3,pPos,0x2020000,j,32);
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				SetRompage(gl_currentpage); 
				DmaCopy(3,0x2020000,((u8*)pPsram)+0x10000-j,j,32);
				CloseWrite();
				
			}
			else //直接拷贝  
			{
				OpenWrite();
				OpenRamWrite();
				*(vu16 *)0x9fe0000 = 0x5502;
				DmaCopy(3,pPos+i,0x2020000,0x10000,32)
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				SetRompage(gl_currentpage); 
				DmaCopy(3,0x2020000,pPsram,0x10000,32)
				u16* pReadCache = (u16*)_ramFatBlockCache ;		
				for(i=0;i<0x8000;i++)
					pPsram[i] = pReadCache[i];
				CloseWrite();
			}
		}
		else
		{//拷贝数据到PSram中，大小0x10000,从XCODE开始
			if((filestart+skip +0x10000)>0x800000) // 游戏数据.
			{
				*(vu16 *)0x9fe0000 = 0x5510;
				i=(filestart+skip +0x10000) - 0x800000 ;
				OpenWrite();
				OpenRamWrite();
				DmaCopy(3,pPos+filestart+skip,0x2020000,0x10000-i,32)
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				*(vu16 *)0x9fe0000 = 0x5511;
				DmaCopy(3,0x2020000,pPsram,0x10000-i,32)
				CloseWrite();
				
				OpenWrite();
				OpenRamWrite();
				SetRompage(gl_currentpage+page+0x1000); //游戏开始的地方就是
				DmaCopy(3,pPos,0x2020000,i,32)
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				SetRompage(gl_currentpage); //游戏开始的地方就是				
				DmaCopy(3,0x2020000,(u8*)pPsram+0x10000-i,i,32)
				CloseWrite();
				
			}  
			else //直接拷贝
			{
				OpenWrite();
				OpenRamWrite();
//				*(u8*)0x0E00FFFF = 0x5A ; //写
				*(vu16 *)0x9fe0000 = 0x5504;
				DmaCopy(3,pPos+filestart+skip,0x2020000,0x10000,32);
				u16* pReadCache = (u16*)_ramFatBlockCache ;		
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				*(vu16 *)0x9fe0000 = 0x5505;
				SetRompage(gl_currentpage); 
				DmaCopy(3,0x2020000,pPsram,0x10000,32)				
				for(i=0;i<0x8000;i++)
					pPsram[i] = pReadCache[i];
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
//				DmaCopy(3,0x2020000,pPsram,0x8000,32)
//				DmaCopy(3,0x2028000,pPsram+0x8000,0x10000,32)
				CloseWrite();
	
			}
		}

		pPos = (u8*)pPsram ;
		SetRompage(gl_currentpage); //
	}
	else
	{//nand flash 的文件
 		pPsram = (u16*)_Ez3PsRAM ; //psram开始的地方
		pPos = (u8*)_Ez3PsRAM ;
		//计算xcode所在的位置．
		filestart = (pPos[0xB5]) + (pPos[0xb6]<<8) + (pPos[0xb7]<<16) ;
		filestart = filestart<<4 ;
		//处理256M PSM 
		if(filestart>=0x1000000)
		{
			SetRompage(gl_currentpage+0x800);
			filestart-=0x1000000;
			
		}
		
		if(filestart ==0)
		{
			memset(name,0,256);
			WriteSram(_GoldenSaver,(u8*)name,256);
			if(glhard)
				return PROC_HARDRESET ;
			else
				return PROC_SOFTRESET;
		}
		pPos = (u8*)_Ez3PsRAM + filestart ;
		
	}
	*(u8*)0x0E00FFFF = 0xA5 ; //读
	*(vu16 *)0x9fe0000 = 0x5512;
	//准备数据
	rfcount = (*((u16*)&pPos[22])) ;
	//首先找到起始地址
	skip = 24+8*(*((u16*)&pPos[20])); // XCODE 大小。		
	cheatsize = 0x1E0 ;
	if(memcmp(cheatcode,&pPos[skip],16))
		cheatsize = 0x6C0 ;
	
	skip += cheatsize*(*((u16*)&pPos[20])) ; //入口程序的大小
	if(rfcount<1)
	{
		memset(name,0,256);
		WriteSram(_GoldenSaver,(u8*)name,256);
		if(glhard)
			return PROC_HARDRESET ;
		else
			return PROC_SOFTRESET;
	}
	if(rfcount>256)
	{
		int isa ;
		CreateWindow(gl_warning,gl_golderror,60,40,1);
		while(1){
			sleep(15);	keys.update();
			if(keys.press(KEY_A))
			{
				isa=1;
				break;
			}
			if(keys.release(KEY_B))
			{
				isa=0;
				break;
			}
		}
		if(isa)
		{
			memset(name,0,256);
			WriteSram(_GoldenSaver,(u8*)name,256);
			if(glhard)
				return PROC_HARDRESET ;
			else
				return PROC_SOFTRESET;
		}
		else
		{
			return  PROC_ENTERDISKSHELL ;
		}
	}
	u16 ccpos ,tcount;
	u16 m = tcount=0 ;
	pPos = pPos + skip ;
	pbPos = pPos ;
	skip = 0 ; page = 0 ;
	for(i=0;i<rfcount;i++)
	{
		ccpos = (*(u16*)&pPos[14+page]);
		DmaCopy(3,&pPos[page],bk,16,32);
		bk[14] = 0;
		DmaCopy(3,bk,pVBack,16,32);
		pVBack ++ ;
		startpos[i] = tcount ;
		cheatnum[i] = ccpos ;
		tcount ++;
		skip += 16 ;
		for(j=0;j<ccpos;j++)
		{
			m = *((u16*)&pPos[skip+14]) ;
			DmaCopy(3,&pPos[skip],bk,16,32);
			bk[14] = 0;
			DmaCopy(3,bk,pVBack,16,32);
			pVBack ++ ;
			tcount ++;
			skip += (m*16) ;
			skip += 16 ;
			page = skip ;
		}
	}
	pVBack = (CHEATST *)_UnusedVram ;
	while(1)
	{
		ti++;
        VBlankIntrWait();                           // Complete V Blank interrupt
		//chsh初始化
		count = 0 ;
		if(bcreate)//////////////////////////////////////////////////////////////
		{
		//头部信息
			//清空结构体数组
			for(u8 i=0;i<FM_MSN;i++)
			{
				fmf.sectnum=0;
				strcpy(fmf.sect[i].filename,"\0");
				fmf.sect[i].isDir=0;
				fmf.sect[i].filesize=0;
			}

			//逐条设置文件信息
			for(i=viewfi;i<viewfi+FM_MSN;i++)
			{
				if(i>rfcount-1)
					break;
				DmaCopy(3,&pVBack[startpos[i]],bk,16,32);
				sprintf(fmf.sect[i-viewfi].filename,"%s",bk);
				count++;
			}
			fmf.sectnum=count;
			//计算拉条数据
			shell_scr_totalH = (160-FM_TITLE_H-FM_FOOT_H-3)*100;
			if(rfcount>FM_MSN)
				shell_scr_H=shell_scr_totalH*FM_MSN/rfcount;
			else
				shell_scr_H=shell_scr_totalH;
			
			if(rfcount/FM_MSN == newfi/FM_MSN)
				shell_scr_Y=(FM_TITLE_H+1)*100 + shell_scr_totalH*viewfi/rfcount + 200;
			else
				shell_scr_Y=(FM_TITLE_H+1)*100 + shell_scr_totalH*viewfi/rfcount;

			bcreate = 0;
			bchgpage = 1;
			//DrawBG((u16*)res.res_TXTBG,0);
		}
		if(bchgpage)///////////////////换页//////////////////////////////////////
		{
			//刷新浏览器头
			sprintf(msg,"%s\\%s",CurrentDisk,CurrentFileName);
			if(strlen(msg)>29) //每行最多显示29个字母
			{
				msg[29]=0; msg[28]='.'; msg[27]='.'; msg[26]='.';
			}
			DrawPic(res.res_FILEBGHEAD,0,0,240,FM_TITLE_H,0,0,0);
			DrawHZText12(msg,0, 60,3, FM_CL_BG,0);
			//画出背景图案
			char FileStr[128];
			u8 num=(fmf.sectnum>FM_MSN)?FM_MSN:fmf.sectnum;
			for(u8 i=0;i<FM_MSN;i++)
			{
				DrawPic((u16*)res.res_FILEBG+((viewfi+i)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+(i)*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			//DrawPic(res.res_FILEBG,0,FM_TITLE_H,240,(FM_ROW_H<<3),0,0,0);
			//画icon,以及文件大小和文件名
			oldFi = fi-viewfi;
			newFi = newfi - viewfi;

			for(u8 i=0;i<num;i++)
			{
				DrawHZText12(fmf.sect[i].filename,FM_F_NAME_W/6, FM_F_NAME_X, FM_TITLE_H+2+i*FM_ROW_H, FM_CL_BLACK,0);
				if(newFi!=i+viewfi)
				{
					if(pCheat[i+viewfi] == 0)
						sprintf(FileStr,"OFF");
					else 
					{
						DmaCopy(3,&pVBack[startpos[i+viewfi]+pCheat[i+viewfi]],bk,16,32);
						sprintf(FileStr,"%s",bk);
					}
				}
				DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr), FM_TITLE_H+2+i*FM_ROW_H, FM_CL_BLACK,0);
			}
			//刷新选中文件

			////高亮显示选中文件
			u16 w = strlen(fmf.sect[newFi].filename)*6+2;
			Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,w,FM_ROW_H-2,RGB(3,20,20),0);

			DrawHZText12(fmf.sect[newFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);
			if(pCheat[newfi] == 0)
				sprintf(FileStr,"OFF");
			else 
			{
				DmaCopy(3,&pVBack[startpos[newfi]+pCheat[newfi]],bk,16,32);
				sprintf(FileStr,"%s",bk);
			}
			DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BLACK,0);
			syncVVram();
			bchgpage = 0;
		}
		if(bscroll)//////////////////////////////////////////////////////////////
		{
			//刷新文件区
			char FileStr[128];
			u16 count = 0;
			oldFi = fi-viewfi;
			newFi = newfi - viewfi;
			if(newFi == 0)//向上选择，屏幕下滚
			{
				for(u8 ii=FM_MSN-1;ii>0;ii--)
				{
					moveScreen(FM_TITLE_H + FM_ROW_H*(ii-1),FM_TITLE_H + FM_ROW_H*(ii),FM_ROW_H,0);
				}
				DrawPic((u16*)res.res_FILEBG+((viewfi)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+newFi*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			else			//向下选择，屏幕上滚
			{
				for(u8 ii=1;ii<FM_MSN;ii++)
				{
					moveScreen(FM_TITLE_H + FM_ROW_H*(ii),FM_TITLE_H + FM_ROW_H*(ii-1),FM_ROW_H,0);
				}
				DrawPic((u16*)res.res_FILEBG+((fi+1)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+newFi*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			//逐条设置文件信息
			for(u8 i=0;i<fmf.sectnum;i++)
			{
				if(i+viewfi>rfcount-1)
					continue;
				DmaCopy(3,&pVBack[startpos[i+viewfi]],bk,16,32);
				sprintf(fmf.sect[i%FM_MSN].filename,"%s",bk);
			}

			//取消高亮文件
			u16 w = strlen(fmf.sect[oldFi].filename)*6+2;
			DrawPic((u16*)res.res_FILEBG+((fi)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+FM_ROW_H*oldFi,240,FM_ROW_H,0,0,0);
			DrawHZText12(fmf.sect[oldFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
			if(pCheat[fi] == 0)
				sprintf(FileStr,"OFF");
			else 
			{
				DmaCopy(3,&pVBack[startpos[fi]+pCheat[fi]],bk,16,32);
				sprintf(FileStr,"%s",bk);
			}
			DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);

			//高亮显示选中文件
			w = strlen(fmf.sect[newFi].filename)*6+2;
			Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,w,FM_ROW_H-2,RGB(3,20,20),0);//画阴影

			DrawHZText12(fmf.sect[newFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);//写新背景
			if(pCheat[newfi] == 0)
				sprintf(FileStr,"OFF");
			else 
			{
				DmaCopy(3,&pVBack[startpos[newfi]+pCheat[newfi]],bk,16,32);
				sprintf(FileStr,"%s",bk);
			}
			DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BLACK,0);
			syncVVram();
			bscroll = 0;
		}
		if(bchg)//////////////////////////////////////////////////////////////
		{
			//刷新选中文件
			char FileStr[128];
			u16 w ;
			oldFi = fi-viewfi;
			newFi = newfi - viewfi;
			////取消高亮文件
			w = strlen(fmf.sect[oldFi].filename)*6+2;
			DrawPic((u16*)res.res_FILEBG+((fi)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+FM_ROW_H*oldFi,240,FM_ROW_H,0,0,0);
			if(oldFi != newFi)
			{
				DrawHZText12(fmf.sect[oldFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
				if(pCheat[fi] == 0)
					sprintf(FileStr,"OFF");
				else 
				{
					DmaCopy(3,&pVBack[startpos[fi]+pCheat[fi]],bk,16,32);
					sprintf(FileStr,"%s",bk);
				}
				DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+oldFi*FM_ROW_H, FM_CL_BLACK,0);
			}
			////高亮显示选中文件
			w = strlen(fmf.sect[newFi].filename)*6+2;
			Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,w,FM_ROW_H-2,RGB(3,20,20),0);

			DrawHZText12(fmf.sect[newFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);
			if(pCheat[newfi] == 0)
				sprintf(FileStr,"OFF");
			else 
			{
				DmaCopy(3,&pVBack[startpos[newfi]+pCheat[newfi]],bk,16,32);
				sprintf(FileStr,"%s",bk);
			}
			DrawHZText12(FileStr,FM_F_SIZE_W/6, FM_F_SIZE_X2-6*strlen(FileStr),FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BLACK,0);
			bchg = 0;
			syncVVram();
		}
		fi=newfi;
		if(needfade)
		{
			FadeInOut(1,0,50);//消雾
			needfade=0;
		}

		//--------------------------------------------------------------------------------
		//--------------------------------------------------------------------------------
		//--------------------------------------------------------------------------------
		keys.update();
		if ( (keys.press(KEY_UP))&&(holdt=ti) || (keys.hold(KEY_UP)&&(ti-holdt>15)) )//向上移动鼠标
		{
			if(fi>0)
			{
				newfi--;
				if(viewfi-newfi>0)
				{
					//翻屏上移一个文件
					viewfi--;
					bscroll = 1;
				}
				else
				{
					//本屏上移一个文件
					bchg = 1;
				}
			}
		}
		if ( (keys.press(KEY_DOWN))&&(holdt=ti) || (keys.hold(KEY_DOWN)&&(ti-holdt>15)) )//向下移动鼠标
		{
			if(fi<rfcount-1)
			{
				newfi++;
				if(newfi-viewfi>FM_MSN-1)
				{
					//翻屏下移一个文件
					viewfi++;
					bscroll = 1;
				}
				else
				{
					//本屏下移一个文件
					bchg = 1;
				}
			}
		}
		if (keys.press(KEY_A))
		{
			pCheat[newfi] ++ ;
			if(pCheat[newfi]>cheatnum[newfi])
				pCheat[newfi] = 0 ;
			bchg = 1;
			
		}
		if (keys.press(KEY_SELECT))
		{
			return PROC_ENTERDISKSHELL ; //返回
		}
		if (keys.hold(KEY_START)&&keys.hold(KEY_L))
		{//不带金手指的开始
			u8 gold = 1 ;
			WriteSram(_GoldenEnable,&gold,1);	
			WriteSram(_GoldenSaver,pCheat,256);
			if(glhard)
				return PROC_HARDRESET ;
			else
				return PROC_SOFTRESET;
		}
		if (keys.release(KEY_START))
		{//带金手指的复位
			u8 gold = 0 ;
			WriteSram(_GoldenEnable,&gold,1);	
			WriteSram(_GoldenSaver,pCheat,256);
			if(glhard)
				return PROC_HARDRESET ;
			else
				return PROC_SOFTRESET;
		}

	}//while
}

#include "savermanage.h"

void softrest(void)
{
	char name[32];
	keypad keys;
	if(!glLoadsaver)
	{//寻找存档文件
		FormatSaverName(glNorFS.filename,name,0);
		ReadFileToSram(name);
	}

	Clock_Disable();
	DisableNandbit();

	SetRompage(gl_currentpage);
	if(strcmp(CurrentDisk,"EZ-Disk"))
	{
		WriteSaverInfo(glNorFS.filename,glNorFS.saversize,0);
       	//CloseRamWrite();
       	_SetEZ2Control(0,0);
       	_SetEZ2Control(0,1);
		_SetEZ2Control(0,0);       	
		SetRampage(16);
		SetRompageWithSoftReset(glNorFS.rompage);
	}
	else
	{//nand
		glNorFS.saversize = (*(u16 *)(_Ez3PsRAM + 0xbA) & 0x00ff) ;
		WriteSaverInfo(glNorFS.filename,glNorFS.saversize,1);
       	CloseRamWrite();
       	_SetEZ2Control(0,0);
       	_SetEZ2Control(0,1);
		_SetEZ2Control(0,0);  
		SetRampage(16);     	
		if(gl_norsize == 0x3000000)
		{
			SetRompageWithHardReset(0x300);
		}
		else
		{
			SetRompageWithHardReset(0x100);
		}
	}
}

void hardrest(void)
{
	char name[32];
	if(!glLoadsaver)
	{//寻找存档文件
		FormatSaverName(glNorFS.filename,name,0);
		ReadFileToSram(name);
	}

	Clock_Disable();
	DisableNandbit(); 

	SetRompage(gl_currentpage);
	if(strcmp(CurrentDisk,"EZ-Disk"))
	{
		WriteSaverInfo(glNorFS.filename,glNorFS.saversize,0);
	   	//CloseRamWrite();
       	_SetEZ2Control(0,0);
       	_SetEZ2Control(0,1);
		_SetEZ2Control(0,0);       	
		SetRampage(16);
		SetRompageWithHardReset((glNorFS.rompage)&0xFF);
	}
	else
	{//nand
		glNorFS.saversize = (*(u16 *)(_Ez3PsRAM + 0xbA) & 0x00ff) ;
		WriteSaverInfo(glNorFS.filename,glNorFS.saversize,1);
   		CloseRamWrite();
       	_SetEZ2Control(0,0);
       	_SetEZ2Control(0,1);
		_SetEZ2Control(0,0);       	
		SetRampage(16);
		if(gl_norsize == 0x3000000)
		{
			SetRompageWithHardReset(0x300);
		}
		else
		{
			SetRompageWithHardReset(0x100);
		}
	}
}

void ShowTime()
{
	int timer =0;
	char time[32];
	CLOCK_TIME clocktime ;	
	keypad keys ;
	DrawFilePic("itemdisc.bmp",60,40,128,80,1,0x7FFF,1);	//画窗口
	DrawHZText12("Time",0,60+7,40+20, 3,1);	//粗体标题
	DrawHZText12("Time",0,60+8,40+20, 3,1);
	while(1)
	{
		timer ++ ;
		VBlankIntrWait();
		keys.update();
		if (keys.press(KEY_A)||keys.press(KEY_B))
		{
			return ;
		}
		if(!(timer%30))
		{
			GetTime_Orignal(&clocktime);
			sprintf(time,"20%1d%1d-%1d%1d-%1d%1d\n%1d%1d:%1d%1d:%1d%1d",
												(clocktime.year>>4),(clocktime.year&0xF),
												(clocktime.month>>4),(clocktime.month&0xF),
												(clocktime.data>>4),(clocktime.data&0xF),
												(clocktime.hour>>4),(clocktime.hour&0xF),
												(clocktime.minute>>4),(clocktime.minute&0xF),
												(clocktime.second>>4),(clocktime.second&0xF));
			Clear(60+54,40+16,62,40,0x7FBB,1);
			DrawHZTextRect(time,0,60+54,40+16,62,40,14,0,2,1);	//写消息
			
		}
	}
	
}

int LoadSaver()
{
	int ok ;
	u8 isopen ;
	keypad keys ;
	CreateWindow(gl_warning,gl_readsaver,60,40,1);
	while(1){
		sleep(5);	keys.update();
		if(keys.press(KEY_A))
		{
			isopen=1;
			break;
		}
		if(keys.release(KEY_B))
		{
			isopen=0;
			break;
		}
	}
	if(isopen)
	{
		ok = ReadFileToSram(glNorFS.filename) ;
		if(!ok) 
			glLoadsaver = 1 ;
	}
	return PROC_ENTERDISKSHELL ;
}

int WriteSaverEx()
{
	char buf[64];
	char name[64];
	char savername[32] ;
	keypad keys ;
	u32 ti =0,holdt=0,isopen=0;
	u16 bchange = 0 ;
	int number=0;
	SAVEFILEINFO saverinfo ;
	int get,handle;

 	SetRampage(0);
	ReadSram(_FileInfoSaver,(u8*)&saverinfo,256);
	FormatSaverName(saverinfo.gamename,savername,1);
	int size = saverinfo.saversize * 4096 ;
	sprintf(buf,"%s\\saver\\%s",gl_saverfile,savername);
	CreateWindow(" "," ",60,40,1);
	DrawHZTextRect(buf,0,60+54,40+16,62,40,14,0,2,1);	//写消息
	while(1){
		ti++;
		VBlankIntrWait();		
		sleep(5);	keys.update();
		bchange = 0 ;
		isopen=0;
		if ( (keys.press(KEY_UP))&&(holdt=ti) || (keys.hold(KEY_UP)&&(ti-holdt>15)) )//向上移动鼠标
		{
			number++ ;
			if(number>=100)
				number = 1 ;
			bchange =1 ;
		}
		if ( (keys.press(KEY_DOWN))&&(holdt=ti) || (keys.hold(KEY_DOWN)&&(ti-holdt>15)) )//向下移动鼠标
		{
			number-- ;
			if(number<=0)
				number = 99 ;
			bchange =1 ;
		}
		if(keys.press(KEY_A))
		{
			isopen=1;
		}
		if(keys.release(KEY_B))
		{
			isopen=0;
			break;
		}
		if(bchange)
		{
			FormatSaverName(saverinfo.gamename,savername,number);
			sprintf(buf,"%s\\saver\\%s",gl_saverfile,savername);
			Clear(60+54,40+16,62,40,0x7FBB,1);
			DrawHZTextRect(buf,0,60+54,40+16,62,40,14,0,2,1);	//写消息
		}
		if(isopen)
		{//写入
			get = fat_init(1);
			if(get)
			{
				CreateWindow(gl_warning,gl_faterror,60,40,1);
				while(1){
					sleep(15);	keys.update();
					if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
				}
				return PROC_ENTERDISKSHELL;
			}
			get = fat_getfirst("\\Saver",buf);
			if(get)
			{//创建一个dir
				fat_mkdir("\\Saver");
			}
			sprintf(name,"\\Saver\\%s",savername);
			sprintf(buf,"%s%s[%dKB]",gl_writing,name,size/1024);
			CreateWindow(gl_waiting,buf,60,40,1);
			
			handle = fat_creat(name,ATTR_ARCHIVE);
			if(handle<0)
				handle =fat_open(name);
			OpenWrite();
			CopybackSaver((u8*)0x8800000,size,16);
			CloseWrite();
			fat_write(handle,(char*)0x8800000,size);
			fat_close(handle);
			fat_deinit();			
			break;
		}
	}
	
	return PROC_ENTERDISKSHELL ;
}


//******************************************************************************
//**** 显示菜单函数
//**** 指定背景图片为bg，标题图片为titleBG，输入菜单字符串数组mnu，指定菜单项数量count及默认焦点defualtI
#define MENU_ITEM_SHOW_COUNT	6
int ShowWin_Menu(char *bg, char *titleBG,char *overlap, char** mnu, u8 count, u8 defaultI)
{
	u16 btnx=40,btny=46, menui=defaultI,  refresh=1, updt=1;
	u16 txtcolor;
	u32 holdt=0,ti=0;
	keypad keyy ;
	keypad *keys = &keyy ;
	u16 mnustarti=0,mnushownum=MENU_ITEM_SHOW_COUNT;
	if(count<=mnushownum)
		mnushownum=count;
	if(defaultI>count-1)
		defaultI=count-1;

	if(defaultI>MENU_ITEM_SHOW_COUNT)//确定如何显示先前曾选中的文件位置结构
	{
		if(defaultI<count-MENU_ITEM_SHOW_COUNT)
			mnustarti=defaultI;
		else
			mnustarti=count-MENU_ITEM_SHOW_COUNT;
	}
	else
		mnustarti=0;


	while(1)
	{
		sleep(30); 
   		VBlankIntrWait();                           // Complete V Blank interrupt
		keys->update(); ti++;

		if(updt)	//当菜单文本内容结构需要改变时，需要处理如下
		{
			DrawFilePic(bg,0,0,240,160,0,0,0); 
			if(titleBG[0]!=0)
				DrawFilePic(titleBG,36,0,168,42,0,0,0); 
			if(overlap[0]!=0)
				DrawFilePic(overlap,0,0,240,39,0,0,0); 			
			//打印菜单项
			for(u8 i=mnustarti;i<mnustarti+mnushownum;i++)
				DrawHZText12(mnu[i], 0,btnx+12,48+FM_ROW_H*(i-mnustarti),0,0);
			updt = 0;
			refresh = 1;
		}
		if(refresh)	//选择条位置改变时
		{
			syncVVram();
			Clear(btnx-3,btny+(menui-mnustarti)*FM_ROW_H,160+6,FM_ROW_H-2,RGB(3,20,20),1);//画阴影
			DrawHZText12(mnu[menui], 0,btnx+12,btny+(menui-mnustarti)*FM_ROW_H+2,0,1);
			//FadeStatus(0,1);
			refresh=0;
		}


		if ( (keys->press(KEY_UP)&&(holdt=ti)) || (keys->hold(KEY_UP)&&(ti-holdt>15)) )//向上移动鼠标
		{
			if(menui>0)
			{
				menui--;
				if(menui<mnustarti)
				{
					mnustarti--;
					updt=1;
				}
				refresh=1;
			}
		}

		if ( (keys->press(KEY_DOWN)&&(holdt=ti)) || (keys->hold(KEY_DOWN)&&(ti-holdt>15)) )//向下移动鼠标
		{
			if(menui<count-1)
			{
				menui++;
				if(menui>mnustarti+MENU_ITEM_SHOW_COUNT-1)
				{
					mnustarti++;
					updt=1;
				}
				refresh=1;
			}
		}

		if (keys->release(KEY_B))
		{
			return -1;
		}
		if (keys->release(KEY_A))
		{
			return menui;
		}
	}//while
}

u16 popupmenu(u16 x,u16 y)
{
	//返回值为选择的序列
	//首先画出图片来
	DrawFilePic("itemdisc.bmp",x,y,128,80,1,0x7FFF,1);	//画窗口
    DrawIcon(x+13,y+26,7,1);					//大图标
	DrawFilePic("ok.bmp",x+7,y+58,24,14,1,0x7FFF,1);	//画OK
	DrawFilePic("x.bmp",x+34,y+58,12,12,1,0x7FFF,1);	//画X
	
}

u32	execAButton()
{
	FM_NOR_FS tmpNorFS ;
	return PROC_ENTERDISKSHELL;
}

u32 FormatNor()
{
	keypad keys ;
	if(strcmp(CurrentDisk,"EZ-Disk"))
	{
		CreateWindow("",gl_norformat,60,40,1);
		while(1){
			sleep(5);	keys.update();
			if(keys.press(KEY_A))
			{
				break;
			}
			if(keys.press(KEY_B))
			{
					sprintf(CurrentDisk,"Game Disk");
					CurrentPath[0] = 0 ;
					return PROC_ENTERDISKSHELL ;
			}
		}
		OpenWrite();  
		SetSerialMode();
		Block_Erase(0);
		CloseWrite();
		sprintf(CurrentDisk,"Game Disk");
		CurrentPath[0] = 0 ;
		
	}	
	return PROC_ENTERDISKSHELL ;
}
u32  EnterWriteGBAtoNor()
{
	FM_NOR_FS tmpNorFS ;
	char path[maxNameLen*2];
	char msg[maxNameLen];
	u8* pReadCache = (u8*)_ramFatBlockCache ;		
	keypad keys ;
	u32 j=0,i=0;
	int get=0 ;
	DmaCopy(3,&glNorFS,&tmpNorFS,sizeof(FM_NOR_FS),32);
	if(!strcmp(CurrentPath,"\\"))
		sprintf(path,"%s%s",CurrentPath,tmpNorFS.filename);
	else
		sprintf(path,"%s\\%s",CurrentPath,tmpNorFS.filename);

		
	if(strcmp(CurrentDisk,"EZ-Disk"))
	{
		if(CurrentIndexSelect==gl_norgamecounter)
		{	
			CreateWindow("",g1_norDelete,60,40,1);
			while(1){
				sleep(5);	keys.update();
				if(keys.press(KEY_A))
				{
					break;
				}
				if(keys.press(KEY_B))
				{
						sprintf(CurrentDisk,"Game Disk");
						CurrentPath[0] = 0 ;
						return PROC_ENTERDISKSHELL ;
				}
			}
							
			OpenWrite();  
			SetSerialMode();
			Block_Erase(gl_norOffset-tmpNorFS.filesize);
			CloseWrite();
			sprintf(CurrentDisk,"Game Disk");
			CurrentPath[0] = 0 ;
			return PROC_ENTERDISKSHELL ;
		}
	}
	else
	{
		
			if(GetFileType(tmpNorFS.filename)!=PROC_GBA_VIEW)
			{
				CurrentFileName[0]=0;
				CurrentIndexSelect = 0 ;
				CurrentIndexView = 0 ;
				return PROC_ENTERDISKSHELL ;
			}
			OpenWrite();
			CheckEz4Card();
			CloseWrite();			
//			if(g_EZ4CardType==EZ4_1Chip)
//				return PROC_ENTERDISKSHELL;
			if( (gl_norOffset+tmpNorFS.filesize)>gl_norsize)
			{
				CreateWindow("",g1_norSpace,60,40,1);
				while(1){
					sleep(5);	keys.update();
					if(keys.release(KEY_A))
					{
						//如果选择不写回到当前目录
						CurrentFileName[0]=0;
						CurrentIndexSelect = 0 ;
						CurrentIndexView = 0 ;
						return PROC_ENTERDISKSHELL ;
					}
				}
			}			
			  
			//这里弹出提示是否写NOR Flash的
			CreateWindow("",g1_norWrite,60,40,1);
			while(1){
				sleep(5);	keys.update();
				if(keys.press(KEY_A))
				{
					break;
				}
				if(keys.press(KEY_B))
				{
					//如果选择不写回到当前目录
					CurrentFileName[0]=0;
					CurrentIndexSelect = 0 ;
					CurrentIndexView = 0 ;  
					return PROC_ENTERDISKSHELL ;
				}  
			}			
			OpenWrite();  
			SetSerialMode();
			CreateWindow("Info"," ",60,40,1);			
			for(j=gl_norOffset;j<(gl_norOffset+tmpNorFS.filesize);j+=0x40000)
			{
				chip_reset();
				sprintf(msg,"%s%dMb",g1_norErasing,(j-gl_norOffset)/0x20000);
				Clear(60+54,40+16,62,40,0x7FBB,1);
				DrawHZTextRect(msg,0,60+54,40+16,62,56,14,0,2,1);	//写消息
				Block_Erase(j);
			}  
			CloseWrite();
			int hdfile;  
			
			get = fat_init(1);
			if(get)
			{
				CreateWindow(gl_warning,gl_faterror,60,40,1);
				while(1){
					sleep(15);	keys.update();
					if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
				}
			}
			hdfile = fat_open(path);
			for(i=gl_norOffset;i<(gl_norOffset+tmpNorFS.filesize);i+=0x20000)
			{
				fat_read(hdfile,pReadCache,0x20000);				  
				OpenWrite();  
				SetSerialMode();
				sprintf(msg,"%s%dMb",g1_norWriting,(i-gl_norOffset)/0x20000);   
				Clear(60+54,40+16,62,40,0x7FBB,1);
				DrawHZTextRect(msg,0,60+54,40+16,62,56,14,0,2,1);	//写消息
				for(j=0;j<0x20000;j+=0x8000)
					WriteEZ4Flash(i+j,pReadCache+j,0x8000);
				CloseWrite();
				
			}
			fat_close(hdfile);  
			fat_deinit();  
			
			//写中文文件名
			OpenWrite();
			ldRead(pReadCache,0x10000);
			if(gl_norgamecounter==0)
				memset(pReadCache,0,0x1000);
			for(i=gl_norgamecounter*256;i<(gl_norgamecounter*256+strlen(tmpNorFS.filename));i++)
				pReadCache[i] = tmpNorFS.filename[i-gl_norgamecounter*256];
			pReadCache[(gl_norgamecounter)*256+strlen(tmpNorFS.filename)]=0;
			for(j=(gl_norgamecounter+1)*256;j<0x10000;j++)
			{
				pReadCache[j]=0;
			}
			ldWrite(_SRAMSaver,pReadCache,0x1000);
			CloseWrite();
			CurrentFileName[0]=0;
			CurrentIndexSelect = 0 ;
			CurrentIndexView = 0 ;
			gl_norOffset = gl_norOffset+tmpNorFS.filesize;
			gl_norgamecounter++;
			return PROC_ENTERDISKSHELL ;
	}
		/*blowfish add,modify data 2006-06-01,write gba or nds rom from nand to flash***********************/
			return PROC_ENTERDISKSHELL ;

}

u8 gShow=1,gx=0;
void WaitingErase()
{
	u8* pReadCache = (u8*)_ramFatBlockCache ;
	u8 ww = 20 ;
	if(gShow)
	{
		//CreateWindow(gl_tip,"",60,40,1);	
		DrawFilePic("itemdisc.bmp",60,40,128,80,1,0x7FFF,1);	//画窗口
		DrawHZText12(gl_tip,0,50+54,40+16, 3,1);	//粗体标题
		DrawHZText12(gl_tip,0,50+55,40+16, 3,1);
		gShow = 0 ;
		DmaCopy(2,0x6000000+240*2*76,_ramFatBlockCache,240*2*16,32);
	}
	//
	while(ww--) 
   		VBlankIntrWait();
		
	gx ++ ;
	if(gx>72)
		gx = 0 ;
	
	DmaCopy(2,_ramFatBlockCache,0x6000000+240*2*76,240*2*16,32);
	DrawHZTextRect(gl_waiting,0,30+54+gx,40+36,62,80,10,0,2,1);	//写消息
}