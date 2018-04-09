#ifndef	SHELL_HEADER
#define SHELL_HEADER

#include "AGB.h"
#include "global.h"

typedef struct
{
	char FileName[MAX_PATH];
	unsigned int isDir;
	unsigned int size;
	unsigned int padsize;
} RomFile;


//******************************************************************************
//**** 此目录结构是norFlash中的结构，写在分区的最后2M位置,注意
//**** 
typedef struct
{
	char FileName[MAX_PATH-4];
	unsigned int filetype ;
	unsigned int rompage; //位置
	unsigned int size;
	unsigned int saversize;
} NorRomFile;

//******************************************************************************
//**** 返回一个路径中的斜杠数目
u16 getDirLayerNum(char* path);

//******************************************************************************
//**** 返回一个路径中的文件名
void getDirFileName(char* path, char* filename);

//******************************************************************************
//**** 返回当前目录中ROM文件总数
u16 GetDirFileCount(char *path);

//******************************************************************************
//**** 根据文件索引号，在指定DIR的ROM中检索某文件，返回该文件地址
u8* GetCurRomFile(char *path,u16 index,u8 isgoon);

//******************************************************************************
//**** 根据文件名和路径，确定并返回该文件指针
u8* GetRomFileByName(char *path, char *Name);

//******************************************************************************
//**** 文件管理器主函数
//int runShell(void);

//******************************************************************************
//**** EZ3磁盘Shell主函数
int runEz3DiskShell(void);
	int  GetFileListFromNor(void);
	u32  EnterWriteGBAtoNor() ;
	u32 FormatNor();
//******************************************************************************
//**** 进度条处理函数
void StartProgress();
void StopProgress();
//******************************************************************************
//**** EZ3金手指项目选择函数。
int EnterCheatCodeSelect();
//******************************************************************************
//**** EZ3软复位函数。
void softrest(void);
void hardrest(void);
//******************************************************************************
//**** EZ3时钟显示功能
void ShowTime();
//******************************************************************************
//**** EZ3加载存档
int LoadSaver();
int WriteSaverEx();
//******************************************************************************
//***** 弹出菜单项
u16 popupmenu();

void WaitingErase();

//******************************************************************************
//**** 显示菜单函数
//**** 指定背景图片为bg，标题图片为titleBG，输入菜单字符串数组mnu，指定菜单项数量count及默认焦点defualtI
//int ShowWin_Menu(char *bg, char *titleBG, char** mnu, u8 count, u8 defaultI);
int ShowWin_Menu(char *bg, char *titleBG,char *overlap, char** mnu, u8 count, u8 defaultI);

#endif
