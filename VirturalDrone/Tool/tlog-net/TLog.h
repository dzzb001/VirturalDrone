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

namespace Tool
{

class TLog
{
public:
	//��Ϣ����
	static void SetConfig(
		std::string strName = "default", 
		std::string strServerIP = "127.0.0.1",
		int nPort = 621002);

	//�����Ƿ�������Ļ���(������ǰ����Ļ�����־)
	static bool SetPrint(bool bClosePrint);

	//�����Ƿ������������(������ǰ�����������־)
	static bool SetNetout(bool bCloseNet);

	//���캯��
	TLog(const char *lpszFile, int nLine, int nLevel);

	//������"()"������
	void operator()(const char *pszFmt, ...) const;

#if defined (_WIN32) || defined (_WINDOWS_)
	void operator()(const wchar_t * pszFmt, ...) const;
#endif

private:

	//���縨���ṹ
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

	const char * m_lpszFile;		//Դ�ļ�·��
	const int m_nLine;				//�к�
	const int m_nLevel;				//��־�ȼ�

	static std::string m_sstrName;	//��ģ������
	static std::string m_sstrIP;	//��־������IP��ַ
	static int m_snPort;			//��־������������־�Ķ˿�
	static bool m_sbClosePrint;		//�Ƿ�������Ļ�����true��ʾ������Ļ���
	static bool m_sbCloseNet;		//�Ƿ��������������true��ʾ�����������

	//�����־���ݵ���־������
	void SendMsg(const char * lpszMsg) const;

};

//���������������־��һ���
#define LogN(n)		Tool::TLog(__FILE__, __LINE__, n)
#define LogMsg		LogN(0)

}

#endif

