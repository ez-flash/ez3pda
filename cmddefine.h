#ifndef	CMDDEFINE_HEADER
#define CMDDEFINE_HEADER

#define CMD_NULL		0x0
#define CMD_SHOWFILE	0x1
#define CMD_CLEAR_CLIENT	0x2
#define CMD_SELECT		0x3

#define PROC_DIR_VIEW		0x0
#define PROC_GBA_VIEW		0x1
#define PROC_MID_VIEW		0x3
#define PROC_TXT_VIEW		0x4
#define PROC_UNKNOWN		0x5
#define PROC_JPG_VIEW		0x6
#define PROC_HTM_VIEW		0xB
#define PROC_C_VIEW			0xC
#define PROC_H_VIEW			0xD

#define PROC_ZIPGBA_VIEW 	0xE
#define PROC_SAVER_VIEW 	0x9

#define PROC_ENTERDISKSHELL	0x101	//nand 和 Nor文件列表选择都在此
#define PROC_SOFTRESET		0x102
#define PROC_HARDRESET		0x103
#define PROC_GOLDENSELECT	0X104
#define PROC_EZWORD			0x105
#define PROC_LOADSAVER     	 0x106
#define PROC_WRITESAVEREX	0x107
#define PROC_SELECT_LANG		0x108
#define PROC_WRITE2NORFLASH		0x109
#define PROC_FORMATNORFLASH		0x110

#define PROC_SHELL			0x1
#define PROC_DESKTOP		0x5
#define EXEC_TXT			0x5A1C
#endif

