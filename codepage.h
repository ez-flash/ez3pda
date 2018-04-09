#ifndef __CODEPAGE_H
#define __CODEPAGE_H

typedef unsigned short _wchar_t ;

int uni2char(_wchar_t uni,unsigned char *out, int boundlen) ;
int char2uni(unsigned char *rawstring, int boundlen,
			_wchar_t *uni) ;


#endif
