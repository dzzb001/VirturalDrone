// 2009-02-04
// XStrUtil.cpp
// guoshanhe
// 包装一些方便字符串操作的函数

#include "XStrUtil.h"
#include <math.h>

namespace xbase {

string& XStrUtil::ChopHead(string &strSrc, const char *pcszCharSet/* = " \t\r\n"*/)
{
	if (pcszCharSet == NULL) return strSrc;
	size_t pos = strSrc.find_first_not_of(pcszCharSet);
	return strSrc.erase(0, pos);
}

string& XStrUtil::ChopTail(string &strSrc, const char *pcszCharSet/* = " \t\r\n"*/)
{
	if (pcszCharSet == NULL) return strSrc;
	size_t pos = strSrc.find_last_not_of(pcszCharSet);
	if (pos == string::npos)
	{
		strSrc.clear();
		return strSrc;
	}
	return strSrc.erase(++pos);
}

string& XStrUtil::Chop(string &strSrc, const char *pcszCharSet/* = " \t\r\n"*/)
{
	ChopHead(strSrc, pcszCharSet);
	return ChopTail(strSrc, pcszCharSet);
}

void XStrUtil::ToUpper(char *pszSrc)
{
	if (pszSrc == NULL) return;
	char *pos = (char *)pszSrc;
	char diff = 'A' - 'a';
	while (*pos != '\0')
	{
		if ('a' <= *pos && *pos <= 'z')
		{
			*pos += diff;
		}
		pos++;
	}
	return;
}

void XStrUtil::ToLower(char *pszSrc)
{
	if (pszSrc == NULL) return;
	char *pos = (char *)pszSrc;
	char diff = 'A' - 'a';
	while (*pos != '\0')
	{
		if ('A' <= *pos && *pos <= 'Z')
		{
			*pos -= diff;
		}
		pos++;
	}
	return;
}

void XStrUtil::ToLower(string &strSrc)
{
	return ToLower((char *)strSrc.c_str());
}

void XStrUtil::ToUpper(string &strSrc)
{
	return ToUpper((char *)strSrc.c_str());
}

int XStrUtil::Compare(const char* pszSrc1, const char* pszSrc2, int length/* = -1*/)
{
	ASSERT(pszSrc1 && pszSrc2);

	int ret = 0;
	const char *left = pszSrc1;
	const char *right = pszSrc2;

	while ((length != 0) && (*left != '\0') && (*right != '\0'))
	{
		if (length > 0) length--;
		ret = *left++ - *right++;
		if (ret != 0) return ret;
	}
	if (length == 0) return 0;
	return (*left - *right);
}

int XStrUtil::Compare(const string &str1, const string &str2, int length/* = -1*/)
{
	return Compare(str1.c_str(), str2.c_str(), length);
}

int XStrUtil::CompareNoCase(const char* pszSrc1, const char* pszSrc2, int length/* = -1*/)
{
	ASSERT(pszSrc1 && pszSrc2);

	int ret = 0;
	const char *left = pszSrc1;
	const char *right = pszSrc2;

	while ((length != 0) && (*left != '\0') && (*right != '\0'))
	{
		if (length > 0) length--;
		ret = ::tolower(*left++) - ::tolower(*right++);
		if (ret != 0) return ret;
	}
	if (length == 0) return 0;
	return (*left - *right);
}

int XStrUtil::CompareNoCase(const string &str1, const string &str2, int length/* = -1*/)
{
	return CompareNoCase(str1.c_str(), str2.c_str(), length);
}

uint32 XStrUtil::Split(const string &strSrc, vector<string> &vItems, const char *pcszCharSet/* = " \r\n\t"*/, int nMaxCount/* = -1*/)
{
	vItems.clear();

	size_t pos_begin = 0;
	size_t pos_end = 0;
	int count = 0;
	while (pos_end != string::npos)
	{
		pos_begin = strSrc.find_first_not_of(pcszCharSet, pos_end);
		if (pos_begin == string::npos) break;
		pos_end = strSrc.find_first_of(pcszCharSet, pos_begin);
		string strTmp(strSrc, pos_begin, pos_end - pos_begin);
		if (!strTmp.empty())
		{
			count++;
			vItems.push_back(strTmp);
		}
		if (nMaxCount > 0 && count >= nMaxCount)
		{
			break;
		}
	}
	return (uint32)vItems.size();
}

BOOL XStrUtil::ToInt(const string &strSrc, int &nValue, int radix/* = 10*/)
{
	char* endPtr = 0;
	string str = strSrc;
	
	Chop(str);
	if (str.empty()) return FALSE;

	errno = 0;
	nValue = (int)strtol(str.c_str(), &endPtr, radix);
	if (endPtr - str.c_str() != (int)str.size())
	{
		return FALSE;
	}
 	if ((errno == ERANGE) && ((nValue == LONG_MIN) || (nValue == LONG_MAX)))
 	{
 		return FALSE;
 	}
	return TRUE;
}

int XStrUtil::ToIntDefault(const string &strSrc, int def/* = -1*/, int radix/* = 10*/)
{
	char* endPtr = 0;
	int nValue = 0;
	string str = strSrc;

	Chop(str);
	if (str.empty()) return def;

	errno = 0;
	nValue = (int)strtol(str.c_str(), &endPtr, radix);
	if (endPtr - str.c_str() != (int)str.size())
	{
		return def;
	}
	if ((errno == ERANGE) && ((nValue == LONG_MIN) || (nValue == LONG_MAX)))
	{
		return def;
	}
	return nValue;
}

int XStrUtil::TryToIntDefault(const string &strSrc, int def/* = -1*/, int radix/* = 10*/)
 {
	char* endPtr = 0;
	int nValue = 0;
	string str = strSrc;

	Chop(str);
	if (str.empty()) return def;

	errno = 0;
	nValue = (int)strtol(str.c_str(), &endPtr, radix);
	if (endPtr == str.c_str())
	{
		return def;
	}
	if ((errno == ERANGE) && ((nValue == LONG_MIN) || (nValue == LONG_MAX)))
	{
		return def;
	}
	return nValue;
}

BOOL XStrUtil::ToUInt(const string &strSrc, uint32 &uValue, int radix/* = 10*/)
{
	char* endPtr = 0;
	string str = strSrc;

	Chop(str);
	if (str.empty()) return FALSE;

	errno = 0;
	uValue = (int)strtoul(str.c_str(), &endPtr, radix);
	if (endPtr - str.c_str() != (int)str.size())
	{
		return FALSE;
	}
	if ((errno == ERANGE) && (uValue == ULONG_MAX))
	{
		return FALSE;
	}
	return TRUE;
}

uint32 XStrUtil::ToUIntDefault(const string &strSrc, uint32 def/* = 0*/, int radix/* = 10*/)
{
	char* endPtr = 0;
	uint32 uValue = 0;
	string str = strSrc;

	Chop(str);
	if (str.empty()) return def;

	errno = 0;
	uValue = (int)strtol(str.c_str(), &endPtr, radix);
	if (endPtr - str.c_str() != (int)str.size())
	{
		return def;
	}
	if ((errno == ERANGE) && (uValue == ULONG_MAX))
	{
		return def;
	}
	return uValue;
}

uint32 XStrUtil::TryToUIntDefault(const string &strSrc, uint32 def/* = 0*/, int radix/* = 10*/)
{
	char* endPtr = 0;
	uint32 uValue = 0;
	string str = strSrc;

	Chop(str);
	if (str.empty()) return def;

	errno = 0;
	uValue = (int)strtol(str.c_str(), &endPtr, radix);
	if (endPtr == str.c_str())
	{
		return def;
	}
	if ((errno == ERANGE) && (uValue == ULONG_MAX))
	{
		return def;
	}
	return uValue;
}

BOOL XStrUtil::ToFloat(const string &strSrc, double &value)
{
	char* endPtr = 0;
	string str = strSrc;

	Chop(str);
	if (str.empty()) return FALSE;

	errno = 0;
	value = (int)strtod(str.c_str(), &endPtr);
	if (endPtr - str.c_str() != (int)str.size())
	{
		return FALSE;
	}
	if (errno == ERANGE)
	{
		return FALSE;
	}
	return TRUE;
}

double XStrUtil::ToFloatDefault(const string &strSrc, double def/* = 0.0*/)
{
	char* endPtr = 0;
	double fValue = 0.0;
	string str = strSrc;

	Chop(str);
	if (str.empty()) return def;

	errno = 0;
	fValue = (int)strtod(str.c_str(), &endPtr);
	if (endPtr - str.c_str() != (int)str.size())
	{
		return def;
	}
	if (errno == ERANGE)
	{
		return def;
	}
	return fValue;
}

double	XStrUtil::TryToFloatDefault(const string &strSrc, double def/* = 0.0*/)
{
	char* endPtr = 0;
	double fValue = 0;
	string str = strSrc;

	Chop(str);
	if (str.empty()) return def;

	errno = 0;
	fValue = (int)strtod(str.c_str(), &endPtr);
	if (endPtr == str.c_str())
	{
		return def;
	}
	if (errno == ERANGE)
	{
		return def;
	}
	return fValue;
}

string XStrUtil::ToString(int nVal, const char* cpszFormat)
{
	char buf[32];
	if (cpszFormat == NULL)
		sprintf(buf, "%d", nVal);
	else
		sprintf(buf, cpszFormat, nVal);
	return buf;
}

string XStrUtil::ToString(uint32 uVal, const char* cpszFormat)
{
	char buf[32];
	if (cpszFormat == NULL)
		sprintf(buf, "%u", uVal);
	else
		sprintf(buf, cpszFormat, uVal);
	return buf;
}

string XStrUtil::ToString(int64 nlVal, const char* cpszFormat)
{
	char buf[64];
	if (cpszFormat == NULL)
		sprintf(buf, "%lld", nlVal);
	else
		sprintf(buf, cpszFormat, nlVal);
	return buf;
}

string XStrUtil::ToString(uint64 ulVal, const char* cpszFormat)
{
	char buf[64];
	if (cpszFormat == NULL)
		sprintf(buf, "%llu", ulVal);
	else
		sprintf(buf, cpszFormat, ulVal);
	return buf;
}

string XStrUtil::ToString(double fVal, const char* cpszFormat)
{
	char buf[64];
	if (cpszFormat == NULL)
		sprintf(buf, "%f", fVal);
	else
		sprintf(buf, cpszFormat, fVal);
	return buf;
}

} // namespace xbase
