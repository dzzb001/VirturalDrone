// 2008-06-05
// XLog.h
// guoshanhe
// ��־�ࣨ���̰߳�ȫ��

#pragma once

#ifndef _X_LOG_H_
#define _X_LOG_H_

#include "XDefine.h"

namespace xbase {

// 日志输出级别
enum XLOG_PRIORITY
{
	XLOG_NONE		=	0,	// 不打印日志
	XLOG_CRASH		=   1,  // 严重错误,导致程序不能继续运行
	XLOG_ERR		=   2,  // 程序错误,导致处理失败(如申请内存失败等)
	XLOG_WARNING	=	3,	// 程序警告,可能导致错误产生(如传入不合法参数)
	XLOG_NOTICE		=   4,  // 正常但是值得注意的情况
	XLOG_INFO	    =   5,  // 业务相关的信息(不影响程序流程，如显示用户登陆下线信息)
	XLOG_DEBUG	    =   6,  // 调试信息(不影响程序流程，如打印当前内存池中未使用内存块数目等)
	XLOG_TRACE		=	7,	// 打印程序运行轨迹
	XLOG_PRIORITY_MAX
};

// 日志输出位置
enum XLOG_OPTIONS
{
	XLOG_OPTION_FILE		=   0x01,   // log on to file, default
	XLOG_OPTION_CONSOLE		=   0x02,   // log on the console if errors in sending
	XLOG_OPTION_STDERR		=   0x04,   // log on the stderr stream
	XLOG_OPTION_TCP			=	0x08,	// log on the tcp stream
};

class XLogParam
{
public:
public:
	uint32			uOptions;			// 指定输出位置, see XLOG_OPTIONS
	XLOG_PRIORITY	uPriority;			// 指定输出级别, see XLOG_PRIORITY
	string			strPath;			// 日志文件总路径
	string			strIdent;			// 日志分类名称(如多个程序日志输出在同一文件夹下时)
	uint32			uMaxLineEachFile;	// 每个日志文件最大容纳行数，多出时创建新日志文件
	uint32			uKeepDays;			// 日志文件保存天数
	uint32			uListenPort;		// 输出tcp流时监听端口

	XLogParam()
		: uOptions(XLOG_OPTION_CONSOLE)
		, uPriority(XLOG_TRACE)
		, uMaxLineEachFile(0)
		, uKeepDays(30)
		, uListenPort(6015)
	{
		// empty
	}
};

class XLog
{
public:

	// 日志系统初始化
	static BOOL Init(const XLogParam& logParam);

	//  设置日志输出的信息级别
	static void SetPriority(uint32 uPriority/*see XLogPriority*/);

	//  获取日志输出的信息级别
	static uint32 GetPriority();

	// 设置日志输出指向
	static void SetOptions(uint32 uOptions/*see XLogOption*/);

	// 获取日志输出指向
	static uint32 GetOptions();

	// 设置日志的保存天数
	static void SetKeepDays(uint32 uDays);

	// 获取日志的保存天数
	static uint32 GetKeepDays();

	// 设置TCP日志流监听端口(0表示禁用)
	static BOOL SetTcpPort(uint32 port);

	// 输出
	static BOOL Printf(uint32 uPriority/*see XLogPriority*/, const char cszFormat[], ...);

	// 输出
	//static BOOL Printf(uint32 uPriority/*see XLogPriority*/, const char cszFormat[], ...);
	static BOOL Printf(uint32 nPriority/*see XLogPriority*/, const char cszFormat[], va_list marker);
};

#ifdef __DEBUG__
class XLogTrace
{
public:
	XLogTrace(const char *msg, int line)
		: m_msg(msg)
		, m_line(line)
	{
		XLog::Printf(XLOG_TRACE, "=>Entry %s():%d", m_msg, m_line);
	}

	~XLogTrace()
	{
		XLog::Printf(XLOG_TRACE, "<=Leave %s():%d", m_msg, m_line);
	}
private:
	const char *m_msg;
	int   m_line;
};
#define XLOGTRACE()		XLogTrace _tmplogtrace_(__X_FUNCTION__, __X_LINE__)
#else//__DEBUG__
#define XLOGTRACE()
#endif//__DEBUG__


#define XLOG_PROCESS_ERROR(Condition) \
	do  \
	{   \
		if (!(Condition))       \
		{                       \
			XLog::Printf(        \
				XLOG_ERR,    \
				"XLOG_PROCESS_ERROR(%s) at %s in %s:%s \n", #Condition, __X_FUNCTION__, __X_FILE__, __X_LINE__\
			);                  \
			goto Exit0;         \
		}                       \
	} while (false)

#define XLOG_PROCESS_SUCCESS(Condition) \
	do  \
	{   \
		if (Condition)          \
		{                       \
			XLog::Printf(        \
				XLOG_NOTICE,    \
				"XLOG_PROCESS_SUCCESS(%s) at %s in %s:%s \n", #Condition, __X_FUNCTION__, __X_FILE__, __X_LINE__ \
			);                  \
			goto Exit1;         \
		}                       \
	} while (false)

#define XLOG_CHECK_ERROR(Condition) \
	do  \
	{   \
		if (!(Condition))       \
		{                       \
			XLog::Printf(        \
				XLOG_NOTICE,    \
				"XLOG_CHECK_ERROR(%s) at %s in %s:%s \n", #Condition, __X_FUNCTION__, __X_FILE__, __X_LINE__ \
			);                  \
		}                       \
	} while (false)

#define XLOG_PROCESS_ERROR_RET(Condition, Code) \
	do  \
	{   \
		if (!(Condition))       \
		{                       \
			XLog::Printf(        \
				XLOG_ERR,    \
				"XLOG_PROCESS_ERROR_RET_CODE(%s, %d) at %s in %s:%s \n", #Condition, Code, __X_FUNCTION__, __X_FILE__, __X_LINE__ \
			);                  \
			nResult = Code;     \
			goto Exit0;         \
		}                       \
	} while (false)

#define XLOG_COM_PROCESS_ERROR(Condition) \
	do  \
	{   \
		if (FAILED(Condition))  \
		{                       \
			XLog::Printf(        \
				XLOG_ERR,    \
				"XLOG_COM_PROCESS_ERROR(%s) at %s in %s:%s \n", #Condition, __X_FUNCTION__, __X_FILE__, __X_LINE__ \
			);                  \
			goto Exit0;         \
		}                       \
	} while (false)


#define XLOG_COM_PROCESS_SUCCESS(Condition)   \
	do  \
	{   \
		if (SUCCEEDED(Condition))   \
		{                           \
			XLog::Printf(            \
				XLOG_NOTICE,        \
				"XLOG_COM_PROCESS_SUCCESS(%s) at %s in %s:%s \n", #Condition, __X_FUNCTION__, __X_FILE__, __X_LINE__ \
			);                      \
			goto Exit1;             \
		}                           \
	} while (false)


#define XLOG_COM_PROCESS_ERROR_RET(Condition, Code)     \
	do  \
	{   \
		if (FAILED(Condition))      \
		{                           \
			XLog::Printf(            \
				XLOG_ERR,        \
				"XLOG_COM_PROC_ERROR_RET_CODE(%s, 0X%X) at %s in %s:%s \n", #Condition, Code, __X_FUNCTION__, __X_FILE__, __X_LINE__ \
			);                      \
			hrResult = Code;        \
			goto Exit0;             \
		}                           \
	} while (false)

#define XLOG_COM_CHECK_ERROR(Condition) \
	do  \
	{   \
		if (FAILED(Condition))       \
		{                       \
			XLog::Printf(        \
				XLOG_NOTICE,    \
				"XLOG_COM_CHECK_ERROR(%s) at %s in %s:%s \n", #Condition, __X_FUNCTION__, __X_FILE__, __X_LINE__ \
			);                  \
		}                       \
	} while (false)

} // namespace xbase

using namespace xbase;

#endif//_X_LOG_H_
