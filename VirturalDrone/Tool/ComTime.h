#pragma once
#include <time.h>
#include <chrono>

#ifndef _WIN32
#include <sys/time.h>
#endif // !_WIN32


class CComTime
{
public:
	CComTime()
	{
	}

	~CComTime()
	{
	}

	//获取两个时间的差值
	static time_t Span(tm &tmL, tm &tmR)
	{
		return mktime(&tmL) - mktime(&tmR);
	}

	//获取当前时间
	static time_t NowTimeT()
	{
		std::chrono::system_clock::time_point tmCur = std::chrono::system_clock::now();
		return std::chrono::system_clock::to_time_t(tmCur);
	}
	
	//获取当前时间
	static tm Now()
	{
		std::chrono::system_clock::time_point tmCur = std::chrono::system_clock::now();
		time_t tt = std::chrono::system_clock::to_time_t(tmCur);
		tm t;
#ifdef _WIN32
		localtime_s(&t, &tt);
#else	
		localtime_r(&tt, &t);
#endif // _WIN32
		return t;	
	}

	static tm Timet2Gmt(time_t &tt)
	{	
		tm t;
#ifdef _WIN32
		gmtime_s(&t, &tt);
#else	
		gmtime_r(&tt, &t);
#endif // _WIN32
		return t;
	}

	static tm Timet2Gmt(time_t &&tt)
	{
		tm t;
#ifdef _WIN32
		gmtime_s(&t, &tt);
#else	
		gmtime_r(&tt, &t);
#endif // _WIN32
		return t;
	}
	static tm GMT(int nAfterSeconds = 0)
	{
		std::chrono::system_clock::time_point tmCur = std::chrono::system_clock::now();
		time_t tt = std::chrono::system_clock::to_time_t(tmCur) + nAfterSeconds;
		return Timet2Gmt(tt);
	}

#ifndef _WIN32

	//获取启动时间毫秒数
	static unsigned int GetTickCount()
	{
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
	}
#else
	static unsigned int GetTickCount()
	{
		return ::GetTickCount();
	}
#endif // _WIN32


	//格式化时间为2012-01-12 15:04:51
	static std::string FormatTime(const tm &t)
	{
		char buff[32] = { 0 };
		if (0 == strftime(buff, 32, "%F %T", &t))
		{
			return std::string();
		}
		return std::string(buff);
	}

	//格式化时间为2012-01-12 15:04:51
	static std::string FormatTime(const time_t &tt)
	{
		tm t;
#ifdef _WIN32
		localtime_s(&t, &tt);
#else	
		localtime_r(&tt, &t);
#endif // _WIN32
		char buff[32] = { 0 };
		if (0 == strftime(buff, 32, "%F %T", &t))
		{
			return std::string();
		}
		return std::string(buff);
	}


	//格式胡时间为23 Jan 1997 15:35:06 GMT
	static std::string FormatTime1(const tm &t)
	{
		char buff[32] = { 0 };
		if (0 == strftime(buff, 32, "%d %B %Y %T", &t))
		{
			return std::string();
		}
		return std::string(buff);
	}

	//返回自程序运行以来的毫秒数，本函数精度比较高
	static long long Tick()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now().time_since_epoch()
			).count();
	}	

#ifdef _WIN32
	struct timeval_t
	{
		time_t tv_sec;			//单位:秒
		time_t tv_usec;			//单位:微秒 或这232皮秒
	};
#else
	typedef struct timeval timeval_t;
#endif // _WIN32


	static int GetTimeOfDay(timeval_t *tv)
	{
#ifdef _WIN32	
		static time_t tmOffset = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::system_clock::now().time_since_epoch()
			).count() - std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::high_resolution_clock::now().time_since_epoch()
				).count();
		time_t t = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::high_resolution_clock::now().time_since_epoch()
			).count() + tmOffset;
		tv->tv_sec = (long)(t / 1000000);
		tv->tv_usec = t % 1000000;
		return 0;
#else
		return gettimeofday(tv, nullptr);//第二个参数时区已经不再被支持
#endif // _WIN32
	}

	//为RTCP获取当前时刻的NTP时间，也就是自1900-01-01 00:00:00的时间
	static int NTP(timeval_t *tv)
	{
		GetTimeOfDay(tv);
		tv->tv_sec += 0x83AA7E80;//从1970-01-01 00:00:00 转到 1900-01-01 00:00:00为起点计算的秒数
		//printf("%d -- %I64d\n", sizeof(timeval_t), tv->div_sec.tv_232psec);
		tv->tv_usec = (time_t)(tv->tv_usec * 4294.967296 + 0.5);//换算为约1/2^32=232皮秒为单位的值，小数部分四舍五入。2^32/10^6 = 4294.967296
		return 0;
	}

};