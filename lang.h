#ifndef _LANG_H
#define _LANG_H

extern char*	gl_warning ;
extern char*	gl_tip ;
extern char*	gl_lskip ;
extern char*	gl_faterror ;
extern char*	gl_nofile ;
extern char*	gl_unkownType ;
extern char*	gl_waitread ;
extern char*	gl_fileerror ;
extern char*	gl_showprog ;
extern char*	gl_op_error ;
extern char*	gl_golderror ;
extern char*	gl_readsaver ;
extern char*	gl_saverfile ;
extern char*	gl_waiting ;
extern char*	gl_writing ;
extern char*	gl_firstin ;
extern char*    gl_nosaver;
extern char*    gl_flashwrite ; 
extern char* gl_checkingsaver;

/*********blowfish add for NOR OP**********/
extern char*   	g1_norDelete;
extern char*   	g1_norSpace;
extern char*   	g1_norWrite;
extern char*	g1_norErasing;
extern char*   	g1_norWriting;
extern char*	gl_norformat;
extern char* 	gl_psramout;
extern char*	gl_nosupport;
/*********blowfish add for NOR OP**********/
extern char**  	gl_hint_content;
extern char**  	gl_hint_title;
extern char**   gl_option_set ;


void LoadEnglish();
void LoadChinese();

#endif

