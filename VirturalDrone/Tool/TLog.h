/*******************************************************************************
* ��Ȩ���� (C) 2007
*     �κ�һ���õ����ļ��������ˣ�������ʹ�úʹ������ļ������뱣���ļ�������
* �ļ����ƣ� TLog.h
* �ļ���ʶ�� 
* ����ժҪ�� ���ļ�������һ�����������־�ĺꡣ
* ����˵���� ʹ����Щ�����������־�����ᱻ��UDP���ݰ�����ʽ���͵�ָ����IP����ʱ
*			ͨ����־��������������ڸ�IP�ϻ�ȡ����Щ��־��Ϣ����Debugģʽ�£��ú�
*			���γ�TRACE��
*			�����ʹ�÷������£�
*			1������TLog.h��TLog.cpp������Ҫʹ�����Ĺ��̡�
*			2������Ҫʹ����������־����ĵط�������ͷ�ļ���
*			3��ֱ����ʹ��"printf"��"TRACE"һ��ʹ��"LogMsg"��"LogN(n)"��
*			4��һ�ֳ����������ǣ���ʹ���ߵ�ģ����ת�����Լ���"Log"����"#define Log LogN(1000)"��
*			   Ȼ����ֱ����ʹ��"printf"ʹ��"Log"��д��־��
* ע�����
*			1����printf�Լ�TRACE��һ�����ǣ�����־������Ļ���ӡ�����������־ĩ
*			β���Զ�����"\n"��
*			2������־��Ĭ������������־��������Ҫ��������ָ��IP������־����Ҫ
*			����"TLog::SetNetout(false)"��ֹ���Ρ�
* ��ǰ�汾�� V1.0
* ��    �ߣ� �ܷ�
* ������ڣ� 2007-09-01
*
* ��������		����		�޸�����
--------------------------------------------------------------------------------
* 2008-04-10	�ܷ�		������־���ʹ����windows��֧�ֿ��ַ���־�����
*******************************************************************************/
#ifndef _TLOG_H_1234789329489217489271849372891
#define _TLOG_H_1234789329489217489271849372891

#include <string>
#include <thread>
#include <mutex>
#include <fstream>

namespace Tool
{	
enum {
		Error=0,
		Warning,
		Info,
		Debug
	};
class TLog
{

public:

	//��ʼ��
	static void Init();

	//��ʼ��
	//static void Init(std::string &strPath, std::string &strIdent, unsigned int uKeepDays);
	static void Init(const char *strPath, const char *strIdent, unsigned int uKeepDays);

	//�����Ƿ�������Ļ���
	static void SetPrint(bool bOpen);

	//�����Ƿ�������ļ�
	static void SetWriteFile(bool bOpen);

	//������־����ȼ�
	static void SetLogLevel(int nLevel);

	//ɾ�����ڵ���־
	static void _RemoveOldLogFile();

	//���캯��
	TLog();

	//������"()"������
	void operator()(const char *pszFmt, ...) const;

	//�ж���־�ȼ��������
	void operator()(int nLeval, const char *pszFmt, ...) const;

#if defined _WIN32
	void operator()(const wchar_t * pszFmt, ...) const;
	void operator()(int nLeval, const wchar_t * pszFmt, ...) const;
#endif

	//����һ����ǰ����־�ļ���
	static std::string GetFileName(int nOffHour = 0);
	
protected:

	static bool m_bPrint;
	static bool m_bWriteFile;
	static std::mutex m_lock;
	static unsigned int m_nCount;
	static std::ofstream m_fLog;

	static int  m_uKeepDays; //��־����ʱ��
	static std::string m_strPath;
	static std::string m_strCurrFileName;
	static std::string m_strIdent;			//ϵͳ����
	static int m_nLevel;

};

//���������������־��һ���
#define LogN(n)		Tool::TLog()
#define LogMsg		LogN(0)



}

#endif

