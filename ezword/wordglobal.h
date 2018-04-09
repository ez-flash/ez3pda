
#ifndef WORDGLOBAL_H
#define WORDGLOBAL_H

#define RUN_ENV_REAL			//GBA运行,如屏蔽后则为模拟器运行
#define	NO_LOGO_DBG				//用于调试状态，不显示LOGO

#define PARKFLAG				"EZPDA"
#define WORDLEN		32



/////////////////////////////////////////////////////////////////////////////////
//
//	OLD_APP_文件结构
//
//  ORG			INT			HZK			INT			headsect1		ciku1		trans1		headsect2		ciku1		trans2	... ...	
//	原始数据	HZKSIZE		汉字库数据	词库数目	词库1头结构		词库1		词库1翻译	词库1头结构		词库1		词库1翻译
//

//
//	NEW_LIB_文件结构
//
//  ORG			INT			headsect1		ciku1		trans1		headsect2		ciku1		trans2	... ...	
//	文件系统	词库数目	词库1头结构		词库1		词库1翻译	词库1头结构		词库1		词库1翻译
//



typedef struct headsect{
	u16 wordnum;	
	u16 wordnum2;	
	unsigned int translen;
	unsigned int reserved2;
	unsigned int reserved3;
} ;

typedef struct wordsect{
	char word[WORDLEN];
	unsigned int size;
	unsigned int address;
} ;
typedef struct softhead{
	char data1[256];
	char data2[256];
	char data3[256];
	char data4[256];
} ;

#endif

