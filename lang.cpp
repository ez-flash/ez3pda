#include "lang.h"

char*	gl_warning ;
char*	gl_tip ;
char*	gl_lskip ;
char*	gl_faterror ;
char*	gl_nofile ;
char*	gl_unkownType ;
char*	gl_waitread ;
char*	gl_fileerror ;
char*	gl_showprog ;
char*	gl_op_error ;
char*	gl_golderror ;
char*	gl_readsaver ;
char*	gl_saverfile ;
char*	gl_waiting ;
char*	gl_writing ;
char*	gl_firstin ;
char*   gl_nosaver;
char*  gl_flashwrite ;
char* gl_checkingsaver;
/*********blowfish add for NOR OP**********/
char*   g1_norDelete;
char*   g1_norSpace;
char*   g1_norWrite;
char*	g1_norErasing;
char*   g1_norWriting;
char*	gl_norformat;
char*   gl_psramout;
char*	gl_nosupport;
/*********blowfish add for NOR OP**********/
char**  gl_hint_content;
char**  gl_hint_title;
char**  gl_option_set ;

#define ICON_COUNT	7
#define OPTION_CNT		2
extern const char *hint_content[ICON_COUNT] ;
extern const char *hint_content_en[ICON_COUNT] ;
extern const char *hint_title[ICON_COUNT] ;
extern const char *hint_title_en[ICON_COUNT] ;
extern const char *option_set_ch[OPTION_CNT] ;
extern const char *option_set_en[OPTION_CNT] ;


//中文
const char zh_warning[]="警告";
const char zh_tip[]="提示";
const char zh_lskip[]="长按L跳过存档备份";
const char zh_faterror[]="FAT文件系统出错!";
const char zh_nofile[]="无此文件!";
const char zh_unkownType[]="未知文件类型,打开该文件Y/N";
const char zh_waitread[]="请等待文档的读取";
const char zh_fileerror[]="文件格式错误";
const char zh_showprog[]="当前进度显示";
const char zh_op_error[]="未知处理返回";
const char zh_golderror[]="金手指出错!继续=A 返回B";
const char zh_readsaver[]="读取存档,原存档被覆盖Y/N";
const char zh_saverfile[]="存档文件";
const char zh_waiting[]="等待";
const char zh_writing[]="写入";
const char zh_firstin[]="掉电或者系统刚刚进入";
const char zh_nosaver[]="对不起，您先前没有设置存盘点!";
const char zh_writeflash[]="将把此256M游戏写入Flash,Y/N";
const char zh_norDelete[]="删除Y/N";
const char zh_norFormat[]="格式化Flash区";
const char zh_psramout[]="没有足够的PSRAM空间";
const char zh_nosupport[]="此卡不支持GBA游戏";
const char zh_norSpace[]="没有足够的FLASH空间";
const char zh_norWrite[]="写入NOR Flash Y/N";
const char zh_norErasing[]="正在擦除";
const char zh_norWriting[]="正在写";
const char zh_checkingsaver[] = "检查存档" ;

//英文
const char en_warning[]="WARNING";
const char en_tip[]="TIP";
const char en_lskip[]="Hold L to skip Saving";
const char en_faterror[]="FAT Filesystem Error";
const char en_nofile[]="No such File";
const char en_unkownType[]="Unknown type Open or not ?";
const char en_waitread[]="Waiting for reading";
const char en_fileerror[]="File format error";
const char en_showprog[]="progressing ";
const char en_op_error[]="Unkonwn operation";
const char en_golderror[]="Cheat error!continue=A returnB";
const char en_readsaver[]="Read saver to current";
const char en_saverfile[]="Save file";
const char en_waiting[]="waiting";
const char en_writing[]="writing";
const char en_firstin[]="Battery is dry or first in";
const char en_nosaver[]="sorry,no saver has been saved";
const char en_writeflash[]="A 256MROM be writen to Flash,Y/N";
const char en_norDelete[]="Delete Y/N";
const char en_norFormat[]="Format Nor Flash Y/N"; 
const char en_psramout[]="PSRAM is not enough";
const char en_norSpace[]="NOR Space is not Enough";
const char en_nosupport[]="This card do not support GBA game";
const char en_norWrite[]="Write NOR Flash Y/N";
const char en_norErasing[]="Erasing...";
const char en_norWriting[]="Writing...";
const char en_checkingsaver[] = "Checking Saver" ;
/*
//法文
const char en_warning[]="ATTENTION";
const char en_tip[]="ASTUCE";
const char en_lskip[]="Enfoncer L pour ne pas sauvegarder";
const char en_faterror[]="Erreur systeme de fichier FATr";
const char en_nofile[]="Fichier introuvable";
const char en_unkownType[]="Type inconnu Ouvrir ou non?";
const char en_waitread[]="Lecture en cours";
const char en_fileerror[]="Erreur format de fichier";
const char en_showprog[]="en cours ";
const char en_op_error[]="Operation inconnue";
const char en_golderror[]="Erreur cheat!Continuer=A Retour=B";
const char en_readsaver[]="Charger slot1";
const char en_saverfile[]="Sauvegarder";
const char en_waiting[]="En attente";
const char en_writing[]="ecriture";
const char en_firstin[]="Batterie neuve ou HS";
const char en_nosaver[]="Desole rien n'a ete sauvegarde";
*/

void LoadChinese()
{
	gl_warning = (char*)zh_warning ;
	gl_tip = (char*)zh_tip ;
	gl_lskip = (char*)zh_lskip ;
	gl_faterror = (char*)zh_faterror ;
	gl_nofile = (char*)zh_nofile;
	gl_unkownType = (char*)zh_unkownType ;
	gl_waitread = (char*)zh_waitread ;
	gl_fileerror = (char*)zh_fileerror ;
	gl_showprog = (char*)zh_showprog;
	gl_op_error = (char*)zh_op_error ; 
	gl_golderror = (char*)zh_golderror ;
	gl_readsaver = (char*)zh_readsaver ;
	gl_saverfile = (char*)zh_saverfile ;
	gl_waiting = (char*)zh_waiting ;
	gl_writing = (char*)zh_writing ;
	gl_firstin = (char*)zh_firstin ;
	gl_nosaver = (char*)zh_nosaver;
	gl_flashwrite = (char*)zh_writeflash;
	
	g1_norDelete = (char *)zh_norDelete;
	gl_norformat = (char *)zh_norFormat;
	gl_psramout = (char *)zh_psramout;
	gl_nosupport = (char *)zh_nosupport;
	g1_norSpace = (char *)zh_norSpace;
	g1_norWrite = (char *)zh_norWrite;
	g1_norErasing = (char *)zh_norErasing;
	g1_norWriting = (char *)zh_norWriting;
	gl_checkingsaver = (char*)zh_checkingsaver ;
	
	gl_hint_title = (char**)hint_title ;
	gl_hint_content = (char**)hint_content;
	gl_option_set = (char**)option_set_ch ;
}

void LoadEnglish()
{
	gl_warning = (char*)en_warning ;
	gl_tip = (char*)en_tip ;
	gl_lskip = (char*)en_lskip ;
	gl_faterror = (char*)en_faterror ;
	gl_nofile = (char*)en_nofile;
	gl_unkownType = (char*)en_unkownType ;
	gl_waitread = (char*)en_waitread ;
	gl_fileerror = (char*)en_fileerror ;
	gl_showprog = (char*)en_showprog;
	gl_op_error = (char*)en_op_error ;
	gl_golderror = (char*)en_golderror ;
	gl_readsaver = (char*)en_readsaver ;
	gl_saverfile = (char*)en_saverfile ;
	gl_waiting = (char*)en_waiting ;
	gl_writing = (char*)en_writing ;
	gl_firstin = (char*)en_firstin ;
	gl_nosaver = (char*)en_nosaver;
	gl_flashwrite = (char*)en_writeflash;

	g1_norDelete = (char *)en_norDelete;
	gl_norformat = (char *)en_norFormat;
	gl_psramout = (char *)en_psramout;
	gl_nosupport = (char *)en_nosupport;
	g1_norSpace = (char *)en_norSpace;
	g1_norWrite = (char *)en_norWrite;
	g1_norErasing = (char *)en_norErasing;
	g1_norWriting = (char *)en_norWriting;
	gl_checkingsaver = (char*)en_checkingsaver ;
	
	gl_hint_title = (char**)hint_title_en ;
	gl_hint_content = (char**)hint_content_en;
	gl_option_set = (char**)option_set_en ;
}
