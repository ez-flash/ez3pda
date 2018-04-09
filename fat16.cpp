// fat16.cpp : Defines the entry point for the console application.
//

#include "fat16.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nandop.h"
#include "sdopera.h"
#include "ecc_check.h"
#include "hard.h"
#include "inram.h"
#include "global.h"
#include "BGfunc.h"
#include "codepage.h"

struct _BPB Bpb;

DWORD RootDirSectors;	// Numbers of sectors occupied by Root Directory.
DWORD FirstDataSector;	//The first sector of cluster 2 (the data region of the disk).
DWORD FirstFatSecNum;	//Start sector of the first fat data structure.
DWORD FirstRootDirSecNum;	//Start sector of root directory.

DWORD  hidesec ;
WORD* FatCache ;//=(WORD*)0x2000000;
BYTE* blockCache ;//=(BYTE*)0x2020000;
BYTE  SDorNand ;
///////////////////////////////////////////////////////////////////
// disk cache management
///////////////////////////////////////////////////////////////////
DWORD dwPagecache[2112/4] ;
BYTE *PageCache=( BYTE*)dwPagecache;
BYTE *SectorCache = PageCache ;
DWORD CurrentCacheSector = 0xFFFFFFFF;
DWORD CurrentCachePage = 0xFFFFFFFF;
DWORD CountofClusters ;
int	 BlockAddress = -1;
int  haswrite = 0 ;
int  zerocluster = -1;
extern u16		NANDSize ;

char msg[256];
//Read a special sector into disk cache.
//NULL indicate failed.
//this for test

#include <AGB.h>
#include "BGFunc.h"
void Outputline(char *p)
{
	DmaClear(3, 0,VRAM,240*2*12,32);
	DrawHZText12(p,48,0,0,0x7FFF,1 );
}

DWORD fat_locate(const char *path, struct _file *file) ;


int toupper(int c)
{
	if(c >= 'a' && c <= 'z')
		return (c+('A'-'a'));
	else
		return c;
}

void strupr(char* string)
{
	while(*string)
	{
		*string = toupper(*string);
		string ++ ;
	}
}

int strnicmp(const char *s1, const char *s2, int len)
{
 /* Yes, Virginia, it had better be unsigned */
 unsigned char c1, c2;

 c1 = 0; c2 = 0;
 if (len) {
  do {
   c1 = *s1; c2 = *s2;
   s1++; s2++;
   if (!c1)
    break;
   if (!c2)
    break;
   if (c1 == c2)
    continue;
   c1 = toupper(c1);
   c2 = toupper(c2);
   if (c1 != c2)
    break;
  } while (--len);
 }
 return (int)c1 - (int)c2;
}

int stricmp(const char *s1, const char *s2)
{
	int len1 , len2 ;
	len1 = strlen(s1);
	len2 = strlen(s2);
	return  strnicmp(s1, s2, (len1>len2)?len1:len2) ;
}

WORD SectorNum2ClusterNum(DWORD SectorNum)
{
	return (WORD)((SectorNum - FirstDataSector) / Bpb.SecPerClus + 2);
}

WORD GetSecPerCls() 
{
	switch(Bpb.SecPerClus)
	{
		case 128:
			return 7 ;
		case 64:
			return 6 ;
		case 32:
			return 5 ;
		case 16:
			return 4 ;
		case 8:
			return 3 ;
		case 4:
			return 2 ;
	};
	return 2 ;
}

WORD GetFisrtCls()
{
	WORD k ;
	k =  FirstDataSector + hidesec - 2*Bpb.SecPerClus ;
	return k ;
}

BYTE* GetSectorData(DWORD StartSector)
{
	DWORD page = StartSector/4 ;
	DWORD pos = StartSector%4 ;
	int cluster =-7 ;
	if(SDorNand)
	{
	
		bool ret = 1 ;
		DWORD add = StartSector*512 + hidesec*512 ;
		SD_Enable();
		ret = SD_ReadSingleBlock(add,SectorCache,528) ;
		SD_Disable();
/*		
		if(page != CurrentCachePage)
		{
			SD_Enable();
			ret = SD_ReadMultiBlock(add,PageCache,2048) ;
			SD_Disable();
		}
		SectorCache = PageCache + pos*512 ;
*/
		
		if(!ret)
		{
			char pmsg[64];
			sprintf(pmsg,"[sector:%x] data:%x",(unsigned int)StartSector,SectorCache[0]);
			CreateWindow("error",pmsg,60,40,1);
		}
	
		CurrentCachePage = page ;	
		CurrentCacheSector = StartSector;
		return SectorCache ;
	}

	if(page == CurrentCachePage)
	{
		SectorCache = PageCache + pos*512 ;
	}
	else
	{//从 page*2048 读数据
		if((StartSector)&&(StartSector>=FirstDataSector))
		{
			cluster = SectorNum2ClusterNum(StartSector);
		}
		if(cluster!=zerocluster)
		{
			//
			OpenWrite();
			EnableNand8bit();
			NAND_Reset();
			Nand_Read8(page*2048,PageCache,2112);
			DisableNandbit();
			CloseWrite();
		}
		else
			memset(PageCache,0,2112);
			//DmaClear(3,0,PageCache,2112,32);		
		SectorCache = PageCache + pos*512 ;
	}
	CurrentCachePage = page ;	
	CurrentCacheSector = StartSector;
	zerocluster = -1 ;
	return SectorCache;
}

//Flush cache data onto disk.
void Flush()
{
	BYTE tmpBuf[2112] ;
	register int i ;
	if(SDorNand)
		return ;
	if(BlockAddress != -1)
	{
		if(haswrite)
			return ;
		OpenWrite();
		EnableNand8bit();
		NAND_Reset();
		NAND_Erase(BlockAddress*64*2048);
		NAND_Reset();
		for(i=0;i<64;i++)
		{
				
			DmaCopy(3,blockCache+(i<<11),tmpBuf,2048,16);
			//memcpy(tmpBuf,blockCache+(i<<11) ,2048);
		
			make_ecc_512(&tmpBuf[2048+8],tmpBuf);
			make_ecc_512(&tmpBuf[2048+8+16],tmpBuf+512);
			make_ecc_512(&tmpBuf[2048+8+32],tmpBuf+1024);
			make_ecc_512(&tmpBuf[2048+8+48],tmpBuf+1536);
			Nand_Write((BlockAddress*64+i)*2048 , tmpBuf,2112);
			NAND_Reset();
		}
		DisableNandbit();
		haswrite = 1 ;
		CloseWrite();
	}
}

//Write data into special sector. Size can more than bytes per sector.
int SetSectorData(DWORD StartSector, BYTE* Buffer, DWORD Size)
{
	register int i ;
	//memcpy(&((BYTE*)MemDisk)[StartSector * Bpb.BytsPerSec], Buffer, Size);
	int  blcAddress = StartSector/256 ;
	if(SDorNand)
	{//写入
		DWORD ss = 0 ;
		SD_Enable();
		do
		{
			SD_WriteSingleBlock(ss+StartSector*512+ hidesec*512,Buffer+ss,512);
			SD_WriteWaitDataline(0) ;
			ss += 512 ;
		}
		while(ss<Size);
		SD_Disable();
		return 0;
	}
	if(BlockAddress != blcAddress)
	{
		Flush();
		OpenWrite();
		EnableNand8bit();
		NAND_Reset();
		for(i=0;i<64;i++)
		{
			Nand_Read8((blcAddress*64+i)*2048,blockCache+(i*2048),2048);
		}
		DisableNandbit();

		CloseWrite();
		BlockAddress = blcAddress ;
	}	
	
	OpenWrite();	
	DmaCopy(3, Buffer,&((BYTE*)blockCache)[(StartSector-blcAddress*256)*512], Size,16);
	CloseWrite();
	
	//memcpy(&((BYTE*)blockCache)[(StartSector-blcAddress*256)*512], Buffer, Size) ;
	haswrite = 0;

	return 0;
}


///////////////////////////////////////////////////////////////////
//fat16 low interface
///////////////////////////////////////////////////////////////////
bool serchPDT()
{
/*
硬盘分区表（DPT）
---------------------
偏移地址 字节数 含义分析

01BE 1 分区类型：00表示非活动分区：80表示活动分区；其他为无效分区。

01BF~01C1 3 *分区的起始地址（面/扇区/磁道），通常第一分区的起始地址开始
于1面0道1扇区，因此这三个字节应为010100

01C2 1 #分区的操作系统的类型。

01C3~01C5 3 *该分区的结束地址（面/扇/道）

01C6~01C9 4 该分区起始逻辑扇区

01CA~01CD 4 该分区占用的总扇区数

注释: * 注意分区的起始地址（面/扇区/磁道）和结束地址（面/扇/道）中字节分配:

00000000 01000001 00010101
~~~~~~~~ ==^^^^^^ ========

~ 面(磁头) 8 位
^ 扇区 6 位
= 磁道 10 位

# 分区的操作系统类型(文件格式标志码)

4---DOS FAT16<32M
5---EXTEND
6---DOS FAT16>32M
7---NTFS(OS/2)
83---LINUX>64M


DPT 总共64字节（01BE--01FD）, 如上所示每个分区占16个字节, 所以可以表示四个分区, 这也
就是为什么一个磁盘的主分区和扩展分区之和总共只能有四个的原因. 
*/
	BYTE* bootsec;
	bootsec = GetSectorData(0);
	if(((bootsec[0]==0xeb)||(bootsec[0]==0xe9))&&(bootsec[0x1fe]==0x55)&&(bootsec[0x1FF]==0xAA)) 
		return 0x0;
	else if((bootsec[0x1fe]==0x55)&&(bootsec[0x1FF]==0xAA))
	{//this disk has a pdt(分区表)
		hidesec = *(WORD*)(bootsec+0x1C6) ; //起始逻辑地址
/*	
	char pmsg[64];
	sprintf(pmsg,"[7]%x",hidesec);
	DrawHZText12(pmsg,0, 2, 140,0xFFFF, 1 );	
	while(1);
*/		
		return true ;
	}
	else 
		return false ;
}

// 0 indicate success.
int fat_init(int useEram)
{
	BYTE* bootsec;
	DWORD DataSec;
	BYTE* Cache;
	WORD* pFat;
	DWORD Sector;
	
//char pmsg[64];
	
	if(useEram)
	{
		FatCache =(WORD*)_ramFatCache;
		blockCache =(BYTE*)_ramFatBlockCache;
	}
	else
	{
		FatCache =(WORD*)_psFatCache;
		blockCache =(BYTE*)_psFatBlockCache;
	}
	// Initialize BPB.
	hidesec = 0 ;
	CurrentCachePage = 0xFFFFFFFF;
	serchPDT();
	CurrentCachePage = 0xFFFFFFFF;
	bootsec = GetSectorData(0);
//CreateWindow("测试","GetSectorData(0)!\n",60,40,1);
	if((bootsec[0]!=0xeb)&&(bootsec[0]!=0xe9)) return 0x100;
	
	Bpb.BytsPerSec = bootsec[11]+(bootsec[12]<<8);
	Bpb.SecPerClus = bootsec[13];
	Bpb.RsvdSecCnt = *((WORD*)&bootsec[14]);
	Bpb.NumFATs = bootsec[16];
	Bpb.RootEntCnt = bootsec[17]+(bootsec[18]<<8);
	if((Bpb.TotSec = (bootsec[19]+(bootsec[20]<<8))) == 0)
		Bpb.TotSec = *((DWORD*)&bootsec[32]);
	if((Bpb.FATSz = *((WORD*)&bootsec[22])) == 0)
		Bpb.FATSz = *((DWORD*)&bootsec[36]);

	RootDirSectors = ((Bpb.RootEntCnt * 32) + (Bpb.BytsPerSec - 1)) / Bpb.BytsPerSec;

	DataSec = Bpb.TotSec - (Bpb.RsvdSecCnt + (Bpb.NumFATs * Bpb.FATSz) + RootDirSectors);
	CountofClusters = DataSec / Bpb.SecPerClus;
	
	/*FAT12 < 4085, FAT32 >= 65525*/
	if(CountofClusters < 4085 || CountofClusters >= 65525)
		return 1;

	FirstFatSecNum = Bpb.RsvdSecCnt;
	FirstRootDirSecNum = Bpb.RsvdSecCnt + (Bpb.NumFATs * Bpb.FATSz);
	FirstDataSector = Bpb.RsvdSecCnt + (Bpb.NumFATs * Bpb.FATSz) + RootDirSectors;

	for(Sector = FirstFatSecNum, pFat = FatCache; Sector <(FirstFatSecNum+Bpb.FATSz); Sector++, pFat+=Bpb.BytsPerSec/sizeof(WORD))
	{
		Cache = GetSectorData(Sector);
		if(Cache == NULL)
		{
			//free(FatCache);
			return 3;
		}
		OpenWrite();
		DmaCopy(3, Cache,pFat, Bpb.BytsPerSec,32);
		CloseWrite();
		//memcpy(pFat, Cache, Bpb.BytsPerSec);

	}	

	memset(&gfat_get, 0, sizeof(FatGet));
	memset(&handles, 0, sizeof(handles));


	return 0;
}

void fat_deinit() //写入分区表
{
	register DWORD i,j;

	// close all opened files
	for(i=0; i<16; i++)
	{
		if(handles[i].valid)
		{
			fat_close(i);
		}
	}

	// flush the fat cache to disk.
	for(i = 0; i < Bpb.NumFATs; i++)
	{
		for(j=0;j<Bpb.FATSz;j++)
			SetSectorData(FirstFatSecNum+i*Bpb.FATSz + j, ((BYTE*)FatCache)+j*Bpb.BytsPerSec , Bpb.BytsPerSec );
	}
	Flush();
	CurrentCacheSector = 0xFFFFFFFF;
	CurrentCachePage = 0xFFFFFFFF;
	BlockAddress = -1;
}

DWORD ClusterNum2SectorNum(WORD ClusterNum)
{
	return FirstDataSector + (ClusterNum - 2) * Bpb.SecPerClus;
}

//return next cluster num,
//0xffff indicate no next cluster.
//Note! : this function will dirty cache!
WORD GetNextClusterNum(WORD ClusterNum)
{
	return FatCache[ClusterNum];
}

// alloc a free cluster. policy is searching from prev cluster number, if no free cluster till end of fat, then search from head of fat.
// return a cluster number. 0xffff indicate faild, disk overflow.
// argument 0 : no prev cluster.
WORD AllocCluster(WORD PrevClusterNum)
{
	static WORD LastAllocClusterNum=0;
	WORD i;

	if(LastAllocClusterNum == 0)
		LastAllocClusterNum = PrevClusterNum;

	for(i = LastAllocClusterNum; i < CountofClusters /*Bpb.BytsPerSec * Bpb.FATSz / sizeof(WORD)*/; i++)
	{
		if(FatCache[i] == 0)
		{
			OpenWrite();
			FatCache[i] = 0xffff;	// flag with 0xffff, this is the last cluster.
			
			LastAllocClusterNum = i;
			
			//chain this cluster to prev one.
			if(PrevClusterNum != 0)
				FatCache[PrevClusterNum] = LastAllocClusterNum;
				
			CloseWrite();
			zerocluster = LastAllocClusterNum ;

			return LastAllocClusterNum;
		}
	}

	// we have to search from head of fat
	for(i = 2; i < CountofClusters/*Bpb.BytsPerSec * Bpb.FATSz / sizeof(WORD)*/; i++)
	{
		if(FatCache[i] == 0)
		{
			OpenWrite();
			FatCache[i] = 0xffff;	// flag with 0xffff, this is the last cluster.
			
			LastAllocClusterNum = i;
			
			//chain this cluster to prev one.	
			if(PrevClusterNum != 0)
				FatCache[PrevClusterNum] = LastAllocClusterNum;
			CloseWrite();
			zerocluster = LastAllocClusterNum ;
			return LastAllocClusterNum;
		}
	}
	
	return 0xffff;
}

// free cluster chain.
// argument 0 : no prev cluster.
void FreeCluster(WORD StartClusterNum)
{
	WORD Cluster;
	WORD NextCluster;

	Cluster = StartClusterNum;

	while(Cluster != 0xffff)
	{
		NextCluster = FatCache[Cluster];
		OpenWrite();		
		FatCache[Cluster] = 0x0000;
		CloseWrite();
		Cluster = NextCluster;
	}
}


int vfat_create_shortname(char*path,char* dosname)
{
	int i ,j,pos;
	DWORD locate ;
	char first[10] = {0};
	char ext[4] = {0};
	char filename[256];
	char full[768];
	char tmp[8];
	char*p = strrchr(path,'\\');
	strcpy(filename,p+1);
	strncpy(full,path,p-path );
	unsigned int len = strlen(filename);
	struct _file * fp = NULL;
	
	for(i=len-1 ;i>=0 ;i--)
	{
		if(filename[i] == 0x2E) //找到
			break;
	}
	pos = i ;
	if(i == 0)
	{
		ext[0] = 0 ;
	}
	else if((len -i)>=4)
		strncpy(ext, filename+i+1,3) ;
	else
		strcpy(ext,filename+i+1);
	//查找文件是否存在
	for(i=1;i<0x100;i++)
	{
		memcpy(first,filename , 8);
		sprintf(tmp,"~%d",i);
		pos = strlen(tmp);
		//形成文件
		for(j=0;j<8-pos;j++)
		{
			if(first[j]>=0x80)
				j++ ;
		}
		if(j==8-pos)
		{
			memcpy(&first[8-pos],tmp,pos);
		}
		else
		{
			memcpy(&first[8-pos-1],tmp,pos);
			first[7] = 0x20 ;
		}
		strncpy(full,path,p-path );
		sprintf(full,"%s%s",full,first);
		locate = fat_locate(full, fp );
		if(locate == 0xFFFFFFFF)
			break;
	}
	strcpy(dosname,first);
	return 0 ;
}

int Local2uni(char *name , _wchar_t *outname , int &len)
{
	int i=0,outlen = 0 ;
	int namelen = strlen(name);
	char intmp[4];
	for(i=0;i<namelen ;i++)
	{
		if(name[i]<=0x80)
			outname[outlen] = name[i] ;
		else
		{
			intmp[0] = name[i];
			intmp[1] = name[i++];
			char2uni((unsigned char*)intmp, 2 , &outname[outlen]) ;
		}
		outlen ++ ;
	}
	len = outlen ;
	return 0 ;
}

int BuildSlot(char *pathname ,struct _DIR* newdir,int &numofdir)
{
	char filename[256] ;
	_wchar_t uniname[256];
	char shortname[12];
	int len , outlen , i , longname , offset; 
	char*p = strrchr(pathname,'\\');
	strcpy(filename,p+1);
	len = strlen(filename);
	struct dir_slot * pdir = (struct dir_slot *)newdir ;
	struct _DIR*  pd2 ;
	longname = 0 ;
	for(i = 0 ;i<len ;i++)
	{
		if(filename[i] >=0x80)
		{
			longname = 1 ;
			break;
		}
		if(len >11)
		{
			longname = 1 ;
			break; 
		}
	}
	
	if(longname)
	{
		vfat_create_shortname(pathname,shortname);
		Local2uni(filename, uniname, outlen) ;		
		numofdir =  outlen / 13;;
		for(i=numofdir;i>0;i--)
		{

			pdir ->id = i ;
			pdir->attr = ATTR_EXT ;
			pdir->reserved = 0;
			//pdir->alias_checksum = cksum;
			pdir->start = 0;
			offset = (i - 1) * 13;
			memcpy(pdir->name0_4, uniname + offset, 5);
			memcpy(pdir->name5_10, uniname + offset + 5, 6);
			memcpy(pdir->name11_12, uniname + offset + 11, 2);
			pdir ++ ;
		}
		newdir->Name[0]  |= 0x40 ; 
		pd2 = (struct _DIR* )pdir ;
		//memcpy(
	}
	else
	{
		numofdir = 1 ;
	}
	return 0 ;
}

// return the first sector number of dir content .
// 0xffffffff indicate failed.
int AllocDir(DWORD ParentDirSectorNum, struct _DIR* new_dir, struct _file * fp)
{
	BYTE* Cache;
	struct _DIR *dir;
	DWORD i;
	WORD PrevCluster = 0xFFFF ;
	WORD Cluster;
	DWORD DirSectorNum = ParentDirSectorNum;

	//if(dirname == NULL)
		//return 1;

	if(ParentDirSectorNum == FirstRootDirSecNum)
	{
		for(i=0; i<Bpb.RootEntCnt * sizeof(struct _DIR) / Bpb.BytsPerSec; i++)
		{
			Cache = GetSectorData(DirSectorNum);
			if(Cache == NULL)
				return 2;
			
			for(dir = (struct _DIR *)Cache; (BYTE*)dir < Cache + Bpb.BytsPerSec; dir++)
			{
				if(dir->Name[0] == '\0' || dir->Name[0] == ERCHAR ||(dir->Name[0]==0xFF&&dir->Name[1]==0xFF))
				{
					memcpy(dir, new_dir, sizeof(struct _DIR));					
					SetSectorData(DirSectorNum, Cache , Bpb.BytsPerSec);

					if(fp)
					{
						fp->DirSectorNum = DirSectorNum;
						fp->DirIndex = ((BYTE*)dir - Cache) / sizeof(struct _DIR);
						memcpy(&fp->dir, new_dir, sizeof(struct _DIR));
					}
					Flush();
					return 0;
				}
			}
			DirSectorNum++;
		}
		// root dir have no room.
	 	return 3; 
	}
	
	else
	{
		Cluster = SectorNum2ClusterNum(DirSectorNum);
		
		while(Cluster != 0xffff)
		{
			for(i=0; i< Bpb.SecPerClus; i++)
			{
				Cache = GetSectorData(DirSectorNum);
				if(Cache == NULL)
					return 2;
				
				for(dir = (struct _DIR *)Cache; (BYTE*)dir < Cache + Bpb.BytsPerSec; dir++)
				{
					if(dir->Name[0] == '\0' || dir->Name[0] == ERCHAR||(dir->Name[0]==0xFF&&dir->Name[1]==0xFF))
					{
						memcpy(dir, new_dir, sizeof(struct _DIR));
						SetSectorData(DirSectorNum, Cache , Bpb.BytsPerSec);
						
						if(fp)
						{
							fp->DirSectorNum = DirSectorNum;
							fp->DirIndex = ((BYTE*)dir - Cache) / sizeof(struct _DIR);
							memcpy(&fp->dir, new_dir, sizeof(struct _DIR));
						}
						
						Flush();
						return 0;
					}
				}
				DirSectorNum++;
			}
			
			PrevCluster = Cluster;
			Cluster = GetNextClusterNum(Cluster);
			DirSectorNum = ClusterNum2SectorNum(Cluster);
		}
		
		//
		// we have to extend this parent dir room.
		//
		Cluster = AllocCluster(PrevCluster);
		if(Cluster == 0xffff)
			return 4;
		
		DirSectorNum = ClusterNum2SectorNum(Cluster);
		
		Cache = GetSectorData(DirSectorNum);
		if(Cache == NULL)
			return 2;
		
		dir = (struct _DIR *)Cache;
		
		memcpy(dir, new_dir, sizeof(struct _DIR));
		SetSectorData(DirSectorNum, Cache , Bpb.BytsPerSec);
		Flush();
		
		if(fp)
		{
			fp->DirSectorNum = DirSectorNum;
			fp->DirIndex = ((BYTE*)dir - Cache) / sizeof(struct _DIR);
			memcpy(&fp->dir, new_dir, sizeof(struct _DIR));
		}
		
		return 0;
	}

	return 5;
}

int DeleteDir(struct _file *file)
{
	BYTE* Cache;
	struct _DIR *dir;

	Cache = GetSectorData(file->DirSectorNum);
	if(Cache == NULL)
		return 1;

	dir = (struct _DIR *)Cache;
	dir += file->DirIndex;

	dir->Name[0] = ERCHAR;
	SetSectorData(file->DirSectorNum, Cache , Bpb.BytsPerSec);
	Flush();

	return 0;
}

// helper functions

// NULL indicate failed.
//Valid format is full path:	\[8.3\]*DIR_Name
//limits:
// length < 256 && !(special char)

char  vfat_bad_char(char  w)
{
	return (w < 0x20)
	    || (w == '*') || (w == '?') || (w == '<') || (w == '>')
	    || (w == '|') || (w == '"') || (w == ':') || (w == '/')
	    || (w == '\\');
}

char  vfat_replace_char(char w)
{
	return (w == '[') || (w == ']') || (w == ';') || (w == ',')
	    || (w == '+') || (w == '=');
}

char vfat_skip_char(char w)
{
	return (w == '.') || (w == ' ');
}

int vfat_is_used_badchars(char *s, int len)
{
	int i;

	for (i = 0; i < len; i++)
	{
		if (vfat_bad_char(s[i]))
			return -4 ;
		//if(vfat_replace_char(s[i]))
		//	return -4 ;
	}
	return 0;
}

int get_valid_format(const char *fullpath)
{
	char ptmp[maxNameLen+16];
	char *pslash = (char*)fullpath ;
	char *plin =  (char*)fullpath ;
	int len = strlen(fullpath);
	if(pslash[0] != '\\') //非法的路径名
		return -1 ;
	if(len == 1)
		return 0 ;
	if((len>1)&&(pslash[len-1] == '\\'))
		return -1 ;
	plin ++ ;
	do
	{
		pslash = strchr(plin,'\\');
		memset(ptmp,0,maxNameLen);
		if(pslash)
		{
			if((pslash-plin)>maxNameLen)
				return -2 ;
			memcpy(ptmp,plin, pslash-plin );
			len = strlen(ptmp);
			plin = pslash +1 ;
		}
		else
		{
			if(strlen(plin)>maxNameLen)
				return -2 ;
			memcpy(ptmp,plin, strlen(plin));
			len = strlen(ptmp);
			plin = 0 ;
		}
		if(vfat_is_used_badchars(ptmp,len)<0)
			return -5;
		if (len == 3 || (len > 3 && ptmp[3] == '.'))
		{	/* basename == 3 */
			if (!strnicmp(ptmp, "aux", 3) ||
			    !strnicmp(ptmp, "con", 3) ||
			    !strnicmp(ptmp, "nul", 3) ||
			    !strnicmp(ptmp, "prn", 3))
				return -3;
		}
		if (len == 4 || (len > 4 && ptmp[4] == '.')) {	/* basename == 4 */
		/* "com1", "com2", ... */
			if ('1' <= ptmp[3] && ptmp[3] <= '9') {
				if (!strnicmp(ptmp, "com", 3) ||
			   		 !strnicmp(ptmp, "lpt", 3))
				return -3;
			}
		}

	}
	while(plin);
	return 0 ;
}

//
void unformat_name(char * filename, const unsigned char dirname[11])
{
	int i;
	int j;

	memset(filename, 0, 14);

	for(i=0; i<8; i++)
	{
		if(dirname[i] != 0x20)
			filename[i] = dirname[i];
		else
			break;
	}
	
	if(dirname[8] != 0x20)
	{
		filename[i] = '.';
		j = i + 1;
		
		for(i=8; i<11; i++,j++)
		{
			if(dirname[i] != 0x20)
				filename[j] = dirname[i];
			else
				break;
		}
	}
}

//以下的常量仅在SectorSearch中使用
static DWORD longfirstSector ;  		//长文件名第一个ｄｉｒ的sector
static DWORD longfirstSectoroffset ; 	//长文件名第一个ｄｉｒ的sector的偏移
static WORD Islongfile = 0 ;				//是否长文件名
static WORD nameoffset ;				//用于多次进入的名称拷贝偏移指示
static char longname[maxNameLen+1];
DWORD SectorSearch(DWORD Sector, const char *dirname, struct _file *file)
{
	int i , out;
	BYTE* Cache;
	struct _DIR *dir;
	struct dir_slot *ldir;
	_wchar_t  chardir[16] ;
	unsigned char single[4];
	unsigned int j;
//char tmp[64];
	Cache = GetSectorData(Sector);
	if(Cache == NULL)
		return 0xffffffff;
	
	
	dir = (struct _DIR *)Cache;
	ldir = (struct dir_slot*)Cache; 
	for(j=0; j< Bpb.BytsPerSec / sizeof(struct _DIR);j++) //一个sector仅有16个slot
	{	
		if(Islongfile&&(ldir->attr == 0xF))
		{//继续上个sector 未完事业
			memcpy(chardir,ldir->name0_4,10);
			memcpy(chardir+5,ldir->name5_10,12);
			memcpy(chardir+11,ldir->name11_12,4);
			for(i = 12;i>=0;i--)
			{
				if(chardir[i] == 0xFFFF)
					continue ;
				if(chardir[i] != 0x0)
					out = uni2char(chardir[i],single,4);
				else
				{
					single[0] = 0 ;
					out = 1 ;
				}
				if(out<0)
				{
					//continue;
					single[0] = '?' ;
					out = 1 ;
				}
				memcpy(longname+nameoffset-out,single,out);
				nameoffset -= out ;
			}
		
		}
		else if((dir->Name[0] != 0xE5 )&&(ldir->attr == 0xF)&&((ldir->id&0xC0) == 0x40))
		{ //长文件名的第一个，开始处理

			longfirstSector = Sector ;
			longfirstSectoroffset = j ;
			Islongfile=1 ;
			nameoffset = maxNameLen;
			memset(longname,0,maxNameLen); 
			//
			memcpy(chardir,ldir->name0_4,10);
			memcpy(chardir+5,ldir->name5_10,12);
			memcpy(chardir+11,ldir->name11_12,4);
//			DrawHZText12("1--",0,10,10,FM_CL_WHITE,1);
			for(i = 12;i>=0;i--)
			{
			//	sprintf(msg,"i=%d",i);
		//		DrawHZText12(msg,0,50,10*(12-i),FM_CL_WHITE,1);
				if(chardir[i] == 0xFFFF)
					continue ;
				if(chardir[i] != 0x0)
					out = uni2char(chardir[i],single,4);
				else
				{
					single[0] = 0 ;
					out = 1 ;
				}
				if(out<0)
				{
					//continue;
					single[0] = '?' ;
					out = 1 ;
				}
				memcpy(longname+nameoffset-out,single,out);
				nameoffset -= out ;
			}
//			DrawHZText12("2--",0,10,150,FM_CL_WHITE,1);
		
						
		}
		else if((dir->Name[0] != 0xE5 )&&(dir->Name[0]!=0xFF&&dir->Name[1]!=0xFF) &&(dir->Name[0]!=0x0)&&(dir->Attr!=ATTR_VOLUME_ID))
		{//忽略掉0xe5 , 和0 , 及0xFF(特别处理) -- 短文件名处理
			unsigned char dname[16];
			memcpy(dname,dir->Name,11);
			dname[11]=0;
			if(Islongfile)
			{
				char dosname[16] ;
	                     unformat_name(dosname,dname);
				if((!stricmp(dirname,dosname))||(!strnicmp(longname+nameoffset,dirname,strlen(longname+nameoffset)))&&(strlen(dirname)==strlen(longname+nameoffset)))
				{
					memset(file, 0, sizeof(struct _file));
					file->DirSectorNum = Sector; //--------
					file->DirIndex = j;
					file->longfilename = 1 ;
					file->longfisrtSector= longfirstSector ;
					file->longfisrtSectoroffset = longfirstSectoroffset ;
					memcpy(&(file->dir), dir, sizeof(struct _DIR));

//sprintf(tmp,"%s,%x",longname+nameoffset ,file->dir.FileSize);
//CreateWindow("SectorSearch2", tmp , 10, 10, 1) ;
//while(*(vu16*)0x04000130 == 0x3FF );
//while(*(vu16*)0x04000130 != 0x3FF );
		
					Islongfile = 0 ;
					return ClusterNum2SectorNum(dir->FstClusLO);
				}
				Islongfile = 0 ;
			}
			else
			{
				Islongfile = 0 ;
				if(!(dir->Attr & ATTR_VOLUME_ID))
	                     {//进一步处理
	                     	unformat_name(longname,dname);
					if(!stricmp(longname,dirname))
					{
						memset(file, 0, sizeof(struct _file));
						file->DirSectorNum = Sector;
						file->DirIndex = j;
						file->longfilename = 0 ;
						file->longfisrtSector = 0 ;
						file->longfisrtSectoroffset = 0 ;
						memcpy(&(file->dir), dir, sizeof(struct _DIR));
						return ClusterNum2SectorNum(dir->FstClusLO);
					}
				}
			}
		}
		else
		{	//其他的跳过不处理 
			Islongfile = 0 ;
		}
		dir ++ ;
		ldir ++ ;
	}
	return 0xffffffff;
}

//以下的常量仅在SectorGet中使用
static DWORD sg_longfirstSector ;  		//长文件名第一个ｄｉｒ的sector
static DWORD sg_longfirstSectoroffset ; 	//长文件名第一个ｄｉｒ的sector的偏移
static WORD sg_Islongfile  = 0;				//是否长文件名
static WORD sg_nameoffset ;				//用于多次进入的名称拷贝偏移指示
static char sg_longname[maxNameLen];


int SectorGet(DWORD Sector, struct FatGet *fat_get)
{
	BYTE* Cache;
	struct _DIR *dir;
	struct dir_slot *ldir;
	_wchar_t  chardir[16] ;
	unsigned char single[4];
	unsigned int j;
	int i,out ;

//char tmp[64];

	Cache = GetSectorData(Sector);
	if(Cache == NULL)
		return 1;

	dir = (struct _DIR *)Cache;
	ldir = (struct dir_slot*)Cache; 
	
	dir += fat_get->DirIndex;
	ldir += fat_get->DirIndex;

	for(j= fat_get->DirIndex ; (j< Bpb.BytsPerSec / sizeof(struct _DIR)); j++)
	{
//sprintf(tmp,"[Sector%x:%x]%x, %x,%x[L%x]",Sector,j,ldir->id ,dir->Name[1],ldir->attr,sg_Islongfile);
//CreateWindow("SectorGet", tmp , 10, 10, 1, 1) ;

		if(sg_Islongfile&&(ldir->attr == 0xF))
		{//继续上个sector 未完事业
//CreateWindow("SectorGet", "1" , 10, 10, 1, 1) ;
			memcpy(chardir,ldir->name0_4,10);
			memcpy(chardir+5,ldir->name5_10,12);
			memcpy(chardir+11,ldir->name11_12,4);
			for(i = 12;i>=0;i--)
			{
				if(chardir[i] == 0xFFFF)
					continue  ;
				if(chardir[i] != 0x0)
					out = uni2char(chardir[i],single,4);
				else
				{
					single[0] = 0 ;
					out = 1 ;
				}
				if(out<0)
				{
					//continue;
					single[0] = '?' ;
					out = 1 ;
				}
				
				memcpy(sg_longname+sg_nameoffset-out,single,out);
				sg_nameoffset -= out ;
//CreateWindow("uni2char", (char*)sg_longname+sg_nameoffset , 10, 10, 1, 1) ;
			}
		}
		else if((dir->Name[0] != 0xE5 )&&(ldir->attr == 0xF)&&((ldir->id&0xC0) == 0x40))
		{ //长文件名的第一个，开始处理

//CreateWindow("SectorGet", "2" , 10, 10, 1, 1) ;
			sg_Islongfile = 1 ;
			sg_longfirstSector = Sector ;
			sg_longfirstSectoroffset = j ;
			sg_nameoffset = maxNameLen;
			//
			memset(sg_longname,0,maxNameLen);
			memcpy(chardir,ldir->name0_4,10);
			memcpy(chardir+5,ldir->name5_10,12);
			memcpy(chardir+11,ldir->name11_12,4);
			for(i = 12 ;i>=0;i--)
			{
				memset(single,0,4);
				if(chardir[i] == 0xFFFF)
					continue ;
				if(chardir[i] != 0x0)
					out = uni2char(chardir[i],single,4);
				else
				{
					single[0] = 0 ;
					out = 1 ;
				}
				if(out<0)
				{
					//continue;
					single[0] = '?' ;
					out = 1 ;
				}
				memcpy(sg_longname+sg_nameoffset-out,single,out);
				sg_nameoffset -= out ;

//sprintf(tmp,"[chardir:%x]%s [%x]",chardir[i],single,sg_nameoffset);
//CreateWindow(tmp, (char*)sg_longname+sg_nameoffset , 10, 10, 1, 1) ;
			}
		}
		else if((dir->Name[0] != 0xE5 )&&(dir->Name[0]!=0xFF&&dir->Name[1]!=0xFF) &&(dir->Name[0]!=0x0)&&(dir->Attr!=ATTR_VOLUME_ID))
		{//忽略掉0xe5 , 和0 , 及0xFF(特别处理) -- 短文件名处理
//sprintf(tmp,"SectorGet %x",sg_Islongfile);
//CreateWindow(tmp, "3" , 10, 10, 1, 1) ;
			unsigned char dname[16];
			memcpy(dname,dir->Name,11);
			dname[11]=0;
			if(sg_Islongfile)
			{
				fat_get->DirSectorNum = Sector;
				fat_get->DirIndex = fat_get->DirIndex+1;
				fat_get->longfilename = 1 ;
				fat_get->longfisrtSector = sg_longfirstSector ;
				fat_get->longfisrtSectoroffset = sg_longfirstSectoroffset ;
				memset(fat_get->filename,0,maxNameLen);
				memcpy(fat_get->filename,sg_longname+sg_nameoffset,maxNameLen-sg_nameoffset)	;			
				if(fat_get->DirIndex == (int)(Bpb.BytsPerSec / sizeof(struct _DIR)))
				{
					fat_get->DirIndex = 0;
					fat_get->DirSectorNum ++ ;
				}
			}
			else
			{
				fat_get->DirSectorNum = Sector;
				fat_get->DirIndex = fat_get->DirIndex+1;
				unformat_name(fat_get->filename, dname);				
				if(fat_get->DirIndex == (int)(Bpb.BytsPerSec / sizeof(struct _DIR)))
				{
					fat_get->DirIndex = 0;
					fat_get->DirSectorNum ++ ;
				}
			}
			sg_Islongfile = 0 ;
			return 0;
		}
		else
		{
//CreateWindow("SectorGet", "4" , 10, 10, 1, 1) ;
			sg_Islongfile = 0 ;
		}
		fat_get->DirIndex = fat_get->DirIndex +1;		
		dir++ ;
		ldir++ ;
	}
	
	if(fat_get->DirIndex == (int)(Bpb.BytsPerSec / sizeof(struct _DIR)))
	{
		fat_get->DirIndex = 0;
		fat_get->DirSectorNum ++ ;
	}
	
	return 2;
}

char*  toDosDirname(const char *fullpath)
{
	static char static_path[256];
	char* p=static_path;
	char path[80];
	char* ppath = path;
	int dir_len_count = 0; //count dir len.
	int i;

	if(fullpath == NULL || strlen(fullpath) >=80 || *fullpath != '\\')
		return NULL;

	if(strlen(fullpath) > 1 && fullpath[strlen(fullpath)-1] =='\\')
		return NULL;

	strcpy(path, fullpath);
	strupr(path);
	memset(p, 0, 256);

	for(;;)
	{
		switch(*ppath)
		{
		case 0x00:
			{
				if(dir_len_count != 0) // prev is not '\\'
				{
					for(i=0; i<(11 - dir_len_count); i++)
					{
						*p = 0x20;
						p++;
					}
				}
			}
			return static_path;

		case '\\':
			{
				if(p != static_path) // this is not the first '\\'
				{
					if(dir_len_count == 0)// more than one '\\'
						return NULL;

					for(i=0; i<(11 - dir_len_count); i++)
					{
						*p = 0x20;
						p++;
					}
				}
				
				*p = '\\';
				p++;
				
				ppath++;
				dir_len_count =0;
				continue;
			}
			break;

		case '.':
			{
				if(dir_len_count > 8 || dir_len_count ==0) // '\\.' check
					return NULL;

				if(ppath[1] == '.' || ppath[1] == '\\') // more than one '.' or '.\\' check
					return NULL;

				for(i=0; i<(8 - dir_len_count); i++)
				{
					*p = 0x20;
					p++;
				}

				dir_len_count =8;
				ppath++;
				continue;
			}
			break;

		case 0x22:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2F:
		case 0x3A:
		case 0x3B:
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
		case 0x5B:
		case 0x5D:
		case 0x7C:
			return NULL;

		default:
			{
				if(*(unsigned char*)ppath < 0x20)
					return NULL;				
			}
			break;
		}

		*p = *ppath;
		dir_len_count ++;

		if(dir_len_count > 11)
			return NULL;

		p++;
		ppath++;
	}

	return static_path;
}


// return the first sector number of dir/file content.
// 0xffffffff indicate failed.
DWORD fat_search(DWORD Sector, const char* dirname, struct _file *file)
{	
	unsigned int i;
	WORD Cluster;
	DWORD FirstSectorOfFile;
	 struct _file linfile ;
	if(Sector == FirstRootDirSecNum)
	{
		for(i=0; i<Bpb.RootEntCnt * sizeof(struct _DIR) / Bpb.BytsPerSec; i++)
		{
			FirstSectorOfFile = SectorSearch(Sector++, dirname, &linfile);
			if(FirstSectorOfFile != 0xffffffff)
			{
//Outputline((char*)linfile.dir.Name);
//while(*(vu16*)0x04000130 == 0x3FF );
//while(*(vu16*)0x04000130 != 0x3FF );
				memcpy(file,&linfile,sizeof(_file));
				return FirstSectorOfFile;
			}
		}
	}

	else
	{
		Cluster = SectorNum2ClusterNum(Sector);
		while(Cluster != 0xffff)
		{
			for(i=0; i< Bpb.SecPerClus; i++)
			{
				FirstSectorOfFile = SectorSearch(Sector++, dirname, file);
				if(FirstSectorOfFile != 0xffffffff)
					return FirstSectorOfFile;
			}

			Cluster = GetNextClusterNum(Cluster);
			Sector = ClusterNum2SectorNum(Cluster);
		}
	}
	return 0xffffffff;
}

// return the first sector number of dir/file content.
// 0xffffffff indicate failed.
// if path is root dir, file arg is ignore.
DWORD fat_locate(const char *path, struct _file *file)
{
	
	DWORD Sector = FirstRootDirSecNum;
	char pname[maxNameLen] ; 
	char *p1 = (char*)path+1;
	char *p = (char*)path+1;
//char tmp[64] ;	
	if( get_valid_format(path)<0)
		return 0xffffffff;

	// locate next sub dir(s).
	for(;;)
	{
	//第一次进入表示根目录
		if(p == 0)
			return Sector;
		if((*p == 0))
		{
//CreateWindow("quit locate", (char*)path , 10, 10, 1, 1) ;	
			return Sector;
		}
		p = strchr(p1,'\\');
		memset(pname,0,maxNameLen);
		if(p)
			memcpy(pname,p1,p-p1);
		else
			memcpy(pname,p1,strlen(p1));
			
		Sector = fat_search(Sector, pname, file);
//Outputline(pname);
//sprintf(tmp,"%x,%x",file->DirSectorNum,file->dir.FileSize);	
//CreateWindow(tmp,(char*)file->dir.Name , 10, 10, 1) ;
//		while(*(vu16*)0x04000130 == 0x3FF );
//		while(*(vu16*)0x04000130 != 0x3FF );

	
		if(Sector == 0xffffffff)		
				return 0xffffffff;
		p1=p + 1 ;
	}

	// we never go here.
	return 0xffffffff;
}

void fat_datetime(struct FatDateTime *fatdatetime)
{

	CLOCK_TIME systm;
	struct FatDate* pdate = &fatdatetime->Date.fatdate;
	struct FatTime *ptm = &fatdatetime->Time.fattime;

	GetTime(&systm);

	pdate->Day = systm.data;
	pdate->Month = systm.month;
	pdate->Year = systm.year + 20;

	ptm->Hours = systm.hour;
	ptm->Minutes = systm.minute;
	ptm->Second_2s = systm.second / 2;

	fatdatetime->TimeTenth = (systm.second % 2) * 100 ;
}

///////////////////////////////////////////////////////////////////
// fat apis
///////////////////////////////////////////////////////////////////

int fat_mkdir( const char *dirname)
{
	struct _DIR dir;
	DWORD SectorNum,sec;
	char path[512];
	char name[11];
	char *p;
	struct FatDateTime tm;
	struct _file file;	
	BYTE* Cache;
	struct _DIR *pdir;
	WORD NewCluster;
	char dot[11] = {'.', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
	char dotdot[11] = {'.', '.', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};

	// is path format correct ?
	if(get_valid_format(dirname)<0)
		return 1;

	//if exist this dir ?
	if(fat_locate(dirname, NULL) != 0xffffffff)
		return 4;

//处理8+3的文件名
	p = toDosDirname(dirname);
	if(p == NULL)
		return 1;
	//separate path into parent and name
	strncpy(name, &p[strlen(p)-11], 11);

	strcpy(path, dirname);
	p = strrchr(path, '\\');
	if(p == path) // it is root dir.
		*(p+1) = '\0';
	else
		*p = '\0';

	//locate parent path
	SectorNum = fat_locate(path, NULL);
	if(SectorNum == 0xffffffff)
		return 2;

	//fill dir attributes
	memset(&dir, 0, sizeof(dir));
	memcpy(dir.Name, name, 11);
	dir.Attr = ATTR_DIRECTORY;
	fat_datetime(&tm);
	dir.CrtDate = dir.LstAccDate = dir.WrtDate = tm.Date.Date;
	dir.CrtTime = dir.WrtTime = tm.Time.Time;
	dir.CrtTimeTenth = tm.TimeTenth;

	//alloc one dir
	if(AllocDir(SectorNum, &dir, &file) != 0)
		return 3;

	//alloc a cluster
	NewCluster = AllocCluster(0);
	if(NewCluster == 0xffff)
		return 4;

	//flush to disk
	Cache = GetSectorData(file.DirSectorNum);
	if(Cache == NULL)
		return 6;

	pdir = (struct _DIR *)Cache;
	pdir += file.DirIndex;

	pdir->FstClusLO = NewCluster;
	
	SetSectorData(file.DirSectorNum, Cache , Bpb.BytsPerSec);
	Flush();

	//create . and .. dir items.
	zerocluster = NewCluster ;
	sec = ClusterNum2SectorNum(NewCluster) ;
	Cache = GetSectorData(sec);
	if(Cache == NULL)
		return 6;
	memset(Cache , 0 , 2048);

	pdir = (struct _DIR *)Cache;
	memset(pdir, 0, sizeof(struct _DIR));
	memcpy(pdir->Name, dot, 11);
	pdir->Attr = ATTR_DIRECTORY;
	fat_datetime(&tm);
	pdir->CrtDate = pdir->LstAccDate = pdir->WrtDate = tm.Date.Date;
	pdir->CrtTime = pdir->WrtTime = tm.Time.Time;
	pdir->CrtTimeTenth = tm.TimeTenth;
	pdir->FstClusLO = NewCluster;

	pdir++;
	memset(pdir, 0, sizeof(struct _DIR));
	memcpy(pdir->Name, dotdot, 11);
	pdir->Attr = ATTR_DIRECTORY;
	fat_datetime(&tm);
	pdir->CrtDate = pdir->LstAccDate = pdir->WrtDate = tm.Date.Date;
	pdir->CrtTime = pdir->WrtTime = tm.Time.Time;
	pdir->CrtTimeTenth = tm.TimeTenth;
	pdir->FstClusLO = SectorNum2ClusterNum(SectorNum);

	SetSectorData(sec, Cache , Bpb.BytsPerSec*4);
	if(Bpb.SecPerClus>4)
	{
		memset(Cache,0,2112);
		for(int i=4 ; i< Bpb.SecPerClus; i+=4)
			SetSectorData(sec+i+4, Cache , Bpb.BytsPerSec*4);
	}
	Flush();

	return 0;
}

int fat_rmdir( const char *dirname)
{
	DWORD SectorNum;
	struct _file file;
	char filename[13];

	//is dir have no sub dir or file ?
	if(fat_getfirst(dirname, filename) == 0)
		return 3;

	//locate
	SectorNum = fat_locate(dirname, &file);
	if(SectorNum == 0xffffffff)
		return 4;

	// is it a dir ?
	if(!(file.dir.Attr & ATTR_DIRECTORY))
		return 6;

	if(DeleteDir(&file) != 0)
		return 5;

	FreeCluster(file.dir.FstClusLO);

	return 0;
}

struct FatGet gfat_get;

int fat_getfirst(const char *path, char* filename)
{
	DWORD Sector;
	unsigned int i;
	WORD Cluster;


	//if exist this dir ?
	Sector = fat_locate(path, NULL);
	if(Sector == 0xffffffff)
		return 1;


	if(Sector == FirstRootDirSecNum)
	{
		gfat_get.IsRootDir = 1;
		gfat_get.DirIndex = 0;

		for(i=0; i<Bpb.RootEntCnt * sizeof(struct _DIR) / Bpb.BytsPerSec; i++)
		{
			
			if(SectorGet(Sector++, &gfat_get) == 0)
			{
				strcpy(filename, gfat_get.filename);
//CreateWindow("tt", filename, 10, 10, 1, 1) ;
				return 0;
			}
		}
	}

	else
	{
		gfat_get.IsRootDir = 0;
		gfat_get.DirIndex = 0;

		Cluster = SectorNum2ClusterNum(Sector);

		// because the sector is the first sector of parent dir,
		// so it is the first sector of cluster.
		/*
		i = (Sector - FirstDataSector) % Bpb.SecPerClus;
		
		if(i != 0)
		{
			for(; i< Bpb.SecPerClus; i++)
			{
				if(SectorGet(Sector++, &gfat_get) == 0)
				{
					strcpy(filename, gfat_get.filename);
					return 0;
				}
			}

			Cluster = GetNextClusterNum(Cluster);
			Sector = ClusterNum2SectorNum(Cluster);
		}*/
		
		while(Cluster != 0xffff)
		{
			for(i=0; i< Bpb.SecPerClus; i++)
			{
				if(SectorGet(Sector++, &gfat_get) == 0)
				{
					strcpy(filename, gfat_get.filename);
					return 0;
				}
			}

			Cluster = GetNextClusterNum(Cluster);
			Sector = ClusterNum2SectorNum(Cluster);
		}
	}

	return 2;
}

int fat_getnext(char* filename)
{
	DWORD Sector;
	unsigned int i;
	WORD Cluster;

	Sector = gfat_get.DirSectorNum;

	if(gfat_get.IsRootDir)
	{
		i=(Sector - FirstRootDirSecNum) ;

		for(; i<Bpb.RootEntCnt * sizeof(struct _DIR) / Bpb.BytsPerSec; i++)
		{
			if(SectorGet(Sector++, &gfat_get) == 0)
			{
				strcpy(filename, gfat_get.filename);
				return 0;
			}
		}
	}

	else
	{
		Cluster = SectorNum2ClusterNum(Sector);

		i = (Sector - FirstDataSector) % Bpb.SecPerClus;

		if(i != 0)
		{
			for(; i< Bpb.SecPerClus; i++)
			{
				if(SectorGet(Sector++, &gfat_get) == 0)
				{
					strcpy(filename, gfat_get.filename);
					return 0;
				}
			}

			Cluster = GetNextClusterNum(Cluster);
			Sector = ClusterNum2SectorNum(Cluster);
		}

		while(Cluster != 0xffff)
		{
			for(i=0; i< Bpb.SecPerClus; i++)
			{
				if(SectorGet(Sector++, &gfat_get) == 0)
				{
					strcpy(filename, gfat_get.filename);
					return 0;
				}
			}

			Cluster = GetNextClusterNum(Cluster);
			Sector = ClusterNum2SectorNum(Cluster);
		}
	}

	return 2;
}

struct _file handles[16];

int fat_close(int handle)
{
	struct _file *fp;
	struct FatDateTime tm;
	BYTE* Cache;
	struct _DIR *dir;

	if(handle <0 || handle >= (int)(sizeof(handles)/sizeof(struct _file)))
		return -1;

	fp = &handles[handle];

	fat_datetime(&tm);
	fp->dir.LstAccDate = fp->dir.WrtDate = tm.Date.Date;
	fp->dir.WrtTime = tm.Time.Time;

	Cache = GetSectorData(fp->DirSectorNum);
	if(Cache == NULL)
		return -2;

	dir = (struct _DIR *)Cache;
	dir += fp->DirIndex;

	memcpy(dir, &fp->dir, sizeof(struct _DIR));
	SetSectorData(fp->DirSectorNum, Cache , Bpb.BytsPerSec);
	Flush();

	handles[handle].valid = 0;
	return 0;
}

int fat_close_withouwrit(int handle)
{
	struct _file *fp;
	struct FatDateTime tm;
	BYTE* Cache;
	struct _DIR *dir;

	if(handle <0 || handle >= (int)(sizeof(handles)/sizeof(struct _file)))
		return -1;

	fp = &handles[handle];

	fat_datetime(&tm);
	fp->dir.LstAccDate = fp->dir.WrtDate = tm.Date.Date;
	fp->dir.WrtTime = tm.Time.Time;

	Cache = GetSectorData(fp->DirSectorNum);
	if(Cache == NULL)
		return -2;

	dir = (struct _DIR *)Cache;
	dir += fp->DirIndex;

	handles[handle].valid = 0;
	return 0;
}


int fat_creat(const char* filename, BYTE attribute)
{
	struct _DIR dir;
	char path[768];
	char name[256];
	char *p;
	DWORD ParentDirSectorNum;
	struct FatDateTime tm;
	struct _file file;
	struct _DIR *pdir;
	WORD NewCluster;
	BYTE* Cache;

	// is path format correct ?
	if( get_valid_format(filename)<0)
		return -2;

	//if exist this file ?
	if(fat_locate(filename, NULL) != 0xffffffff)
		return -3;

	//separate path into parent and name
	if(strlen(filename)<2)
		return -4 ;
	p = strrchr((char*)filename,'\\');
	strcpy(name,p+1);

	strcpy(path, filename);
	p = strrchr(path, '\\');
	if(p==path)
		*(p+1) = 0 ;
	else
		*p = '\0';

	//locate parent path
	ParentDirSectorNum = fat_locate(path, NULL);
	if(ParentDirSectorNum == 0xffffffff)
		return -4;

	//fill dir attributes
	memset(&dir, 0, sizeof(dir));
	memcpy(dir.Name, name, 11);
	dir.Attr = attribute;
	fat_datetime(&tm);
	dir.CrtDate = dir.LstAccDate = dir.WrtDate = tm.Date.Date;
	dir.CrtTime = dir.WrtTime = tm.Time.Time;
	dir.CrtTimeTenth = tm.TimeTenth;
	dir.FileSize = 0;

	//alloc one dir
	if(AllocDir(ParentDirSectorNum, &dir, &file) != 0)
		return -5;

	//alloc a cluster
	NewCluster = AllocCluster(0);
	if(NewCluster == 0xffff)
		return -6;

	//flush to disk
	Cache = GetSectorData(file.DirSectorNum);
	if(Cache == NULL)
		return -7;

	pdir = (struct _DIR *)Cache;
	pdir += file.DirIndex;

	pdir->FstClusLO = NewCluster;
	SetSectorData(file.DirSectorNum, Cache , Bpb.BytsPerSec);
	Flush();

	return fat_open(filename);
}

long fat_lseek(int handle, long offset, int origin)
{
	struct _file *fp;
	WORD Cluster;
	unsigned int len;
	int i;

	if(handle <0 || handle >= (int)(sizeof(handles)/sizeof(struct _file)))
		return 0;

	fp = &handles[handle];

	switch(origin)
	{
	case SEEK_SET:
		{
			if(offset < 0)
				return -1;

			fp->offset = offset;
		}
		break;

	case SEEK_CUR:
		{
			fp->offset += offset;
		}
		break;

	case SEEK_END:
		{
			fp->offset = fp->dir.FileSize + offset;
		}
		break;

	default:
		return -2;
	}

	// re-locate CurrentSectorNum, SectorOffset
	Cluster = fp->dir.FstClusLO;
	fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
	len = 0;

	while(Cluster != 0xffff)
	{
		for(i=0; i< Bpb.SecPerClus; i++)
		{
			fp->CurrentSectorNum ++;
			len += Bpb.BytsPerSec;

			if(len >= fp->offset)
			{
				fp->SectorOffset = fp->offset % Bpb.BytsPerSec;
				return fp->offset;
			}
			
		}

		Cluster = GetNextClusterNum(Cluster);
		fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
	}

	return handles[handle].offset;
}

int fat_copyfat(const char* filename,unsigned char* pdes)
{
	struct _file * fp = NULL;
	DWORD FirstSectorNum;
	DWORD cluster;
	WORD *pp = (WORD*)pdes;
	int i =0;
	FirstSectorNum = fat_locate(filename, fp);
	if(FirstSectorNum == 0xffffffff)
		return -2;
	cluster = SectorNum2ClusterNum(FirstSectorNum);	
	pp[0] = cluster ;
	i = 1 ;
	do
	{
		cluster = GetNextClusterNum(cluster);
		pp[i] = cluster ;
		i++ ;
		
	}
	while(cluster != 0xFFFF);
	return 0 ;
}

int fat_open(const char* filename)
{
	int i;
	struct _file * fp = NULL;
	DWORD FirstSectorNum;

	for(i=0; i<16; i++)
	{
		if(!handles[i].valid)
		{
			fp = &handles[i];
			break;
		}
	}

	if(fp == NULL)
		return -1;

	FirstSectorNum = fat_locate(filename, fp);
	if(FirstSectorNum == 0xffffffff)
		return -2;

	fp->StartSectorNum = FirstSectorNum;
	fp->CurrentSectorNum = fp->StartSectorNum;
	fp->SectorOffset = 0;
	fp->offset = 0;
	fp->valid = 1;
	return i;
}

unsigned int fat_read(int handle, void* buffer, unsigned int bytes)
{
	BYTE* Cache;
	unsigned int read_bytes =0;
	unsigned int max_copy_bytes_in_sector;
	struct _file *fp;
	WORD Cluster;
	int i;

	if(handle <0 || handle >= (int)(sizeof(handles)/sizeof(struct _file)))
		return 0;

	fp = &handles[handle];
	bytes = (fp->dir.FileSize - fp->offset) > bytes ? bytes : (fp->dir.FileSize - fp->offset);

	Cluster = SectorNum2ClusterNum(fp->CurrentSectorNum);	
	
	i = (fp->CurrentSectorNum - FirstDataSector) % Bpb.SecPerClus;

	if(i != 0)
	{
		for(; i< Bpb.SecPerClus; i++)
		{
			Cache = GetSectorData(fp->CurrentSectorNum);
			if(Cache == NULL)
				return 0;

			Cache += fp->SectorOffset;
			max_copy_bytes_in_sector = (Bpb.BytsPerSec - fp->SectorOffset) > (bytes - read_bytes) ? (bytes - read_bytes) : (Bpb.BytsPerSec - fp->SectorOffset);
			if((max_copy_bytes_in_sector)&&(!(max_copy_bytes_in_sector%4)))
			{
				DmaCopy(3, Cache,buffer, max_copy_bytes_in_sector,32);
			}
			else
				memcpy(buffer, Cache, max_copy_bytes_in_sector);
			
			read_bytes += max_copy_bytes_in_sector;
			fp->SectorOffset += max_copy_bytes_in_sector;
			fp->offset += max_copy_bytes_in_sector;
			buffer = (char*)buffer + max_copy_bytes_in_sector;

			if(fp->SectorOffset == Bpb.BytsPerSec)
			{
				if(i == Bpb.SecPerClus -1)
				{
					Cluster = GetNextClusterNum(Cluster);
					if(Cluster != 0xffff)
						fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
				}
				else
					fp->CurrentSectorNum ++;

				fp->SectorOffset = 0;
			}

			if(read_bytes == bytes)
			{
				return bytes;
			}
		}
	}
	
	while(Cluster != 0xffff)
	{
		for(i=0; i< Bpb.SecPerClus; i++)
		{
			//这里判断读取长度 , 一次读取一个cluster
			if(((bytes - read_bytes) > Bpb.BytsPerSec * Bpb.SecPerClus)&&SDorNand)
			{
				DWORD add = fp->CurrentSectorNum*512 + hidesec*512 ;
				DWORD tlen = Bpb.BytsPerSec * Bpb.SecPerClus ;
				SD_Enable();
				SD_ReadMultiBlock(add,(BYTE*)buffer,tlen) ;
				SD_Disable();

				read_bytes += tlen;
				fp->SectorOffset = 0 ;
				fp->offset += tlen;
				buffer = (char*)buffer + tlen;
				Cluster = GetNextClusterNum(Cluster);
				if(Cluster != 0xffff)
					fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
				i = Bpb.SecPerClus ;
				continue ;
			}
			
		
			Cache = GetSectorData(fp->CurrentSectorNum);

			if(Cache == NULL)
				return 0;

			Cache += fp->SectorOffset;
			max_copy_bytes_in_sector = (Bpb.BytsPerSec - fp->SectorOffset) > (bytes - read_bytes) ? (bytes - read_bytes) : (Bpb.BytsPerSec - fp->SectorOffset);
			if((max_copy_bytes_in_sector)&&(!(max_copy_bytes_in_sector%4)))
			{
				DmaCopy(3, Cache,buffer, max_copy_bytes_in_sector,32);
			}
			else
				memcpy(buffer, Cache, max_copy_bytes_in_sector);
			
			read_bytes += max_copy_bytes_in_sector;
			fp->SectorOffset += max_copy_bytes_in_sector;
			fp->offset += max_copy_bytes_in_sector;
			buffer = (char*)buffer + max_copy_bytes_in_sector;

			if(fp->SectorOffset == Bpb.BytsPerSec)
			{				
				if(i == Bpb.SecPerClus -1)
				{
					Cluster = GetNextClusterNum(Cluster);
					if(Cluster != 0xffff)
						fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
				}
				else
					fp->CurrentSectorNum ++;
				fp->SectorOffset = 0;
			}

			if(read_bytes == bytes)
			{
				return bytes;
			}
		}		
	}

	return 0;
}

unsigned int fat_write(int handle, const char* buffer, unsigned int bytes)
{
	BYTE* Cache;
	unsigned int write_bytes =0;
	unsigned int max_write_bytes_in_sector;
	struct _file *fp;
	int Cluster;
	int PrevCluster;
	int i;

	if(handle <0 || handle >= (int)(sizeof(handles)/sizeof(struct _file)))
		return 0;

	fp = &handles[handle];

	Cluster = SectorNum2ClusterNum(fp->CurrentSectorNum);
	PrevCluster = Cluster;

	i = (fp->CurrentSectorNum - FirstDataSector) % Bpb.SecPerClus;

	if(i != 0)
	{
		for(; i< Bpb.SecPerClus; i++)
		{
			Cache = GetSectorData(fp->CurrentSectorNum);
			if(Cache == NULL)
				return 0;

			Cache += fp->SectorOffset;
			max_write_bytes_in_sector = ((Bpb.BytsPerSec - fp->SectorOffset) > (bytes - write_bytes)) ? (bytes - write_bytes) : (Bpb.BytsPerSec - fp->SectorOffset);
			//memcpy(Cache, buffer, max_write_bytes_in_sector);
			if((max_write_bytes_in_sector)&&(!(max_write_bytes_in_sector%4)))
			{
				DmaCopy(3,buffer, Cache, max_write_bytes_in_sector,32);
			}
			else
				memcpy(Cache, buffer, max_write_bytes_in_sector);
			
			SetSectorData(fp->CurrentSectorNum, Cache , Bpb.BytsPerSec);
			//cacel by yafei
			//Flush();

			write_bytes += max_write_bytes_in_sector;
			fp->SectorOffset += max_write_bytes_in_sector;
			fp->offset += max_write_bytes_in_sector;
			buffer = (char*)buffer + max_write_bytes_in_sector;

			if(fp->offset>=fp->dir.FileSize)
				fp->dir.FileSize = fp->offset ;			
			//cancel by yafei
			//fp->dir.FileSize +=  max_write_bytes_in_sector;			

			if(fp->SectorOffset == Bpb.BytsPerSec)
			{
				if(i == Bpb.SecPerClus -1)
				{
					PrevCluster = Cluster;
					Cluster = GetNextClusterNum(Cluster);
					if(Cluster != 0xffff)
						fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
					else
					{
						Cluster = AllocCluster(PrevCluster);
						if(Cluster == 0xffff)
						{
							//flush add by yafei
							Flush();
							return 0;
						}
						fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
					}
				}
				else
					fp->CurrentSectorNum ++;

				fp->SectorOffset = 0;
			}

			if(write_bytes == bytes)
			{
				//flush add by yafei
				Flush();
				return bytes;
			}
		}
	}

	for(;;)
	{
		for(i=0; i< Bpb.SecPerClus; i++)
		{
			Cache = GetSectorData(fp->CurrentSectorNum);
			if(Cache == NULL)
				return 0;

			Cache += fp->SectorOffset;
			max_write_bytes_in_sector = (Bpb.BytsPerSec - fp->SectorOffset) > (bytes - write_bytes) ? (bytes - write_bytes) : (Bpb.BytsPerSec - fp->SectorOffset);
			if((max_write_bytes_in_sector)&&(!(max_write_bytes_in_sector%4)))
			{
				DmaCopy(3,buffer, Cache, max_write_bytes_in_sector,32);
			}
			else
				memcpy(Cache, buffer, max_write_bytes_in_sector);
			SetSectorData(fp->CurrentSectorNum, Cache , Bpb.BytsPerSec);
			//cancel by yafei
			//Flush();

			write_bytes += max_write_bytes_in_sector;
			fp->SectorOffset += max_write_bytes_in_sector;
			fp->offset += max_write_bytes_in_sector;
			buffer = (char*)buffer + max_write_bytes_in_sector;
			
			if(fp->offset>=fp->dir.FileSize)
				fp->dir.FileSize = fp->offset ;			
			//cancel by yafei
			//fp->dir.FileSize +=  max_write_bytes_in_sector;

			if(fp->SectorOffset == Bpb.BytsPerSec)
			{
				if(i == Bpb.SecPerClus -1)
				{
					PrevCluster = Cluster;
					Cluster = GetNextClusterNum(Cluster);
					if(Cluster != 0xffff)
						fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
					else
					{
						Cluster = AllocCluster(PrevCluster);
						if(Cluster == 0xffff)
						{
							Flush();
							return 0;
						}
						fp->CurrentSectorNum = ClusterNum2SectorNum(Cluster);
					}
				}
				else
					fp->CurrentSectorNum ++;

				fp->SectorOffset = 0;
			}

			if(write_bytes == bytes)
			{
				Flush();
				return bytes;
			}
		}
	}

	// we can not reach here.
	return 0;
}

int fat_remove( const char *filename)
{
	DWORD SectorNum;
	struct _file file;

	//locate
	SectorNum = fat_locate(filename, &file);
	if(SectorNum == 0xffffffff)
		return 4;

	// is it a dir ?
	if(file.dir.Attr & ATTR_DIRECTORY)
		return 6;

	if(DeleteDir(&file) != 0)
		return 5;

	FreeCluster(file.dir.FstClusLO);

	return 0;
}

int fat_get_stat( const char *filename, struct _stat* stat)
{
	DWORD SectorNum;
	struct _file file;

	//locate
	SectorNum = fat_locate(filename, &file);
	if(SectorNum == 0xffffffff)
		return 1;

	stat->Attr = file.dir.Attr;
	stat->CrtDate = file.dir.CrtDate;
	stat->CrtTime = file.dir.CrtTime;
	stat->CrtTimeTenth = file.dir.CrtTimeTenth;
	stat->FileSize = file.dir.FileSize;
	stat->LstAccDate = file.dir.LstAccDate;
	stat->WrtDate = file.dir.WrtDate;
	stat->WrtTime = file.dir.WrtTime;

	return 0;
}

int fat_set_stat( const char *filename, struct _stat* stat)
{
	DWORD SectorNum;
	struct _file file;
	BYTE* Cache;
	struct _DIR *dir;

	//locate
	SectorNum = fat_locate(filename, &file);
	if(SectorNum == 0xffffffff)
		return 1;

	file.dir.Attr = stat->Attr;
	file.dir.CrtDate = stat->CrtDate;
	file.dir.CrtTime = stat->CrtTime;
	file.dir.CrtTimeTenth = stat->CrtTimeTenth;
	file.dir.FileSize = stat->FileSize;
	file.dir.LstAccDate = stat->LstAccDate;
	file.dir.WrtDate = stat->WrtDate;
	file.dir.WrtTime = stat->WrtTime;

	Cache = GetSectorData(file.DirSectorNum);
	if(Cache == NULL)
		return 2;

	dir = (struct _DIR *)Cache;
	dir += file.DirIndex;

	memcpy(dir, &file.dir, sizeof(struct _DIR));
	SetSectorData(file.DirSectorNum, Cache , Bpb.BytsPerSec);
	Flush();

	return 0;
}


int fat_rename( const char *oldname, const char *newname )
{
	struct _DIR dir;
	char path[512];
	char newpath[512];
	char name[11];
	char new_name[11];
	char *p;
	DWORD ParentDirSectorNum;
	struct _file old_file;

	//
	//check oldname file
	//

	// is path format correct ?
	if(get_valid_format(oldname)<0)
		return -2;


	//if exist this file ?
	if(fat_locate(oldname, &old_file) == 0xffffffff)
		return -3;

//处理8+3的文件名
	p = toDosDirname(oldname);
	if(p == NULL)
		return 1;
	
	//separate path into parent and name
	strncpy(name, &p[strlen(p)-11], 11);

	strcpy(path, oldname);
	p = strrchr(path, '\\');
	*p = '\0';


	//
	//check newname file
	//

	if(strchr(newname, '\\') != NULL)
		return -2;

	sprintf(newpath, "%s\\%s", path, newname);

	// is path format correct ?
	if( get_valid_format(newpath)<0)
		return -2;

	//if exist this file ?
	if(fat_locate(newpath, NULL) != 0xffffffff)
		return -3;

	//separate path into parent and name
	strncpy(new_name, &p[strlen(p)-11], 11);



	//locate parent path
	ParentDirSectorNum = fat_locate(path, NULL);
	if(ParentDirSectorNum == 0xffffffff)
		return -4;

	//fill dir attributes
	memcpy(&dir, &old_file.dir, sizeof(struct _DIR));
	memcpy(dir.Name, new_name, 11);

	//alloc one dir
	if(AllocDir(ParentDirSectorNum, &dir, NULL) != 0)
		return -5;

	//delete old one
	if(DeleteDir(&old_file) != 0)
		return -6;

	return 0;
}
