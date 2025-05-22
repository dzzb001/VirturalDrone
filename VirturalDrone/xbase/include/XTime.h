// 2008-03-06 14:58
// XTime.h
// guoshanhe
// 时间类

#pragma once

#ifndef _X_TIME_H_
#define _X_TIME_H_

#include "XDefine.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XTime
////////////////////////////////////////////////////////////////////////////////
class XTime
{
public:
	enum Style
	{
		YYYY_MM_DD_HH_MM_SS,
		YYYY_MM_DD_HH_MM_SS_0,
		YYYY_MM_DD,
		YYYY_MM_DD_0,
		HH_MM_SS,
		HH_MM_SS_0,
		YYYYMMDDHHMMSS,
		YYYYMMDD,
		HHMMSS,
		MMDD,
		YYYYMMDDWWHHMMSS
	};

public:
	XTime(void);

	// 拷贝构造函数
	XTime(const XTime &temp);

	XTime(time_t sec, uint32 usec = 0);

	XTime(const struct tm &t);

	// 支持两种格式
	// 格式1: 2008-03-12 15:01:12
	// 格式2: 20080312150112
	XTime(const string &strTime);

	~XTime(void) {};

public:
	// seconds since 1970-01-01 00:00:00
	time_t tv_sec(void) const { return m_sec; }
	// microseconds since the second
	uint32 tv_usec(void) const { return m_usec; }
	// return struct tm
	struct tm* tv_tm(struct tm *ptm);
	// seconds after the minute - [0,59]
	int local_sec(void) const { return m_tm.tm_sec; }
	// minutes after the hour - [0,59]
	int local_min(void) const { return m_tm.tm_min; }
	// hours since midnight - [0,23]
	int local_hour(void) const { return m_tm.tm_hour; }
	// day of the month - [1,31]
	int local_mday(void) const { return m_tm.tm_mday; }
	// months since January - [1,12]
	int local_mon(void) const { return m_tm.tm_mon + 1; }
	// years - [1900, ]
	int local_year(void) const { return m_tm.tm_year + 1900; }
	// days since Sunday - [0,6]
	int local_wday(void) const { return m_tm.tm_wday; }
	// days since January 1 - [0,365]
	int local_yday(void) const { return m_tm.tm_yday; }

public:
	XTime &operator= (const XTime &temp);
	XTime &operator= (time_t sec);
	XTime &operator= (const struct tm& t);
	XTime &operator+= (time_t sec);
	XTime &operator-= (time_t sec);
	bool operator> (const XTime &temp);
	bool operator>= (const XTime &temp);
	bool operator< (const XTime &temp);
	bool operator<= (const XTime &temp);
	bool operator== (const XTime &temp);

public:
	// 是否有错(结构体构造时，可能因结构体参数错误而构造时间失败)
	BOOL IsErr(void) { return m_bErr; };

	// 根据指定风格打印
	string ToString(int style = YYYY_MM_DD_HH_MM_SS_0);

private:
	time_t		m_sec;		// 秒部分
	uint32		m_usec;		// 微秒部分
	struct tm	m_tm;		// 时间日历表示(本地表示)
	bool		m_bErr;		// 是否出错
};

inline 
struct tm* XTime::tv_tm(struct tm *ptm)
{
	if (ptm == NULL || m_bErr)
	{
		return NULL;
	}
	*ptm = m_tm;
	return ptm;
}

inline
bool XTime::operator> (const XTime &temp)
{
	if ((m_sec > temp.m_sec) || ((m_sec == temp.m_sec) && (m_usec > temp.m_usec)))
	{
		return true;
	}
	return false;
}

inline
bool XTime::operator>= (const XTime &temp)
{
	if ((m_sec > temp.m_sec) || ((m_sec == temp.m_sec) && (m_usec >= temp.m_usec)))
	{
		return true;
	}
	return false;
}

inline
bool XTime::operator< (const XTime &temp)
{
	if ((m_sec < temp.m_sec) || ((m_sec == temp.m_sec) && (m_usec < temp.m_usec)))
	{
		return true;
	}
	return false;
}

inline
bool XTime::operator<= (const XTime &temp)
{
	if ((m_sec < temp.m_sec) || ((m_sec == temp.m_sec) && (m_usec <= temp.m_usec)))
	{
		return true;
	}
	return false;
}

inline
bool XTime::operator== (const XTime &temp)
{
	if (m_sec == temp.m_sec && m_usec == temp.m_usec)
	{
		return true;
	}
	return false;
}

// 获取时区(-12 ~ 13)
int X_GetLocalTimeZone();

// 获取服务启动以来经历的秒值
uint32 X_GetRunningSeconds(void);

// 获取服务启动以来经历的毫秒值
uint64 X_GetRunningMilliseconds(void);

// 获取服务启动以来经历的微秒值
uint64 X_GetRunningMicroseconds(void);

} // namespace xbase

using namespace xbase;

#endif//_X_TIME_H_

