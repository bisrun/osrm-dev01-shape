//-----------------------------------------------------------------------------
//	NvStringParse.h
//		$Generate	: sbhuh
//		$Update : 2008.05.08
//-----------------------------------------------------------------------------
#ifndef _STR_PARSE_
#define _STR_PARSE_

#include <string>
#include <vector>
using		namespace		std;
char *read_number(char *p, unsigned char *number, char *sep = NULL);
char *read_number(char *p, unsigned short *number, char *sep = NULL);
char *read_number(char *p, int *number, char *sep = NULL);
char *read_hex(char *p, unsigned short *number, char *sep = NULL);
char *read_number(char *p, short *number, char *sep = NULL);
char *read_number(char *p, unsigned long *number, char *sep = NULL);
char *read_number(char *p, unsigned int *number, char *sep = NULL);
char *read_number(char *p, long *number, char *sep = NULL);
char *read_string(char *p, char *str, char *sep = NULL);
char *read_nstring(char *p, char *str, int len, char *sep = NULL);
char *read_string_ftrim(char *p, char *str, char *sep = NULL);
char *read_float(char *p, float *number, char *sep = NULL);
char *read_double(char *p, double *number, char *sep = NULL);
char *read_double_ftrim(char *p, double *number, char *sep = NULL);
char *read_skip(char *p, char *sep = NULL);
char *read_nskip(char *p, int skipcnt, char *sep = NULL);
//char *read_string_w(char *p, unsigned char *wp, int iLen);
//char *wtom(char *p, unsigned char *wp, int iLen);
char get_hexchar(char hexnum);
char get_hexnum(char hexchar);

void split(const string& text, char* separators, vector<string>& words);
//int  utf8_to_multibyte(char * utf8, char * sMultibyte, int nAllocMultibyteSize);
//int  multibyte_to_utf8(char * sMultibyte, char * utf8, int nAllocMultibyteSize);
#endif	//	ifndef _STR_PARSE_
