
#ifndef GLOBAL_H
#define GLOBAL_H

//定义的Ez3的一些配置。
#define	_Ez3PsRAM 			0x08400000
#define	_Ez3NorList			0x083c0000
#define _Ez3NorRom			0x09400000
#define _Ez3NorRomEnd		0x09C00000
#define	_UnusedVram 		0x06012c00
#define	_UnusedVramSize 	0x00005400
#define _UnusedEram			0x02013c00
#define _ramFatCache		0x02000000
#define _ramFatBlockCache	0x02020000
#define _psFatCache			(_Ez3PsRAM+0xC00000)
#define _psFatBlockCache	(_Ez3PsRAM+0xC20000)
#define _Ez3NorCNName       0x08028000			//中文列表名称
#define _Ez4NorCNName		0x083E0000
#define MAXDISKNAME			32
#define _GoldenEnable		0x0E008400
#define _GoldenSaver		0x0E008200
#define _FileInfoSaver		0x0E008000
#define _SRAMSaver		0x0E000000

#define _txtBaseSaver		0x0E008404
#define _txtSaver1			0x0E008404
#define _txtSaver2			0x0E008504
#define _txtSaver3			0x0E008604
#define _txtSaver4			0x0E008704


//用于Nor中的文件列表
typedef struct FM_NOR_FILE_SECT{	////文件管理器的文件显示列表单元数据结构
	char filename[256];
	u16 rompage ;
	u16 saversize ;
	u16	savertype;
	u16 reserved ;
	u32 filesize;
	u32 reserved2 ;
} FM_NOR_FS;

//
#define GAMEPAKMem	   (u8*) 0x8000000
#define GAMEPAKRES	GAMEPAKMem  //图像资源位置
#define GBAPAKLINE	(GAMEPAKMem + 256*1024)  //字库和文件资源位置

#define SAVEPOINT	0xE000000				//存盘点
#define SAVEHEADVAL	0x1616
#define SAVEHEAD	SAVEPOINT
#define SAVEISSET	(SAVEPOINT+4)
#define SAVETXTPOS	(SAVEPOINT+8)

#define WORDSAVEPOINT	0x800				//单词学习存盘点


#define MAX_PATH	260
#define HEADSIZE	MAX_PATH+12

//定义窗口颜色属性

#define FM_CL_BG			RGB(31,31,31)		//一般背景色,白
#define FM_CL_TITLE			RGB(8,16,25)			//窗口标题栏,蓝色


#define FM_CL_WHITE			RGB(31,31,31)		//一般背景色,白
#define FM_CL_BLACK			RGB(3,0,0)			//黑色
#define FM_CL_HIGHBLUE		RGB(19,23,27)			//亮蓝
#define FM_CL_BLUE			RGB(4,8,16)			//深蓝
#define FM_CL_HIGHGRAY		RGB(28,28,28)			//亮灰
#define FM_CL_BLUEGRAY		RGB(12,20,22)			//蓝灰
#define FM_CL_GRAY			RGB(23,23,23)			//灰色
#define FM_CL_DUKGRAY		RGB(17,17,17)			//深灰
#define FM_CL_YELLOW		RGB(31,31,8)			//黄色
#define FM_CL_RED			RGB(31,1,1)			//红色

//时间设定的背景
#define TM_CL_BG            0x3F9F

//桌面当前处理类型
#define DESKTOP_FLAG_NULL		0
#define DESKTOP_FLAG_CREAT		1
#define DESKTOP_FLAG_UPD		2
#define DESKTOP_FLAG_OVER		9



//文件浏览器当前处理类型
#define SHELL_FLAG_NULL			0			//无任务
#define SHELL_FLAG_CREATE		1			//新创建
#define SHELL_FLAG_RESHOW		2			//其它程序返回到浏览器
#define SHELL_FLAG_SCR			3			//浏览器翻屏
#define SHELL_FLAG_CHG			4			//浏览器更换文件焦点

//TXT阅读器当前处理类型
#define TXT_FLAG_NULL			0			//无任务
#define TXT_FLAG_CREATE			1			//新创建
#define TXT_FLAG_RESHOW			2			//其它程序返回到txt
#define TXT_FLAG_SCR			3			//TXT翻屏

#define TXT_ROW_H			13			//TXT浏览器的温饱行高
#define TXT_MSN				12			//TXT浏览器的每页显示行数
#define TXT_L				1			//TXT浏览器的文字区域左边界
#define TXT_W				230			//TXT浏览器的文字区域宽度
#define TXT_T				2			//TXT浏览器的文字区域上边界
#define TXT_H				156			//TXT浏览器的文字区域高度

//JPG阅读器当前处理类型
#define JPG_FLAG_NULL			0			//无任务
#define JPG_FLAG_CREATE			1			//新创建


#define FM_TITLE_H			23			//窗口标题栏高
#define FM_FOOT_H			13			//窗口状态栏高
#define FM_SCR_W			6			//窗口拖动条宽

#define FM_ROW_H			17			//浏览器文件行高
#define FM_F_NAME_X			20			//浏览器文件名显示X
#define FM_F_NAME_W			138			//浏览器文件名显示宽度
#define FM_F_SIZE_X2		216			//浏览器文件大小显示位置
#define FM_F_SIZE_W			80			//浏览器文件大小显示宽度
#define FM_FILE_MAX_SHOW_NUM	8
#define FM_MSN	FM_FILE_MAX_SHOW_NUM		//浏览器最多同时显示个数


//显示文件浏览器时，选择刷新哪一部分
#define FM_MD_NULL			0x0			//无
#define FM_MD_HEAD			0x1			//刷新浏览器头
#define FM_MD_FOOT			0x2			//刷新浏览器尾
#define FM_MD_FILES			0x4			//刷新文件区
#define FM_MD_SCR			0x8			//刷新拖动条
#define FM_MD_FOCUS			0x10		//刷新选中文件
#define FM_MD_ALL			FM_MD_HEAD|FM_MD_FOOT|FM_MD_FILES|FM_MD_SCR|FM_MD_FOCUS		//刷新全部
//显示TXT文件时，选择刷新哪一部分
#define FM_TXT_NULL			0x0			//无
#define FM_TXT_LINES			0x1			//刷新全屏文本行
#define FM_TXT_SPCLINE			0x2			//刷新特殊行（刷新的行）




typedef struct FM_MD_FILE_SECT{	////文件管理器的文件显示列表单元数据结构
	char filename[64];
	u8 isDir;
	u32 filesize;
} FM_MD_FS;
typedef struct FM_MD_FILE{		//文件管理器的文件显示列表数据结构
	u16 sectnum;
	FM_MD_FILE_SECT sect[FM_MSN];
}FM_MD_F;

typedef struct FILE_SAVE_INFO{	//该结构体定义保存用户阅读存盘点的信息
	u32 flg ;
	u16 index;
	char path[128];
	char fname[64];
	u16 fi;
	u16 viewfi;
	u16 pos;
	u32 pos2;
}FILE_SI;


typedef struct res_Struct
{
	u8 *res_ZKDATA;		//汉字库
	u8 *res_ASCDATA;	//ASC字库
	u16 *res_DESKBG;	//桌面背景
	u16 *res_DESKICON;	//桌面图标
	u16 *res_FILEICON;	//文件图标
	u16 *res_FILEBG;	//文件管理器背景
	u16 *res_FILEBGHEAD;//文件管理器背景头
	u16 *res_TXTBG;		//记事本背景
	u16 *res_TXTSCUP;	//记事本滑条上
	u16 *res_TXTSCDOWN;	//记事本滑条下
	u16 *res_TXTSCSIGN;	//记事本滑块
	u16 *res_TXTSCBACK;	//记事本滑条背景
}res_struct;



#endif



