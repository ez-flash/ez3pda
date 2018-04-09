#define ICON0_X		0
#define ICON0_Y		0
#define ICON_SPACE	29
#define ICON_BOX_X		80
#define ICON_BOX_Y		70

#define ICON_COUNT	5
#define OPTION_CNT		2

#define ICON_FLAG_X		118
#define ICON_FLAG_Y		8

#define ICON_FLAG_YCOUNT	5
#define DESK_ICON_X0	8
#define DESK_ICON_Y0	6
#define DESK_ICON_DX	36
#define DESK_ICON_DY	29

const char *hint_title[ICON_COUNT] = {
	"我的电脑",
	"存档链接",
//	"英语学习",
	"扫雷游戏",
//	"砖块游戏",
	"设置",
	"帮助"
};
const char *hint_title_en[ICON_COUNT] = {
	"My GBA",
	"Txt Saver",
//	"EZWord",
	"Mines",
//	"Appbrick",
	"Setting",
	"Help",
/*
	"My GBA",
	"Txt Saver",
	"EZWord",
	"Mines",
//	"Appbrick",
	"Regler",
	"Aide",
*/

};

const char *hint_content[ICON_COUNT] = {
	"启动文件管理器, 浏览阅读文件",
	"继续浏览上次保存书签的文档",
//	"英语单词查询、学习、背记",
	"扫雷大战",
//	"打砖块游戏",
	"基本设置",
	"在这里获得更多帮助信息",
};

const char *hint_content_en[ICON_COUNT] = {
	"Starting the file manager",
	"Continue the last Txt File",
//	"studying English Word ",
	"mines By Ivan Mackintosh",
//	"Appbrik, a small funny game",
	"Setting time and language",
	"More help in here",
/*
	"Demarrer l'explorateur de fichiers",
	"Continuer le texte en cours",
	"Etudier l'anglais",
	"Démineur de Ivan Mackintosh",
	"Parametrer l'heure et la langue",
	"Plus d'aide",
*/
};

const char *option_set_ch[OPTION_CNT]={
	"语言设定",
	"存档方式",
};
const char *option_set_en[OPTION_CNT] = {
/*
	"Regler l'heure",
	"Regler le language",
*/
	"Language Setting",
	"Saver Manner",
};
unsigned short desk_icon_list[ICON_COUNT] = {
	0,1,8,7,4,
};

