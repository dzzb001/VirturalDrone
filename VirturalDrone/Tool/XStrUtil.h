// 2009-02-04
// XStrUtil.h
// guoshanhe
// 包装一些方便字符串操作的函数

#pragma once

#ifndef _X_STR_UTIL_H_
#define _X_STR_UTIL_H_

#include "XDefine.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XStrUtil
////////////////////////////////////////////////////////////////////////////////
class XStrUtil
{
public:
	// 去除字符串头(或尾)中在字符集中指定的字符
	static string& ChopHead(string &strSrc, const char *pcszCharSet = " \t\r\n");
	static string& ChopTail(string &strSrc, const char *pcszCharSet = " \t\r\n");
	static string& Chop(string &strSrc, const char *pcszCharSet = " \t\r\n");

	// 字符串转大写(或小写)
	static void ToUpper(char *pszSrc);
	static void ToLower(char *pszSrc);
	static void ToUpper(string &strSrc);
	static void ToLower(string &strSrc);

	// 区分大小写比较
	static int Compare(const char* pszSrc1, const char* pszSrc2, int length = -1);
	static int Compare(const string &str1, const string &str2, int length = -1);

	// 不区分大小写比较
	static int CompareNoCase(const char* pszSrc1, const char* pszSrc2, int length = -1);
	static int CompareNoCase(const string &str1, const string &str2, int length = -1);

	// 根据字符集中指定的分隔字符分解源字符串,并放置到vector中
	// nMaxCount指定期望得到的行数,解析到maxCount将终止并返回,不会继续解析;设为-1表示解析所有
	static uint32 Split(const string &strSrc, vector<string> &vItems, const char *pcszCharSet = " \r\n\t", int nMaxCount = -1);

	// 字符串转整数
	static BOOL     ToInt(const string &strSrc, int &nValue, int radix = 10);
	static int      ToIntDefault(const string &strSrc, int def = -1, int radix = 10);
	static int      TryToIntDefault(const string &strSrc, int def = -1, int radix = 10);
	static BOOL     ToUInt(const string &strSrc, uint32 &uValue, int radix = 10);
	static uint32   ToUIntDefault(const string &strSrc, uint32 def = 0, int radix = 10);
	static uint32   TryToUIntDefault(const string &strSrc, uint32 def = 0, int radix = 10);

	// 字符串转浮点型数
	static BOOL		ToFloat(const string &strSrc, double &value);
	static double	ToFloatDefault(const string &strSrc, double def = 0.0);
	static double	TryToFloatDefault(const string &strSrc, double def = 0.0);

	// 数值转字符串
	static string ToString(int nVal, const char* cpszFormat=NULL/*"%d"*/);
	static string ToString(uint32 uVal, const char* cpszFormat=NULL/*"%u"*/);
	static string ToString(int64 nlVal, const char* cpszFormat=NULL/*"%lld"*/);
	static string ToString(uint64 ulVal, const char* cpszFormat=NULL/*"%llu"*/);
	static string ToString(double fVal, const char* cpszFormat=NULL/*"%f"*/);
};

} // namespace xbase

using namespace xbase;

#endif//_X_STR_UTIL_H_
