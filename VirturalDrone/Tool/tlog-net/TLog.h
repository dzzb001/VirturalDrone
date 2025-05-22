/*******************************************************************************
* 版权所有 (C) 2007
*     任何一个得到本文件副本的人，均可以使用和传播本文件，但请保留文件申明。
* 文件名称： TLog.h
* 文件标识： 
* 内容摘要： 本文件定义了一组用于输出日志的宏。
* 其它说明： 使用这些宏所输出的日志，将会被以UDP数据包的形式发送到指定的IP，届时
*			通过日志服务器软件可以在该IP上获取到这些日志信息。在Debug模式下，该宏
*			会形成TRACE。
*			本软件使用方法如下：
*			1、将本TLog.h和TLog.cpp加入需要使用它的工程。
*			2、在需要使用它进行日志输出的地方包含本头文件。
*			3、直接像使用"printf"或"TRACE"一样使用"LogMsg"或"LogN(n)"。
*			4、一种常见的做法是，在使用者的模块中转定义自己的"Log"，如"#define Log LogN(1000)"；
*			   然后，再直接像使用"printf"使用"Log"来写日志。
* 注意事项：
*			1、和printf以及TRACE不一样的是，本日志类在屏幕或打印窗口输出的日志末
*			尾会自动加入"\n"。
*			2、本日志类默认屏蔽网络日志输出。如果要向网络上指定IP发送日志，需要
*			调用"TLog::SetNetout(false)"禁止屏蔽。
* 当前版本： V1.0
* 作    者： 周锋
* 完成日期： 2007-09-01
*
* 更新日期		作者		修改内容
--------------------------------------------------------------------------------
* 2008-04-10	周锋		更新日志软件使其在windows下支持宽字符日志输出。
*******************************************************************************/
#ifndef _TLOG_H_1234789329489217489271849372891
#define _TLOG_H_1234789329489217489271849372891

#include <string>

namespace Tool
{

class TLog
{
public:
	//信息配置
	static void SetConfig(
		std::string strName = "default", 
		std::string strServerIP = "127.0.0.1",
		int nPort = 621002);

	//设置是否屏蔽屏幕输出(返回以前的屏幕输出标志)
	static bool SetPrint(bool bClosePrint);

	//设置是否屏蔽网络输出(返回以前的网络输出标志)
	static bool SetNetout(bool bCloseNet);

	//构造函数
	TLog(const char *lpszFile, int nLine, int nLevel);

	//操作符"()"的重载
	void operator()(const char *pszFmt, ...) const;

#if defined (_WIN32) || defined (_WINDOWS_)
	void operator()(const wchar_t * pszFmt, ...) const;
#endif

private:

	//网络辅助结构
	struct NetCmd 
	{
		NetCmd(){}
		virtual ~NetCmd(){}

		NetCmd & operator = (const NetCmd &node)
		{
			m_data = node.GetData();
			return *this;
		}

		const std::string & GetData() const {return m_data;}

		void Clear()
		{
			m_data.clear();
		}

		void SetData(const char *pData, unsigned int nlen)
		{
			Clear();
			AddData(pData, nlen);
		}

		void AddData(const char *pData, unsigned int nLen)
		{
			m_data.insert(m_data.end(), pData, pData + nLen);
		}

		template <class T>
		NetCmd & operator << (const T & variable)
		{
			const char *pValue = (const char *)&variable;
			m_data.insert(m_data.end(), pValue, pValue + sizeof(variable));
			return *this;
		}

		NetCmd & operator << (const char * szText)
		{
			return *this << std::string(szText);
		}

		NetCmd & operator << (const std::string &szText)
		{
			unsigned int nLen = (unsigned int)szText.size();
			*this << nLen;
			m_data.insert(m_data.end(), szText.begin(), szText.end());
			return *this;
		}

	private:
		std::string m_data;
	};

	const char * m_lpszFile;		//源文件路径
	const int m_nLine;				//行号
	const int m_nLevel;				//日志等级

	static std::string m_sstrName;	//本模块名称
	static std::string m_sstrIP;	//日志服务器IP地址
	static int m_snPort;			//日志服务器接收日志的端口
	static bool m_sbClosePrint;		//是否屏蔽屏幕输出，true表示屏蔽屏幕输出
	static bool m_sbCloseNet;		//是否屏蔽网络输出，true表示屏蔽网络输出

	//输出日志内容到日志服务器
	void SendMsg(const char * lpszMsg) const;

};

//以下是用于输出日志的一组宏
#define LogN(n)		Tool::TLog(__FILE__, __LINE__, n)
#define LogMsg		LogN(0)

}

#endif

