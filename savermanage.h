#ifndef _savermanage_h
#define _savermanage_h

#include "agb.h"

//******************************************************************************
//**** EZ3读存档文件
void CopybackSaver(u8* pCpbuf,u32 size,u32 startpage);
//******************************************************************************
//**** EZ3读存档文件
u8 CheckSram() ;
u8 Checkaladdin();
u16 CheckLanguage();
//******************************************************************************
//**** EZ3内存读写
void ReadSram(u32 address, u8* data , u32 size );
void WriteSram(u32 address, u8* data , u32 size );
//******************************************************************************
//**** 输入一个文件名，声称存档文件名
void FormatSaverName(char* inname1 , char* outname,int numb);
//******************************************************************************
//**** 写入当前游戏信息
void WriteSaverInfo(char* inname,int saversize,u8 where);
//******************************************************************************
//**** 将当前游戏写入存档
int SaveCurrentSaverToFlash();
//******************************************************************************
//**** 读存档文件
int ReadFileToSram(char* name);
//******************************************************************************
//**** 清除文档
void ClearSaver(int size) ;
//******************************************************************************
//**** 进入存档读取存储界面
int EnterSaver(u8 mode,int sel) ;

/*
typedef struct _SAVERINFO {
char flag[32] ;
char md5code[16];
u8 isSaved ;
u8 where ;
u32 gamesize ;
u16 saversize ;
u8 reserved[7];
char gamename[176];
} SAVEFILEINFO ;
*/
typedef struct _SAVERINFO {
char flag[64] ; //整个游戏的合法标示
char md5code[16];
u32 gamesize ;	//游戏大小
u8 isSaved ;
u8 where ;	//Nand游戏还是Nor游戏
u16 saversize ; //存档大小
u16 language ;
u8 saverforce ;
u8 reserved[5]; 
char gamename[256]; //游戏名
char savername[80]; //存档名
} SAVEFILEINFO ;

#endif //_savermanage_h
