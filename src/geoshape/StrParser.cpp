//-----------------------------------------------------------------------------
//	NvStringParse.cpp
//		$Generate	: sbhuh
//		$Update : 2008.05.08
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "geoshape/StrParser.hpp"


char *read_number(char *p, unsigned char *number, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;

	char buf[512];
	char *str = buf;


	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = (str != buf) ? atoi(buf) : 0xff;
	return *p ? (++p) : 0;
}

char *read_number(char *p, unsigned short *number, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;

	char buf[512];
	char *str = buf;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = (str != buf) ? atoi(buf) : 0xffff;
	return *p ? (++p) : 0;
}

char *read_number(char *p, int *number, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;
	char buf[512];
	char *str = buf;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = (str != buf) ? atoi(buf) : 0xffffffff;
	return *p ? (++p) : 0;
}
char *read_skip(char *p, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;

	while (*p != tk && *p != 0)
	{
		p++;
	}

	return *p ? (++p) : 0;
}
char *read_nskip(char *p, int skipcnt, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';

	if (sep) tk = *sep;

	for (int loop1 = 0; loop1 < skipcnt; loop1++)
	{

		while (*p != tk && *p != 0)
		{

			p++;
		}

		if (loop1 < skipcnt - 1)
		{
			p++;
		}
	}

	return *p ? (++p) : 0;
}
char *read_hex(char *p, unsigned short *number, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;

	char buf[512];
	char *str = buf;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	// #if _MSC_VER >= 1400
	// 	sscanf_s(buf, "%x", number);
	// #else

    sscanf(buf, "%x", (unsigned int *) number);
	//#endif
	return *p ? (++p) : 0;
}

char get_hexchar(char hexnum)
{
	if (hexnum < 0 || hexnum > 15) return -1;
	if (hexnum >= 0 && hexnum <= 9)
		return hexnum + '0';
	else if (hexnum >= 10 && hexnum <= 15)
		return hexnum + 'A';

	return -1;
}
char get_hexnum(char hexchar)
{
	if (hexchar >= ('0') && hexchar <= ('9'))
		return hexchar - '0';
	else if (hexchar >= ('A') && hexchar <= ('F'))
		return (hexchar - 'A') + 10;
	else if (hexchar >= ('a') && hexchar <= ('f'))
		return (hexchar - 'a') + 10;
	return 'z';
}
char *read_number(char *p, short *number, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;

	char buf[512];
	char *str = buf;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = (str != buf) ? atoi(buf) : 0xffff;
	return *p ? (++p) : 0;
}

char *read_number(char *p, unsigned long *number, char *sep)
{
	if (!p)
		return 0;

	char buf[512];
	char *str = buf;

	char tk = '\t';
	if (sep) tk = *sep;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = (str != buf) ? atoi(buf) : 0xffffffff;
	return *p ? (++p) : 0;
}

char *read_number(char *p, unsigned int *number, char *sep)
{
	if (!p)
		return 0;

	char buf[512];
	char *str = buf;

	char tk = '\t';
	if (sep) tk = *sep;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';
	*number = (str != buf) ? atoi(buf) : 0xffffffff;

	return *p ? (++p) : 0;
}
char *read_number(char *p, long *number, char *sep)
{
	if (!p)
		return 0;
	char buf[128];
	char *str = buf;

	char tk = '\t';
	if (sep) tk = *sep;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = (str != buf) ? atoi(buf) : 0xffffffff;
	return *p ? (++p) : 0;
}

//test for " "
char *read_string(char *p, char *str, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;
	int count = 0;
	if (*p == '\"')
		p++;
	str[0] = 0;
	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
		count++;
	}

	if (count > 0 && *(str - 1) == '\"')
		*(str - 1) = '\0';
	else
		*str = '\0';

	return *p ? (++p) : 0;
}

char *read_nstring(char *p, char *str, int len, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;
	int count = 0;
	if (*p == '\"')
		p++;
	str[0] = 0;
	while (*p != tk && *p != 0)
	{
		if (count < len - 1)
		{
			*str++ = *p++;
		}
		else
		{
			p++;
		}

		count++;
	}

	if (count > 0 && *(str - 1) == '\"')
		*(str - 1) = '\0';
	else
		*str = '\0';

	return *p ? (++p) : 0;
}

char *read_string_ftrim(char *p, char *str, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;
	int count = 0;
	if (*p == '\"')
		p++;

	while (tk == *p)
	{
		p++;
	}

	str[0] = 0;
	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
		count++;
	}

	if (count > 0 && *(str - 1) == '\"')
		*(str - 1) = '\0';
	else
		*str = '\0';

	return *p ? (++p) : 0;
}


char *read_float(char *p, float *number, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;

	char buf[512];
	char *str = buf;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = (float)atof(buf);
	return *p ? (++p) : 0;
}

char *read_double(char *p, double *number, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;

	char buf[512];
	char *str = buf;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = atof(buf);
	return *p ? (++p) : 0;
}


char *read_double_ftrim(char *p, double *number, char *sep)
{
	if (!p)
		return 0;

	char tk = '\t';
	if (sep) tk = *sep;

	while (tk == *p)
	{
		p++;
	}


	char buf[512];
	char *str = buf;

	while (*p != tk && *p != 0)
	{
		*str++ = *p++;
	}
	*str = '\0';

	*number = atof(buf);
	return *p ? (++p) : 0;
}

/*

char*	read_string_w(char *p, WCHAR *wp, int iLen)
{
	char szBuf[512];
	int ret;

	if (p == 0 || wp == 0 || iLen <= 0) return 0;

	memset(szBuf, 0x00, sizeof(szBuf));
	p = read_string(p, szBuf);

	if (szBuf[0] == '0' && strlen(szBuf) < 2)
	{
		memset(wp, 0x00, iLen * sizeof(WCHAR));
	}
	else
	{
		ret = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, szBuf, strlen(szBuf) + 1, wp, iLen);
		if (ret == 0)	memset(wp, 0x00, iLen * sizeof(WCHAR));
	}



	return p;
}



char*	wtom(char *p, WCHAR *wp, int iLen)
{
	static char s_szNull[4] = "0";
	int ret;

	if (wp == 0 || p == 0 && iLen <= 0) return s_szNull;

	ret = WideCharToMultiByte(CP_ACP, 0, wp, -1, p, iLen, NULL, NULL);
	if (ret == 0)	return s_szNull;

	return p;
}
*/
void split(const string& text, char* separators, vector<string>& words)
{
	int n = text.length();
	int start, stop;

	start = text.find_first_not_of(separators);
	while ((start >= 0) && (start < n))
	{
		stop = text.find_first_of(separators, start);
		if ((stop < 0) || (stop > n))
			stop = n;
		words.push_back(text.substr(start, stop - start));
		start = text.find_first_not_of(separators, stop + 1);
	}
}
/*
int  utf8_to_multibyte(char * strUTF8, char * sMultibyte, int nAllocMultibyteSize)
{
#define ENTYPE_STR_LEN		2048
	wchar_t sUnicode[ENTYPE_STR_LEN] = { 0, };

	// utf8 --> unicode
	int nUnicodeLen = MultiByteToWideChar(CP_UTF8, 0, strUTF8, strlen(strUTF8), NULL, NULL);
	if (nUnicodeLen >= ENTYPE_STR_LEN - 1)
		return -1;
	MultiByteToWideChar(CP_UTF8, 0, strUTF8, strlen(strUTF8), sUnicode, nUnicodeLen);

	// unicode --> multibyte
	int nMultibyteLen = WideCharToMultiByte(CP_ACP, 0, sUnicode, -1, NULL, 0, NULL, NULL);
	if (nMultibyteLen >= nAllocMultibyteSize - 1)
		return -1;
	WideCharToMultiByte(CP_ACP, 0, sUnicode, -1, sMultibyte, nMultibyteLen, NULL, NULL);


	return 0;
}

int  multibyte_to_utf8(char * sMultibyte, char * strUTF8, int nAllocMultibyteSize)
{
#define ENTYPE_STR_LEN		2048
	wchar_t sUnicode[ENTYPE_STR_LEN] = { 0, };

	// multibyte  --> unicode
	int nUnicodeLen = MultiByteToWideChar(CP_ACP, 0, sMultibyte, strlen(sMultibyte), NULL, NULL);
	if (nUnicodeLen >= ENTYPE_STR_LEN - 1)
		return -1;
	MultiByteToWideChar(CP_ACP, 0, sMultibyte, strlen(sMultibyte), sUnicode, nUnicodeLen);


	// unicode --> utf8 
	int nUtf8Len = WideCharToMultiByte(CP_UTF8, 0, sUnicode, -1, strUTF8, 0, NULL, NULL);
	if (nUtf8Len >= nAllocMultibyteSize - 1)
		return -1;
	WideCharToMultiByte(CP_UTF8, 0, sUnicode, -1, strUTF8, nUtf8Len, NULL, NULL);

	return 0;
}
*/
