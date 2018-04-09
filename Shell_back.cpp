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
char CurrentDisk[32];	//µ±Ç°ËùÔÚ´ÅÅÌ
char CurrentPath[maxNameLen*2];		//µ±Ç°Â·¾¶
char CurrentFileName[maxNameLen]; //µ±Ç°Ñ¡ÔñÎÄ¼þÃû
u16	 CurrentIndexSelect ;		//ËùÑ¡ÎÄ¼þµÄË÷Òý
u16	 CurrentIndexView ;			//±¾Ò³ÃæÎÄ¼þ¿ªÊ¼µÄË÷Òý
FM_NOR_FS  glNorFS ;			//±£ÁôÆäÐÅÏ¢ÔÚÈí¸´Î»Ê±Ê¹ÓÃ
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
//**** ·µ»ØÒ»¸öÂ·¾¶ÖÐµÄÐ±¸ÜÊýÄ¿
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
//**** ·µ»ØÒ»¸öÂ·¾¶ÖÐµÄÎÄ¼þÃû
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
//**** ·µ»Øµ±Ç°Ä¿Â¼ÖÐROMÎÄ¼þ×ÜÊý
u16 GetDirFileCount(char *path)
{
	u8 *fsBase,*fsline;
	char fname[MAX_PATH];
	fsBase = GBAPAKLINE; //ÎÄ¼þÏµÍ³ÆðÊ¼µØÖ·
	RomFile *rf;
	u16 count=0;


	fsline=fsBase;
	rf=(RomFile*)fsline;
	while(rf->FileName[0]!=0)//Î´µ½ÎÄ¼þÄ©Î²
	{
		u16 m,n;
		rf=(RomFile*)fsline;
		fsline=fsline + HEADSIZE + rf->size + rf->padsize;
		m = getDirLayerNum(rf->FileName);
		n = getDirLayerNum(path);
		getDirFileName(rf->FileName,fname);

		if((m==1)&&(n==1))
		{
			if(fname[0]!='.')	//Èç¹ûÄ¿Â¼ÎÄ¼þÃû·Ç .* Òþ²ØÄ£Ê½
				count++;
			continue;
		}
		if((n>1)&&(m == n))
		{
			if(memcmp(rf->FileName,path,strlen(path))==0)
				if(fname[0]!='.')	//Èç¹ûÄ¿Â¼ÎÄ¼þÃû·Ç .* Òþ²ØÄ£Ê½
					count++;
			continue;
		}
	}
	return count;
}


//******************************************************************************
//**** ¸ù¾ÝÎÄ¼þË÷ÒýºÅ£¬ÔÚÖ¸¶¨DIRµÄROMÖÐ¼ìË÷Ä³ÎÄ¼þ£¬·µ»Ø¸ÃÎÄ¼þµØÖ·
u8* GetCurRomFile(char *path,u16 index,u8 isgoon)
{
	static u8* lastpRom=NULL;
	u8 *fsBase,*fsline;
	char fname[MAX_PATH];
	fsBase = GBAPAKLINE; //ÎÄ¼þÏµÍ³ÆðÊ¼µØÖ·
	RomFile *rf;
	u16 count=0;
	u8 seek1stGoon=0;	

	if((isgoon!=0)&&(lastpRom==NULL))//Á¬ÐøÄ£Ê½µÄµÚÒ»´ÎÑ­»·
	{
		seek1stGoon=1;
	}

	if(lastpRom==NULL)
        lastpRom=fsBase;

	if(isgoon==0)//·ÇÁ¬Ðø¶ÁÈ¡ÐÎÊ½
        lastpRom=fsBase;
	
	fsline=lastpRom;

	rf=(RomFile*)fsline;
	while(rf->FileName[0]!=0)//Î´µ½ÎÄ¼þÄ©Î²
	{
		u16 m,n;

		n = getDirLayerNum(path);
		m = getDirLayerNum(rf->FileName);
		getDirFileName(rf->FileName,fname);

		if((m==1)&&(n==1))
		{
			if(((isgoon==0)&&(count==index)) || (seek1stGoon==0)&&(isgoon==1) ||(seek1stGoon==1)&&(count==index) )
			{
				if(isgoon==1)//Á¬Ðø¶ÁÈ¡ÐÎÊ½
					lastpRom=fsline+HEADSIZE + rf->size + rf->padsize;
				else
					lastpRom=NULL;
				if(fname[0]!='.')	//Èç¹ûÄ¿Â¼ÎÄ¼þÃû·Ç .* Òþ²ØÄ£Ê½
					return fsline;
			}
			if(fname[0]!='.')		//Èç¹ûÄ¿Â¼ÎÄ¼þÃû·Ç .* Òþ²ØÄ£Ê½
				count++;
		}
		else if((n>1)&&(m == n))
		{

			if(memcmp(rf->FileName,path,strlen(path))==0)
			{


				if(((isgoon==0)&&(count==index)) || (seek1stGoon==0)&&(isgoon==1) ||(seek1stGoon==1)&&(count==index) )
				{
					if(isgoon==1)//Á¬Ðø¶ÁÈ¡ÐÎÊ½
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

	if(isgoon==1)//Á¬Ðø¶ÁÈ¡ÐÎÊ½
		lastpRom=fsline+HEADSIZE + rf->size + rf->padsize;
	else
		lastpRom=NULL;

	//"ÏµÍ³Ã»ÓÐÕÒµ½ÈÎºÎÎÄ¼þ!
	//dbgPrint("Î´ÕÒµ½: %s, i=%d",path,index);
	//exit(1);
	return NULL;
}


//******************************************************************************
//**** ¸ù¾ÝÎÄ¼þÃûºÍÂ·¾¶£¬È·¶¨²¢·µ»Ø¸ÃÎÄ¼þÖ¸Õë
u8* GetRomFileByName(char *path, char *Name)
{
	u8 *fsBase,*fsline;
	fsBase = GBAPAKLINE; //ÎÄ¼þÏµÍ³ÆðÊ¼µØÖ·
	RomFile *rf;
	char tmpbuf[MAX_PATH];

	fsline=fsBase;
	sprintf(tmpbuf,"%s%s",path,Name);
	rf=(RomFile*)fsline;
	
	while(rf->FileName[0]!=0)//Î´µ½ÎÄ¼þÄ©Î²
	{
		if(strcmp(tmpbuf,rf->FileName)==0)
			return fsline;
		fsline += HEADSIZE + rf->size + rf->padsize;
		rf=(RomFile*)fsline;
	}

	//dbgPrint("Î´ÕÒµ½:%s",Name);
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
//**** EZ3´ÅÅÌ¹ÜÀíÆ÷Ö÷º¯Êý ** ¹ÜÀí´ÅÅÌÄ¿Â¼ÏÂµÄÎÄ¼þ¡£
int runEz3DiskShell(void)
{
	u32  shift =0 , orgtt = 123455, dwName = 0 ;
	char msg[maxNameLen],path[maxNameLen*3];
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
//±È½Ïµ±Ç°±£´æµÄÄ¿Â¼£¬ÒÔÈ·¶¨ËÑË÷µØÖ·
	//sprintf(CurrentDisk,"EZ-Disk");
		
	if(strcmp(CurrentDisk,"EZ-Disk"))
	{		
	//Ê×ÏÈµÃµ½ÎÄ¼þÁÐ±í.
		rfcount = GetFileListFromNor() ;
		gl_norgamecounter = rfcount-1;
	//Éè¶¨Â·¾¶
		sprintf(CurrentDisk,"Game Disk");
		CurrentPath[0] = 0 ;
		if(CurrentFileName[0]==0)
		{
		//¼ÆËãÊý¾ÝÏÔÊ¾
			CurrentIndexSelect = 0 ;
			CurrentIndexView = 0;
		}
	}
	else
	{//nand flash µÄÎÄ¼þ
		FM_NOR_FS *pFile ;
		
		if(CurrentPath[0]==0) //¸ùÄ¿Â¼,»òÕß¸Õ¸Õ½øÈëNand
		{
			CurrentIndexSelect = 0 ;
			CurrentIndexView = 0;
			sprintf(CurrentPath,"\\");
		}
		get = fat_init(0); //²»ÄÜÊ¹ÓÃerternal ram
		if(get)
		{
			CreateWindow(gl_warning,gl_faterror,60,40,1);
			while(1){
				sleep(15);	keys.update();
				if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
			}
			sprintf(CurrentDisk,"Game Disk");
			CurrentPath[0] = 0 ;
			return PROC_ENTERDISKSHELL;//·µ»ØÉÏ¼¶Ä¿Â¼
		}
		count = 0 ;
		ctDir = 0 ;
		ctFile = 0;
		pFile = (FM_NOR_FS*)_UnusedEram;
		sprintf(path,"%s",CurrentPath);

		get = fat_getfirst(path,msg);
		while(!get)
		{	
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
	//ÅÅÐò£¬ÕÒµ½µÄÎÄ¼þÊý¾ÝÐèÒªÅÅÐò£¬½ö½öÎÄ¼þÅÅÐò
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
	   // ÐèÒªÕûÀíÊý¾Ý£¬ÎÄ¼þ¼Ð·ÅÔÚÇ°Ãæ£¬ÎÄ¼þ·ÅÔÚºóÃæ
        if(ctFile)
		    DmaCopy(3,(u8*)pFile,(u8*)&pNorFS[ctDir],sizeof(FM_NOR_FS)*ctFile,32);
	    rfcount = count ; //count ÖÁÉÙÎª1
		
	}
	while(1)
	{
		ti++;
        VBlankIntrWait();                           // Complete V Blank interrupt
    		shift ++ ;
		if(shift >16)
		{ //Ñ­»·ÏÔÊ¾
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

        
		//chsh³õÊ¼»¯
		count = 0 ;
		if(bcreate)//////////////////////////////////////////////////////////////
		{
			newfi = CurrentIndexSelect ;
			viewfi = CurrentIndexView ;
		//Í·²¿ÐÅÏ¢

			if(!strcmp(CurrentPath,"\\"))
				sprintf(path,"%s%s",CurrentPath,CurrentFileName);
			else
				sprintf(path,"%s\\%s",CurrentPath,CurrentFileName);
			sprintf(msg,"%s",path);
			if(strlen(msg)>29) //Ã¿ÐÐ×î¶àÏÔÊ¾29¸ö×ÖÄ¸
			{
				msg[29]=0; msg[28]='.'; msg[27]='.'; msg[26]='.';
			}
			//Çå¿Õ½á¹¹ÌåÊý×é
			for(i=0;i<FM_MSN;i++)
			{
				fmf.sectnum=0;
				strcpy(fmf.sect[i].filename,"\0");
				fmf.sect[i].isDir=0;
				fmf.sect[i].filesize=0;
			}

			//ÖðÌõÉèÖÃÎÄ¼þÐÅÏ¢
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
			//¼ÆËãÀ­ÌõÊý¾Ý
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
		if(bchgpage)///////////////////»»Ò³//////////////////////////////////////
		{
			//Ë¢ÐÂä¯ÀÀÆ÷Í·
			DrawPic(res.res_FILEBGHEAD,0,0,240,FM_TITLE_H,0,0,0);
			//DrawHZText12(msg,0, 60,3, FM_CL_BG,0);
			//»­³ö±³¾°Í¼°¸
			char FileStr[128];
			u8 num=(fmf.sectnum>FM_MSN)?FM_MSN:fmf.sectnum;
			for(u8 i=0;i<FM_MSN;i++)
			{
				DrawPic((u16*)res.res_FILEBG+((viewfi+i)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+(i)*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			//DrawPic(res.res_FILEBG,0,FM_TITLE_H,240,(FM_ROW_H<<3),0,0,0);
			//»­icon,ÒÔ¼°ÎÄ¼þ´óÐ¡ºÍÎÄ¼þÃû
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
			//Ë¢ÐÂÑ¡ÖÐÎÄ¼þ

			oldFi = fi-viewfi;
			newFi = newfi - viewfi;

			////¸ßÁÁÏÔÊ¾Ñ¡ÖÐÎÄ¼þ
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
			//Ë¢ÐÂÎÄ¼þÇø
			char FileStr[128];
			u16 count = 0;
			oldFi = fi-viewfi;
			newFi = newfi - viewfi;
			if(newFi == 0)//ÏòÉÏÑ¡Ôñ£¬ÆÁÄ»ÏÂ¹ö
			{
				for(u8 ii=FM_MSN-1;ii>0;ii--)
				{
					moveScreen(FM_TITLE_H + FM_ROW_H*(ii-1),FM_TITLE_H + FM_ROW_H*(ii),FM_ROW_H,0);
				}
				DrawPic((u16*)res.res_FILEBG+((viewfi)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+newFi*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			else			//ÏòÏÂÑ¡Ôñ£¬ÆÁÄ»ÉÏ¹ö
			{
				for(u8 ii=1;ii<FM_MSN;ii++)
				{
					moveScreen(FM_TITLE_H + FM_ROW_H*(ii),FM_TITLE_H + FM_ROW_H*(ii-1),FM_ROW_H,0);
				}
				DrawPic((u16*)res.res_FILEBG+((fi+1)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+newFi*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			//ÖðÌõÉèÖÃÎÄ¼þÐÅÏ¢
			for(u8 i=0;i<fmf.sectnum;i++)
			{
				if(i+viewfi>rfcount-1)
					continue;
				DmaCopy(3,&pNorFS[i+viewfi],&tmpNorFS,sizeof(FM_NOR_FS),32);
				sprintf(fmf.sect[i%FM_MSN].filename,"%s",tmpNorFS.filename);
				fmf.sect[i%FM_MSN].filesize=tmpNorFS.filesize ;
				fmf.sect[i%FM_MSN].isDir = 0;
			}

			//È¡Ïû¸ßÁÁÎÄ¼þ
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

			//¸ßÁÁÏÔÊ¾Ñ¡ÖÐÎÄ¼þ
			//w = strlen(fmf.sect[newFi].filename)*6+2;
			//Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,w,FM_ROW_H-2,RGB(3,20,20),0);//»­ÒõÓ°
			Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,220-FM_F_NAME_X,FM_ROW_H-2,RGB(3,20,20),0);
			if(fmf.sect[newFi].isDir)
				DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,PROC_DIR_VIEW,0);
			else
				DrawFileIcon(3, FM_TITLE_H+2+newFi*FM_ROW_H,GetFileType(fmf.sect[newFi].filename),0);

			DrawHZText12(fmf.sect[newFi].filename,FM_F_NAME_W/6+8, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);//Ð´ÐÂ±³¾°
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
			//Ë¢ÐÂÑ¡ÖÐÎÄ¼þ
			char FileStr[128];

			oldFi = fi-viewfi;
			newFi = newfi - viewfi;
			////È¡Ïû¸ßÁÁÎÄ¼þ
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

			////¸ßÁÁÏÔÊ¾Ñ¡ÖÐÎÄ¼þ
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
			FadeInOut(1,0,50);//ÏûÎí
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
			FileIndexOfPath[layerIndex]=newfi;	//¼ÍÂ¼ÎÄ¼þ½¹µã
			ViewIndexOfPath[layerIndex]=viewfi;	
			ScreenSaver("ezpdalogo.bmp");
			shellflag=SHELL_FLAG_SCR;
			bcreate = 1;
			needfade = 1;
			*/
		}
		if ( (keys.press(KEY_UP))&&(holdt=ti) || (keys.hold(KEY_UP)&&(ti-holdt>15)) )//ÏòÉÏÒÆ¶¯Êó±ê
		{
			shift =0  ;
			if(fi>0)
			{
				newfi--;
				if(viewfi-newfi>0)
				{
					//·­ÆÁÉÏÒÆÒ»¸öÎÄ¼þ
					viewfi--;
					bscroll = 1;
				}
				else
				{
					//±¾ÆÁÉÏÒÆÒ»¸öÎÄ¼þ
					bchg = 1;
				}
			}
			CurrentIndexSelect = newfi ;
			CurrentIndexView = viewfi ;
		}
		if ( (keys.press(KEY_DOWN))&&(holdt=ti) || (keys.hold(KEY_DOWN)&&(ti-holdt>15)) )//ÏòÏÂÒÆ¶¯Êó±ê
		{
			shift =0  ;
			if(fi<rfcount-1)
			{
				newfi++;
				if(newfi-viewfi>FM_MSN-1)
				{
					//·­ÆÁÏÂÒÆÒ»¸öÎÄ¼þ
					viewfi++;
					bscroll = 1;
				}
				else
				{
					//±¾ÆÁÏÂÒÆÒ»¸öÎÄ¼þ
					bchg = 1;
				}
			}
			CurrentIndexSelect = newfi ;
			CurrentIndexView = viewfi ;
		}
		if (keys.press(KEY_LEFT)&&(holdt=ti) )
		{//ÉÏÒ»Ò³
			shift =0  ;
			if(fi>0)
			{
				newfi -= FM_MSN;
				if(newfi <0)	newfi = 0 ;
				if(viewfi-newfi>0)
				{
					//·­ÆÁÉÏÒÆÒ»¸öÎÄ¼þ
					viewfi -= FM_MSN;
					if(viewfi <0) viewfi = 0 ;
					bcreate = 1;
				}
				else
				{
					//±¾ÆÁÉÏÒÆÒ»¸öÎÄ¼þ
					bcreate = 1;
				}
				CurrentIndexSelect = newfi ;
				CurrentIndexView = viewfi ;
			}
		}
		if (keys.press(KEY_RIGHT)&&(holdt=ti) )
		{//ÏÂÒ»Ò³
			shift =0  ;
			if(fi<rfcount-1)
			{
				newfi+= FM_MSN;
				if(newfi>rfcount-1) newfi = rfcount-1 ;
				if(newfi-viewfi>FM_MSN-1)
				{
					//·­ÆÁÏÂÒÆÒ»¸öÎÄ¼þ
					viewfi+= FM_MSN;
					if(viewfi>rfcount -FM_MSN) viewfi = rfcount -FM_MSN ;
					bcreate = 1 ;
				}
				else
				{
					//±¾ÆÁÏÂÒÆÒ»¸öÎÄ¼þ
					bcreate = 1;
				}
				CurrentIndexSelect = newfi ; 
				CurrentIndexView = viewfi ;
			}
		}
		
		if(keys.press(KEY_SELECT)&&(holdt=ti) )
		{
			/*blowfish add,modify data 2006-06-01,write gba or nds rom from nand to flash**********************/ 	
			DmaCopy(3,&pNorFS[CurrentIndexSelect],&tmpNorFS,sizeof(FM_NOR_FS),32);
			if(!strcmp(CurrentPath,"\\"))
				sprintf(path,"%s%s",CurrentPath,tmpNorFS.filename);
			else
				sprintf(path,"%s\\%s",CurrentPath,tmpNorFS.filename);

			
			if(strcmp(CurrentDisk,"EZ-Disk"))
			{
				if(CurrentIndexSelect==gl_norgamecounter)
				{	
				
					CreateWindow("","Delete",60,40,1);
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
				if( (gl_norOffset+tmpNorFS.filesize)>gl_norsize)
				{
					CreateWindow("Info","Empty is not enough",60,40,1);
					while(1){
						sleep(5);	keys.update();
						if(keys.release(KEY_A))
						{
							//Èç¹ûÑ¡Ôñ²»Ð´»Øµ½µ±Ç°Ä¿Â¼
							CurrentFileName[0]=0;
							CurrentIndexSelect = 0 ;
							CurrentIndexView = 0 ;
							return PROC_ENTERDISKSHELL ;
						}
					}
				}			
				
				//ÕâÀïµ¯³öÌáÊ¾ÊÇ·ñÐ´NOR FlashµÄ
				CreateWindow("","Write NOR Y/N?",60,40,1);
				while(1){
					sleep(5);	keys.update();
					if(keys.press(KEY_A))
					{
						break;
					}
					if(keys.press(KEY_B))
					{
						//Èç¹ûÑ¡Ôñ²»Ð´»Øµ½µ±Ç°Ä¿Â¼
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
					sprintf(msg,"Erasing%dMb",(j-gl_norOffset)/0x20000);
					Clear(60+54,40+16,62,40,0x7FBB,1);
					DrawHZTextRect(msg,0,60+54,40+16,62,56,14,0,2,1);	//Ð´ÏûÏ¢
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
					continue ;
				}
				hdfile = fat_open(path);
				for(i=gl_norOffset;i<(gl_norOffset+tmpNorFS.filesize);i+=0x20000)
				{
					fat_read(hdfile,pReadCache,0x20000);				  
					OpenWrite();  
					SetSerialMode();
					sprintf(msg,"Writing ´%dMb",(i-gl_norOffset)/0x20000);  
					Clear(60+54,40+16,62,40,0x7FBB,1);
					DrawHZTextRect(msg,0,60+54,40+16,62,56,14,0,2,1);	//Ð´ÏûÏ¢
					for(j=0;j<0x20000;j+=0x8000)
						WriteEZ4Flash(i+j,pReadCache+j,0x8000);
					CloseWrite();
					
				}
				fat_close(hdfile);  
				fat_deinit();  
				
				//Ð´ÖÐÎÄÎÄ¼þÃû
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
				return PROC_ENTERDISKSHELL ;
			}
			/*blowfish add,modify data 2006-06-01,write gba or nds rom from nand to flash***********************/
		}
		if ((keys.hold(KEY_A))&&(keys.hold(KEY_L)))
		{//Ó²¸´Î»
			shift =0  ;
			glhard = 1 ;
		}
		if (keys.release(KEY_A)||glhard||(glExecFileTxt==EXEC_TXT))
		{
		
			shift =0  ;
			DmaCopy(3,&pNorFS[CurrentIndexSelect],&tmpNorFS,sizeof(FM_NOR_FS),32);
			if(strcmp(CurrentDisk,"Game Disk"))
			{//²»Ò»Ñù £¬ ÔÚnandÖÐ
				if(tmpNorFS.savertype == 0x5AA5)
				{//dir
					if(!strcmp(tmpNorFS.filename,"."))
					{
						continue ;
					}
					if(!strcmp(tmpNorFS.filename,".."))
					{
						if(strcmp(CurrentDisk,"Game Disk"))
						{//²»Ò»Ñù £¬ ÔÚnand diskÖÐ ,
						
							if(strcmp(CurrentPath,"\\"))
							{//»Øµ½ÉÏ¼¶Ä¿Â¼
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
				{//ÎÄ¼þ²Ù×÷
					u8 isopen , isset ;
					int hdfile,first ;
					u8* pReadCache = (u8*)_ramFatBlockCache ;
					u8* prom = (u8*)_Ez3PsRAM ;
					u32 head[2],pos[256],id , j;
					sprintf(CurrentFileName,tmpNorFS.filename);
					DmaCopy(3,&tmpNorFS,&glNorFS,sizeof(FM_NOR_FS),32);
					u32 prc=GetFileType(tmpNorFS.filename);
					//×¼±¸¶ÁÎÄ¼þ²¿·Ö
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
					case PROC_UNKNOWN: 					//Ö´ÐÐ - Î´Öª
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
					case PROC_TXT_VIEW:					//Ö´ÐÐ - ÎÄ¼þÔÄ¶ÁÆ÷
					case PROC_HTM_VIEW:					//Ö´ÐÐ - ÎÄ¼þÔÄ¶ÁÆ÷
					case PROC_C_VIEW:					//Ö´ÐÐ - ÎÄ¼þÔÄ¶ÁÆ÷
					case PROC_H_VIEW:					//Ö´ÐÐ - ÎÄ¼þÔÄ¶ÁÆ÷
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

					//ÐèÒªÏÈ´ò¿ªÎÄ¼þÈ»ºó¶Áµ½psramÖÐ£¬ÏÔÊ¾
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
					
					case PROC_JPG_VIEW: 				//Ö´ÐÐ - JPGÔÄ¶ÁÆ÷
					//ÐèÒªÏÈ´ò¿ªÎÄ¼þÈ»ºó¶Áµ½psramÖÐ£¬ÏÔÊ¾
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
						
					case PROC_ZIPGBA_VIEW:				//Ö´ÐÐzip½âÑ¹¡£
						//¸ù¾ÝÎÄ¼þÀàÐÍ½øÐÐ²Ù×÷¡£
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
							fat_read(hdfile,msg ,first);
						first = head[1]*0x20000 ;
						
						if(first>0x1000000)
						{//Ð´NOR
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
							{//¿ªÊ¼²Á³ý
							
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
									DrawHZTextRect(msg,0,60+54,40+16,62,40,14,0,2,1);	//Ð´ÏûÏ¢
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
								DrawHZTextRect(msg,0,60+54,40+16,62,40,14,0,2,1);	//Ð´ÏûÏ¢
							}
							StopProgress();	
							return PROC_GOLDENSELECT ;
						}
					case PROC_GBA_VIEW:				//Ö´ÐÐ¿½±´¹¤×÷¡£
						glTotalsize = tmpNorFS.filesize ;
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
							{//¿ªÊ¼²Á³ý
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
									DrawHZTextRect(msg,0,60+54,40+16,62,40,14,0,2,1);	//Ð´ÏûÏ¢
								}
								StopProgress();
								OpenWrite();
								chip_reset();
								memset(pReadCache,0,256);
								ldWrite(_Ez3NorCNName,pReadCache,256);

								CloseWrite();
								sprintf(CurrentDisk,"Game Disk");
								return PROC_GOLDENSELECT ;
							//¿ªÊ¼Ð´ÈëÓÎÏ·
							}
							else
							{
								bchgpage = 1;
							}
						}
						else
						{
							*(u8*)0x0E00FFFF = 0x5A ; //Ð´
							StartProgress();
							
							for(i=0;i<tmpNorFS.filesize;i+=0x20000)
							{
								
								fat_read(hdfile,pReadCache,0x20000);
								
								OpenWrite();
								DmaCopy(3,pReadCache,prom+i,0x20000,32);
								CloseWrite();
								glCursize += 0x20000 ;
								sprintf(msg,"%s  %d%%",gl_showprog,glCursize*100/glTotalsize);
								Clear(60+54,40+16,62,40,0x7FBB,1);
								DrawHZTextRect(msg,0,60+54,40+16,62,40,14,0,2,1);	//Ð´ÏûÏ¢
							}
							StopProgress();
							*(u8*)0x0E00FFFF = 0xA5 ; //¶Á

						}						
						/*²âÊÔÎÄ¼þÊÇ·ñÕýÈ·*/
						/*
						fat_lseek(hdfile,0,SEEK_SET);
						for(i=0;i<tmpNorFS.filesize;i+=0x20000)
						{
							fat_read(hdfile,pReadCache,0x20000);
							for(j=0;j<0x20000;j+=2)
							{
								if(*(u16*)(pReadCache+j)!=*(u16*)(prom+i+j))
									break ;
							}
							if(j<20000)
							{
								
								sprintf(msg,"[%x]%x VS %x",i+j,*(u16*)(pReadCache+j),*(u16*)(prom+i+j));
								CreateWindow("´íÎó",msg,60,40,1);			
								while(1){
									sleep(15);	keys.update();
									if(keys.press(KEY_A)||keys.release(KEY_B))	break;				
								}
								break;
							}
						
						}
						*/
						//¸ù¾ÝÎÄ¼þÀàÐÍ½øÐÐ²Ù×÷¡£
						return PROC_GOLDENSELECT ;
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
			{//Ò»Ñù, ÔÚNorFlashÖÐ
				if(newfi == 0)
				{ //½øÈë×ÓÄ¿Â¼ Nand disk
					sprintf(CurrentDisk,"EZ-Disk");
					CurrentPath[0] = 0 ;
					CurrentIndexSelect = 0 ;
					CurrentIndexView = 0 ;
					return PROC_ENTERDISKSHELL ;
				}
				else
				{//Ó²¼þ¸´Î»½øÈëÓÎÏ·
					//ÐèÒªÌØ±ðµÄ²ÎÊý£¬µÚ¼¸¸öÓÎÏ·¡£										
					sprintf(CurrentFileName,tmpNorFS.filename);
					DmaCopy(3,&tmpNorFS,&glNorFS,sizeof(FM_NOR_FS),32);
					return PROC_GOLDENSELECT ;
				}
			}
		}
		if (keys.release(KEY_SELECT))
		{
			u16 xs,ys ;
			shift =0  ;
			//Ê×ÏÈ¼ì²â
			if(strcmp(CurrentDisk,"Game Disk"))
			{//²»Ò»Ñù £¬ ÔÚnand diskÖÐ ,
				if(tmpNorFS.savertype != 0x5AA5)
				{//·Çdir
					ys = FM_TITLE_H+2+newFi*FM_ROW_H ;
					//xs = 
				}
			}
			else
			{
				if(newfi != 0)
				{ //µÚÒ»¸öÎª×ÓÄ¿Â¼ÂÓ¹ý²»´¦Àí
				}
			}
		}
		if (keys.release(KEY_B))
		{
			shift =0  ;
			if(strcmp(CurrentDisk,"Game Disk"))
			{//²»Ò»Ñù £¬ ÔÚnand diskÖÐ ,
			
				if(strcmp(CurrentPath,"\\"))
				{//»Øµ½ÉÏ¼¶Ä¿Â¼
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
			{//Ó¦¸Ã»Øµ½×ÀÃæ
				//dbgPrint("OK, RETURN!");
				//sleep(1000);
				FadeInOut(0,0,50);
				return PROC_DESKTOP;
			}
			
		}
		if(keys.release(KEY_R))
		{//ÏÔÊ¾Ê±¼ä
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
//**** EZ3´ÅÅÌ¹ÜÀíÆ÷Ö÷º¯Êý ** ¹ÜÀí´ÅÅÌÄ¿Â¼ÏÂµÄÎÄ¼þ¡£
//µÃµ½NorFlashÎÄ¼þ²¢Ð´ÈëÒ»¸ö»º´æÖÐ.
int  GetFileListFromNor()
{
	 *(vu16 *)REG_IME   = 0 ;       
	int page=0 ,count=0,i=0;
	unsigned int StartAddress = _Ez3NorRom;
	FM_NOR_FS *pNorFS = (FM_NOR_FS *)_UnusedVram ;
	FM_NOR_FS tmpNorFS ;
	SetRompage(gl_currentpage) ;
	//char tp[13];
	char temp[256];
	vu16  Value;
	//Éè¶¨µÚÒ»¸öÎªNand diskÄ¿Â¼
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
//	CloseWrite();
//	SetSerialMode();
//	OpenWrite();
//	chip_reset();
//	CloseWrite();
//	sprintf(Msg,"Value=0x%x",*(u16 *)(StartAddress + 0xbe));
//	DrawHZText12(Msg,0,10,70,FM_CL_WHITE,1);
	Value = *(vu16 *)(StartAddress + 0xbe);
	//while( ((*(u16 *)(StartAddress + 0xbe) & 0xff) == 0xCE) || ((*(u16 *)(StartAddress + 0xbe) & 0xff) == 0xCF)|| (*(u16 *)(StartAddress + 0xbe) & 0xff) == 0x00 ))
	while( ((Value&0xFF)==0xCE) || ((Value&0xFF)==0xCF)|| ((Value&0xFF)==0x00))
	{
		
		if(*(vu8 *)(StartAddress+0xb2) == 0x96)
		{
			memcpy(temp,(char*)(StartAddress+0xa0),12);
			temp[12] = 0 ;
			
			if(*(vu16*)(pReadCache+256*(count-1)) == 0)
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
			gl_norOffset+=tmpNorFS.filesize;
			tmpNorFS.savertype = *(vu8 *)(StartAddress+0xbc) ;
			DmaCopy(3,&tmpNorFS,&pNorFS[count],sizeof(FM_NOR_FS),32);
			count ++ ;
		}
		else
		{
			break;
		}
		
		if((*(vu16 *)(StartAddress + 0xb6+2)==0)) //ÓÎÏ·´óÐ¡Îª0
			break;
			

		StartAddress += (*(vu16 *)(StartAddress + 0xb6+2) << 15);
		
		while(StartAddress>=_Ez3NorRomEnd)
		{
			page += 0x1000 ;
			if(page>0x3000) return count;
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
//**** EZ3½ø¶ÈÌõµÄ¹ÜÀí³ÌÐò
void StartProgress()
{
//¿ªÊ¼timer1ÖÐ¶Ï¼ÆÊ±£¬´ó¸ÅÃ¿ÃëÖÓÒ»´Î£¬ÏÈ»­³öframe
	glCursize = 0;
	DrawFilePic("itemdisc.bmp",60,40,128,80,1,0x7FFF,1);	//»­´°¿Ú
	DrawHZText12("½ø¶È",0,60+7,40+20, 3,1);	//´ÖÌå±êÌâ
	DrawHZText12("½ø¶È",0,60+8,40+20, 3,1);
//	DrawHZTextRect("µ±Ç°½ø¶È",0,60+54,40+30,62,40,14,0,2,1);	//Ð´ÏûÏ¢
//	*(vu32*)REG_TM1CNT = TMR_ENABLE|TMR_IF_ENABLE|TMR_PRESCALER_256CK|0x100;
	
}

void StopProgress()
{
//½áÊøÖÐ¶Ï¼ÆÊ±,Ö÷³ÌÐòÐèÒª´¦ÀíÖØÐÂÏÔÊ¾ÎÊÌâ
//	*(vu32*)REG_TM1CNT = 0;
}

typedef struct
{
	char FileName[16];
} CHEATST;

u8   pCheat[256]; //ÎªÁË±£´æ½á¹û
//******************************************************************************
//**** EZ3½ðÊÖÖ¸ÏîÄ¿Ñ¡Ôñº¯Êý¡£
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
 	u8* pPos ,*pbPos; //µ±Ç°ÏÔÊ¾ÔÚÄÇÀï
 	u32 filestart,skip;
	FM_MD_FILE fmf;
	register int i ;
	u16 count=0;
	u16 oldFi,newFi;
	u32 ti = 0 , holdt = 0 , page , j;
	u32 cheatsize = 0;
	keypad keys;
	pVBack = (CHEATST *)_UnusedVram ;
//±È½Ïµ±Ç°±£´æµÄÄ¿Â¼£¬ÒÔÈ·¶¨ËÑË÷µØÖ·
	//Í³¼ÆËùÓÐµÄÑ¡Ïî
	if(strcmp(CurrentDisk,"EZ-Disk"))
	{//norflash ÖÐ
 		pPsram = (u16*)_Ez3PsRAM ; //psram¿ªÊ¼µÄµØ·½
		page = glNorFS.rompage ;
		page = (page>>6)<<12 ;
		SetRompage(gl_currentpage+page); //ÓÎÏ·¿ªÊ¼µÄµØ·½¾ÍÊÇ
		skip  = (glNorFS.rompage  - (page>>6))<<17 ; //ÓÎÏ·¿ªÊ¼µÄÆ«ÒÆµØÖ·
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
			SetRompage(gl_currentpage+page+((skip+filestart)>>23<<12)); //ÓÎÏ·¿ªÊ¼µÄµØ·½¾ÍÊÇ

			i = (filestart + skip )%0x800000 ;
			if((i +0x10000)>0x800000) // ÓÎÏ·Êý¾Ý.
			{
				j=(i +0x10000) - 0x800000 ;
				OpenWrite();
				OpenRamWrite();
				DmaCopy(3,pPos+i,0x2020000,0x10000-j,32);
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				DmaCopy(3,0x2020000,pPsram,0x10000-j,32);
				CloseWrite();
				SetRompage(gl_currentpage+page+((skip+filestart)>>23<<12)+0x1000); //ÓÎÏ·¿ªÊ¼µÄµØ·½¾ÍÊÇ

				OpenWrite();
				OpenRamWrite();
				DmaCopy(3,pPos,0x2020000,j,32);
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				DmaCopy(3,0x2020000,((u8*)pPsram)+0x10000-j,j,32);
				CloseWrite();
				
			}
			else //Ö±½Ó¿½±´  
			{
				OpenWrite();
				OpenRamWrite();
				DmaCopy(3,pPos+i,0x2020000,0x10000,32)
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				DmaCopy(3,0x2020000,pPsram,0x10000,32)
//				u16* pReadCache = (u16*)_ramFatBlockCache ;		
//				for(i=0;i<0x8000;i++)
//					pPsram[i] = pReadCache[i];
				CloseWrite();
			}
		}
		else
		{//¿½±´Êý¾Ýµ½PSramÖÐ£¬´óÐ¡0x10000,´ÓXCODE¿ªÊ¼
			if((filestart+skip +0x10000)>0x800000) // ÓÎÏ·Êý¾Ý.
			{
				i=(filestart+skip +0x10000) - 0x800000 ;
				OpenWrite();
				OpenRamWrite();
				DmaCopy(3,pPos+filestart+skip,0x2020000,0x10000-i,32)
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				DmaCopy(3,0x2020000,pPsram,0x10000-i,32)
				CloseWrite();
				SetRompage(gl_currentpage+page+0x1000); //ÓÎÏ·¿ªÊ¼µÄµØ·½¾ÍÊÇ
				
				OpenWrite();
				OpenRamWrite();
				DmaCopy(3,pPos,0x2020000,i,32)
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				DmaCopy(3,0x2020000,(u8*)pPsram+0x10000-i,i,32)
				CloseWrite();
			}  
			else //Ö±½Ó¿½±´
			{
				OpenWrite();
				OpenRamWrite();
//				*(u8*)0x0E00FFFF = 0x5A ; //Ð´
				DmaCopy(3,pPos+filestart+skip,0x2020000,0x10000,32)
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				VBlankIntrWait();
				sleep(5);
				DmaCopy(3,0x2020000,0x08400000,0x10000,32)
//				u16* pReadCache = (u16*)_ramFatBlockCache ;		
//				for(i=0;i<0x8000;i++)
//					pPsram[i] = pReadCache[i];
				CloseWrite();
			}
		}
		
		pPos = (u8*)pPsram ;
		SetRompage(gl_currentpage); //
	}
	else
	{//nand flash µÄÎÄ¼þ
 		pPsram = (u16*)_Ez3PsRAM ; //psram¿ªÊ¼µÄµØ·½
		pPos = (u8*)_Ez3PsRAM ;
		//¼ÆËãxcodeËùÔÚµÄÎ»ÖÃ£®
		filestart = (pPos[0xB5]) + (pPos[0xb6]<<8) + (pPos[0xb7]<<16) ;
		filestart = filestart<<4 ;
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
	//×¼±¸Êý¾Ý
	rfcount = (*((u16*)&pPos[22])) ;
	//Ê×ÏÈÕÒµ½ÆðÊ¼µØÖ·
	skip = 24+8*(*((u16*)&pPos[20])); // XCODE ´óÐ¡¡£
	
	cheatsize = 0x1E0 ;
	if(memcmp(cheatcode,&pPos[skip],16))
		cheatsize = 0x6C0 ;
	
	skip += cheatsize*(*((u16*)&pPos[20])) ; //Èë¿Ú³ÌÐòµÄ´óÐ¡
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
		//chsh³õÊ¼»¯
		count = 0 ;
		if(bcreate)//////////////////////////////////////////////////////////////
		{
		//Í·²¿ÐÅÏ¢
			sprintf(msg,"%s\\%s",CurrentDisk,CurrentFileName);
			if(strlen(msg)>29) //Ã¿ÐÐ×î¶àÏÔÊ¾29¸ö×ÖÄ¸
			{
				msg[29]=0; msg[28]='.'; msg[27]='.'; msg[26]='.';
			}
			//Çå¿Õ½á¹¹ÌåÊý×é
			for(u8 i=0;i<FM_MSN;i++)
			{
				fmf.sectnum=0;
				strcpy(fmf.sect[i].filename,"\0");
				fmf.sect[i].isDir=0;
				fmf.sect[i].filesize=0;
			}

			//ÖðÌõÉèÖÃÎÄ¼þÐÅÏ¢
			for(i=viewfi;i<viewfi+FM_MSN;i++)
			{
				if(i>rfcount-1)
					break;
				DmaCopy(3,&pVBack[startpos[i]],bk,16,32);
				sprintf(fmf.sect[i-viewfi].filename,"%s",bk);
				count++;
			}
			fmf.sectnum=count;
			//¼ÆËãÀ­ÌõÊý¾Ý
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
		if(bchgpage)///////////////////»»Ò³//////////////////////////////////////
		{
			//Ë¢ÐÂä¯ÀÀÆ÷Í·
			DrawPic(res.res_FILEBGHEAD,0,0,240,FM_TITLE_H,0,0,0);
			DrawHZText12(msg,0, 60,3, FM_CL_BG,0);
			//»­³ö±³¾°Í¼°¸
			char FileStr[128];
			u8 num=(fmf.sectnum>FM_MSN)?FM_MSN:fmf.sectnum;
			for(u8 i=0;i<FM_MSN;i++)
			{
				DrawPic((u16*)res.res_FILEBG+((viewfi+i)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+(i)*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			//DrawPic(res.res_FILEBG,0,FM_TITLE_H,240,(FM_ROW_H<<3),0,0,0);
			//»­icon,ÒÔ¼°ÎÄ¼þ´óÐ¡ºÍÎÄ¼þÃû
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
			//Ë¢ÐÂÑ¡ÖÐÎÄ¼þ

			////¸ßÁÁÏÔÊ¾Ñ¡ÖÐÎÄ¼þ
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
			//Ë¢ÐÂÎÄ¼þÇø
			char FileStr[128];
			u16 count = 0;
			oldFi = fi-viewfi;
			newFi = newfi - viewfi;
			if(newFi == 0)//ÏòÉÏÑ¡Ôñ£¬ÆÁÄ»ÏÂ¹ö
			{
				for(u8 ii=FM_MSN-1;ii>0;ii--)
				{
					moveScreen(FM_TITLE_H + FM_ROW_H*(ii-1),FM_TITLE_H + FM_ROW_H*(ii),FM_ROW_H,0);
				}
				DrawPic((u16*)res.res_FILEBG+((viewfi)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+newFi*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			else			//ÏòÏÂÑ¡Ôñ£¬ÆÁÄ»ÉÏ¹ö
			{
				for(u8 ii=1;ii<FM_MSN;ii++)
				{
					moveScreen(FM_TITLE_H + FM_ROW_H*(ii),FM_TITLE_H + FM_ROW_H*(ii-1),FM_ROW_H,0);
				}
				DrawPic((u16*)res.res_FILEBG+((fi+1)%FM_MSN)*FM_ROW_H*240,0,FM_TITLE_H+newFi*FM_ROW_H,240,FM_ROW_H,0,0,0);
			}
			//ÖðÌõÉèÖÃÎÄ¼þÐÅÏ¢
			for(u8 i=0;i<fmf.sectnum;i++)
			{
				if(i+viewfi>rfcount-1)
					continue;
				DmaCopy(3,&pVBack[startpos[i+viewfi]],bk,16,32);
				sprintf(fmf.sect[i%FM_MSN].filename,"%s",bk);
			}

			//È¡Ïû¸ßÁÁÎÄ¼þ
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

			//¸ßÁÁÏÔÊ¾Ñ¡ÖÐÎÄ¼þ
			w = strlen(fmf.sect[newFi].filename)*6+2;
			Clear(FM_F_NAME_X,FM_TITLE_H+1+newFi*FM_ROW_H,w,FM_ROW_H-2,RGB(3,20,20),0);//»­ÒõÓ°

			DrawHZText12(fmf.sect[newFi].filename,FM_F_NAME_W/6, FM_F_NAME_X,FM_TITLE_H+2+newFi*FM_ROW_H, FM_CL_BG,0);//Ð´ÐÂ±³¾°
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
			//Ë¢ÐÂÑ¡ÖÐÎÄ¼þ
			char FileStr[128];
			u16 w ;
			oldFi = fi-viewfi;
			newFi = newfi - viewfi;
			////È¡Ïû¸ßÁÁÎÄ¼þ
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
			////¸ßÁÁÏÔÊ¾Ñ¡ÖÐÎÄ¼þ
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
			FadeInOut(1,0,50);//ÏûÎí
			needfade=0;
		}

		//--------------------------------------------------------------------------------
		//--------------------------------------------------------------------------------
		//--------------------------------------------------------------------------------
		keys.update();
		if ( (keys.press(KEY_UP))&&(holdt=ti) || (keys.hold(KEY_UP)&&(ti-holdt>15)) )//ÏòÉÏÒÆ¶¯Êó±ê
		{
			if(fi>0)
			{
				newfi--;
				if(viewfi-newfi>0)
				{
					//·­ÆÁÉÏÒÆÒ»¸öÎÄ¼þ
					viewfi--;
					bscroll = 1;
				}
				else
				{
					//±¾ÆÁÉÏÒÆÒ»¸öÎÄ¼þ
					bchg = 1;
				}
			}
		}
		if ( (keys.press(KEY_DOWN))&&(holdt=ti) || (keys.hold(KEY_DOWN)&&(ti-holdt>15)) )//ÏòÏÂÒÆ¶¯Êó±ê
		{
			if(fi<rfcount-1)
			{
				newfi++;
				if(newfi-viewfi>FM_MSN-1)
				{
					//·­ÆÁÏÂÒÆÒ»¸öÎÄ¼þ
					viewfi++;
					bscroll = 1;
				}
				else
				{
					//±¾ÆÁÏÂÒÆÒ»¸öÎÄ¼þ
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
			return PROC_ENTERDISKSHELL ; //·µ»Ø
		}
		if (keys.hold(KEY_START)&&keys.hold(KEY_L))
		{//²»´ø½ðÊÖÖ¸µÄ¿ªÊ¼
			u8 gold = 1 ;
			WriteSram(_GoldenEnable,&gold,1);	
			WriteSram(_GoldenSaver,pCheat,256);
			if(glhard)
				return PROC_HARDRESET ;
			else
				return PROC_SOFTRESET;
		}
		if (keys.release(KEY_START))
		{//´ø½ðÊÖÖ¸µÄ¸´Î»
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
	{//Ñ°ÕÒ´æµµÎÄ¼þ
		FormatSaverName(glNorFS.filename,name,0);
		ReadFileToSram(name);
	}

	Clock_Disable();
	DisableNandbit();

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
		SetRompageWithSoftReset(0x100);
	}
}

void hardrest(void)
{
	char name[32];
	if(!glLoadsaver)
	{//Ñ°ÕÒ´æµµÎÄ¼þ
		FormatSaverName(glNorFS.filename,name,0);
		ReadFileToSram(name);
	}

	Clock_Disable();
	DisableNandbit();

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
		SetRompageWithHardReset(0x100);
	}
}

void ShowTime()
{
	int timer =0;
	char time[32];
	CLOCK_TIME clocktime ;	
	keypad keys ;
	DrawFilePic("itemdisc.bmp",60,40,128,80,1,0x7FFF,1);	//»­´°¿Ú
	DrawHZText12("Time",0,60+7,40+20, 3,1);	//´ÖÌå±êÌâ
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
			DrawHZTextRect(time,0,60+54,40+16,62,40,14,0,2,1);	//Ð´ÏûÏ¢
			
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
	DrawHZTextRect(buf,0,60+54,40+16,62,40,14,0,2,1);	//Ð´ÏûÏ¢
	while(1){
		ti++;
		VBlankIntrWait();		
		sleep(5);	keys.update();
		bchange = 0 ;
		isopen=0;
		if ( (keys.press(KEY_UP))&&(holdt=ti) || (keys.hold(KEY_UP)&&(ti-holdt>15)) )//ÏòÉÏÒÆ¶¯Êó±ê
		{
			number++ ;
			if(number>=100)
				number = 1 ;
			bchange =1 ;
		}
		if ( (keys.press(KEY_DOWN))&&(holdt=ti) || (keys.hold(KEY_DOWN)&&(ti-holdt>15)) )//ÏòÏÂÒÆ¶¯Êó±ê
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
			DrawHZTextRect(buf,0,60+54,40+16,62,40,14,0,2,1);	//Ð´ÏûÏ¢
		}
		if(isopen)
		{//Ð´Èë
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
			{//´´½¨Ò»¸ödir
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
//**** ÏÔÊ¾²Ëµ¥º¯Êý
//**** Ö¸¶¨±³¾°Í¼Æ¬Îªbg£¬±êÌâÍ¼Æ¬ÎªtitleBG£¬ÊäÈë²Ëµ¥×Ö·û´®Êý×émnu£¬Ö¸¶¨²Ëµ¥ÏîÊýÁ¿count¼°Ä¬ÈÏ½¹µãdefualtI
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

	if(defaultI>MENU_ITEM_SHOW_COUNT)//È·¶¨ÈçºÎÏÔÊ¾ÏÈÇ°ÔøÑ¡ÖÐµÄÎÄ¼þÎ»ÖÃ½á¹¹
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

		if(updt)	//µ±²Ëµ¥ÎÄ±¾ÄÚÈÝ½á¹¹ÐèÒª¸Ä±äÊ±£¬ÐèÒª´¦ÀíÈçÏÂ
		{
			DrawFilePic(bg,0,0,240,160,0,0,0); 
			if(titleBG[0]!=0)
				DrawFilePic(titleBG,36,0,168,42,0,0,0); 
			if(overlap[0]!=0)
				DrawFilePic(overlap,0,0,240,39,0,0,0); 			
			//´òÓ¡²Ëµ¥Ïî
			for(u8 i=mnustarti;i<mnustarti+mnushownum;i++)
				DrawHZText12(mnu[i], 0,btnx+12,48+FM_ROW_H*(i-mnustarti),0,0);
			updt = 0;
			refresh = 1;
		}
		if(refresh)	//Ñ¡ÔñÌõÎ»ÖÃ¸Ä±äÊ±
		{
			syncVVram();
			Clear(btnx-3,btny+(menui-mnustarti)*FM_ROW_H,160+6,FM_ROW_H-2,RGB(3,20,20),1);//»­ÒõÓ°
			DrawHZText12(mnu[menui], 0,btnx+12,btny+(menui-mnustarti)*FM_ROW_H+2,0,1);
			//FadeStatus(0,1);
			refresh=0;
		}


		if ( (keys->press(KEY_UP)&&(holdt=ti)) || (keys->hold(KEY_UP)&&(ti-holdt>15)) )//ÏòÉÏÒÆ¶¯Êó±ê
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

		if ( (keys->press(KEY_DOWN)&&(holdt=ti)) || (keys->hold(KEY_DOWN)&&(ti-holdt>15)) )//ÏòÏÂÒÆ¶¯Êó±ê
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
	//·µ»ØÖµÎªÑ¡ÔñµÄÐòÁÐ
	//Ê×ÏÈ»­³öÍ¼Æ¬À´
	DrawFilePic("itemdisc.bmp",x,y,128,80,1,0x7FFF,1);	//»­´°¿Ú
    DrawIcon(x+13,y+26,7,1);					//´óÍ¼±ê
	DrawFilePic("ok.bmp",x+7,y+58,24,14,1,0x7FFF,1);	//»­OK
	DrawFilePic("x.bmp",x+34,y+58,12,12,1,0x7FFF,1);	//»­X
	
}

u8 gShow=1,gx=0;
void WaitingErase()
{
	u8* pReadCache = (u8*)_ramFatBlockCache ;
	u8 ww = 20 ;
	if(gShow)
	{
		//CreateWindow(gl_tip,"",60,40,1);	
		DrawFilePic("itemdisc.bmp",60,40,128,80,1,0x7FFF,1);	//»­´°¿Ú
		DrawHZText12(gl_tip,0,50+54,40+16, 3,1);	//´ÖÌå±êÌâ
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
	DrawHZTextRect(gl_waiting,0,30+54+gx,40+36,62,80,10,0,2,1);	//Ð´ÏûÏ¢
}