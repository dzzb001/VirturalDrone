// 2009-02-04
// XStrUtil.h
// guoshanhe
// ��װһЩ�����ַ��������ĺ���

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
	// ȥ���ַ���ͷ(��β)�����ַ�����ָ�����ַ�
	static string& ChopHead(string &strSrc, const char *pcszCharSet = " \t\r\n");
	static string& ChopTail(string &strSrc, const char *pcszCharSet = " \t\r\n");
	static string& Chop(string &strSrc, const char *pcszCharSet = " \t\r\n");

	// �ַ���ת��д(��Сд)
	static void ToUpper(char *pszSrc);
	static void ToLower(char *pszSrc);
	static void ToUpper(string &strSrc);
	static void ToLower(string &strSrc);

	// ���ִ�Сд�Ƚ�
	static int Compare(const char* pszSrc1, const char* pszSrc2, int length = -1);
	static int Compare(const string &str1, const string &str2, int length = -1);

	// �����ִ�Сд�Ƚ�
	static int CompareNoCase(const char* pszSrc1, const char* pszSrc2, int length = -1);
	static int CompareNoCase(const string &str1, const string &str2, int length = -1);

	// �����ַ�����ָ���ķָ��ַ��ֽ�Դ�ַ���,�����õ�vector��
	// nMaxCountָ�������õ�������,������maxCount����ֹ������,�����������;��Ϊ-1��ʾ��������
	static uint32 Split(const string &strSrc, vector<string> &vItems, const char *pcszCharSet = " \r\n\t", int nMaxCount = -1);

	// �ַ���ת����
	static BOOL     ToInt(const string &strSrc, int &nValue, int radix = 10);
	static int      ToIntDefault(const string &strSrc, int def = -1, int radix = 10);
	static int      TryToIntDefault(const string &strSrc, int def = -1, int radix = 10);
	static BOOL     ToUInt(const string &strSrc, uint32 &uValue, int radix = 10);
	static uint32   ToUIntDefault(const string &strSrc, uint32 def = 0, int radix = 10);
	static uint32   TryToUIntDefault(const string &strSrc, uint32 def = 0, int radix = 10);

	// �ַ���ת��������
	static BOOL		ToFloat(const string &strSrc, double &value);
	static double	ToFloatDefault(const string &strSrc, double def = 0.0);
	static double	TryToFloatDefault(const string &strSrc, double def = 0.0);

	// ��ֵת�ַ���
	static string ToString(int nVal, const char* cpszFormat=NULL/*"%d"*/);
	static string ToString(uint32 uVal, const char* cpszFormat=NULL/*"%u"*/);
	static string ToString(int64 nlVal, const char* cpszFormat=NULL/*"%lld"*/);
	static string ToString(uint64 ulVal, const char* cpszFormat=NULL/*"%llu"*/);
	static string ToString(double fVal, const char* cpszFormat=NULL/*"%f"*/);
};

} // namespace xbase

using namespace xbase;

#endif//_X_STR_UTIL_H_
