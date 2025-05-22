// 2008-03-06 14:58
// XTime.cpp
// guoshanhe
// 时间类


#include "XTime.h"

namespace xbase {

static XTime serviceStartTime;	// 记录系统启动时间

// time_t --> struct tm
static bool _LocalTime(struct tm &_Tm, const time_t &_Time)
{
#ifdef __WINDOWS__
	return (0 == localtime_s(&_Tm, &_Time));
#endif//__WINDOWS__
#ifdef __GNUC__
	return (NULL != localtime_r(&_Time, &_Tm));
#endif// __GNUC__
}

////////////////////////////////////////////////////////////////////////////////
// XTime
////////////////////////////////////////////////////////////////////////////////
XTime::XTime(void)
{
#ifdef __WINDOWS__
	SYSTEMTIME st = {0};
	GetLocalTime(&st);
	m_usec = st.wMilliseconds * 1000;
	m_sec  = time(NULL);
#endif//__WINDOWS__
#ifdef __GNUC__
	struct timeval tv = {0};
	gettimeofday(&tv, NULL);
	m_usec = tv.tv_usec;
	m_sec  = tv.tv_sec;
#endif// __GNUC__

	m_bErr = !_LocalTime(m_tm, m_sec);
}

XTime::XTime(const XTime &temp)
{
	memcpy(this, &temp, sizeof(XTime));
};

XTime::XTime(time_t sec, uint32 usec)
	: m_sec(sec)
	, m_usec(usec)
{
	m_bErr = !_LocalTime(m_tm, m_sec);
}

XTime::XTime(const struct tm &t)
	: m_usec(0)
	, m_tm(t)
	, m_bErr(false)
{
	m_sec = mktime((struct tm *)&t);
	if (m_sec == (time_t)-1)
	{
		m_bErr = true;
		m_sec = 0;
	}
}

// 支持两种格式
// 格式1: 2008-03-12 15:01:12
// 格式2: 20080312150112
XTime::XTime(const string &strTime)
{
	int ret = 0;
	memset(&m_tm, 0, sizeof(m_tm));

	// 尝试使用第一种格式解析
	ret = sscanf(strTime.c_str(), " %d - %d - %d %d : %d : %d", 
				&m_tm.tm_year, &m_tm.tm_mon, &m_tm.tm_mday, &m_tm.tm_hour, &m_tm.tm_min, &m_tm.tm_sec);
	if (ret != 6)
	{
		ret = sscanf(strTime.c_str(), "%4d%2d%2d%2d%2d%2d", 
					&m_tm.tm_year, &m_tm.tm_mon, &m_tm.tm_mday, &m_tm.tm_hour, &m_tm.tm_min, &m_tm.tm_sec);
		if (ret != 6)
		{
			m_bErr = true;
			m_sec = 0;
			m_usec = 0;
			return;
		}
	}
	if (m_tm.tm_year < 1970 || 
		m_tm.tm_year > 2038 ||
		m_tm.tm_mon < 1 ||
		m_tm.tm_mon > 12 ||
		m_tm.tm_mday < 1 ||
		m_tm.tm_mday > 31 ||
		m_tm.tm_hour < 0 ||
		m_tm.tm_hour > 24 ||
		m_tm.tm_min < 0 ||
		m_tm.tm_min > 59 ||
		m_tm.tm_sec < 0 ||
		m_tm.tm_sec > 59)
	{
		m_bErr = true;
		m_sec = 0;
		m_usec = 0;
		return;
	}

	m_tm.tm_year -= 1900;
	m_tm.tm_mon -= 1;
	m_usec = 0;
	m_bErr = false;
	m_sec = mktime(&m_tm);
	if (m_sec == (time_t)-1)
	{
		m_bErr = true;
		m_sec = 0;
	}
}

XTime &XTime::operator= (const XTime &temp)
{
	if (&temp != this)
	{
		memcpy(this, &temp, sizeof(XTime));
	}
	return *this;
}

XTime &XTime::operator= (time_t sec)
{
	m_sec = sec;
	m_usec = 0;
	m_bErr = !_LocalTime(m_tm, m_sec);
	return *this;
}
XTime &XTime::operator= (const struct tm& t)
{
	m_tm = t;
	m_usec = 0;
	m_bErr = false;
	m_sec = mktime((struct tm *)&t);
	if (m_sec == (time_t)-1)
	{
		m_bErr = true;
		m_sec = 0;
	}
	return *this;
}

XTime &XTime::operator+= (time_t sec)
{ 
	m_sec += sec;
	m_bErr = !_LocalTime(m_tm, m_sec);
	return *this;
}

XTime &XTime::operator-= (time_t sec)
{ 
	if (m_sec >= sec)
	{
		m_sec -= sec;
		m_bErr = !_LocalTime(m_tm, m_sec);
	}
	else
	{
		m_sec = 0;
		m_bErr = true;
	}
	return *this;
}

// 根据指定风格打印
string XTime::ToString(int style)
{
	char str[64];

	if (m_bErr)
	{
		return string("");
	}

	switch (style)
	{
	case YYYY_MM_DD_HH_MM_SS:
		sprintf(str, "%d-%d-%d %d:%d:%d", 
			m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday, 
			m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec
			);
		break;
	case YYYY_MM_DD:
		sprintf(str, "%d-%d-%d", 
			m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday
			);
		break;
	case YYYY_MM_DD_0:
		sprintf(str, "%04d-%02d-%02d", 
			m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday
			);
		break;
	case HH_MM_SS:
		sprintf(str, "%d:%d:%d", 
			m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec
			);
		break;
	case HH_MM_SS_0:
		sprintf(str, "%02d:%02d:%02d", 
			m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec
			);
		break;
	case YYYYMMDDHHMMSS:
		sprintf(str, "%04d%02d%02d%02d%02d%02d", 
			m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday, 
			m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec
			);
		break;
	case YYYYMMDD:
		sprintf(str, "%04d%02d%02d", 
			m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday
			);
		break;
	case HHMMSS:
		sprintf(str, "%02d%02d%02d", 
			m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec
			);
		break;
	case MMDD:
		sprintf(str, "%02d%02d", 
			m_tm.tm_mon + 1, m_tm.tm_mday
			);
		break;
	case YYYYMMDDWWHHMMSS:
		sprintf(str, "%04d%02d%02d%02d%02d%02d%02d", 
			m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday, 
			m_tm.tm_wday == 0 ? 7 : m_tm.tm_wday,
			m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec
			);
		break;
	case YYYY_MM_DD_HH_MM_SS_0:
	default:
		sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", 
			m_tm.tm_year + 1900, m_tm.tm_mon + 1, m_tm.tm_mday, 
			m_tm.tm_hour, m_tm.tm_min, m_tm.tm_sec
			);
		break;
	}
	return str;
}

// 获取时区(-12 ~ 13)
int X_GetLocalTimeZone()
{
	time_t t_time = 24 * 3600;
	struct tm tm1;

	_LocalTime(tm1, t_time);
	return (tm1.tm_mday - 1) * 24 + tm1.tm_hour - 24;
}

// 获取服务启动以来经历的秒值
uint32 X_GetRunningSeconds(void)
{
	return (uint32)(XTime().tv_sec() - serviceStartTime.tv_sec());
}

// 获取服务启动以来经历的毫秒值
uint64 X_GetRunningMilliseconds(void)
{
	XTime now;
	uint64 ret = 0;
	ret = (uint64)(now.tv_sec() - serviceStartTime.tv_sec());
	ret *= 1000;
	ret += ((uint64)(1000000 + now.tv_usec() - serviceStartTime.tv_usec())) / 1000;
	ret -= 1000;
	return ret;
}

// 获取服务启动以来经历的微秒值
uint64 X_GetRunningMicroseconds(void)
{
	XTime now;
	uint64 ret = 0;
	ret = (uint64)(now.tv_sec() - serviceStartTime.tv_sec());
	ret *= 1000000;
	ret += (uint64)(1000000 + now.tv_usec() - serviceStartTime.tv_usec());
	ret -= 1000000;
	return ret;
}

} // namespace xbase
