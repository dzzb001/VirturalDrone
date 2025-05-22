#pragma once
#include "MsgParser.h"
#include <atomic>
#include <set>

namespace Sip
{

typedef void            *LPVOID; 
typedef unsigned long   DWORD;
//ע��״̬�ı�ص���������
typedef void (*CT_Register)(LPVOID lpContext,	//��������
							bool bRegister		//true������ false����
							);

class CRegister
{
public:
	CRegister(void);

	~CRegister(void);

	//ע��ע��״̬�ı�ص�����
	void RegCallBack(CT_Register pfn, LPVOID lpContext);

	//���ò���
	void SetParam(
		const std::string &strLocalIp,			//����IP
		int	nLocalPort,							//���ض˿�
		const std::string &strServerIp,			//��������IP
		int	nServerPort,						//�˻�PORT
		const std::string &strServerID,			//��������ID
		const std::string &strID,				//�˻�ID
		const std::string &strPassword,			//����
		int nExpire,							//����ʱ�䣬��λ��
		int nHeartInterval,						//�����������λ��
		int nHeartCount							//ʧȥ����������Ϊ�Ͽ�
		);

	//��������
	bool CmdIn(CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//������
	void Process();

	//ע������ע���ɹ�����true�����򷵻�false
	bool UnregisterQ();

	//�Ƿ�ע��ɹ�
	bool IsRegisted();

	//��ȡ����������Ϣ
	std::string & ServerID(){return m_strServerID;}
	std::string & ServerIP(){return m_strServerIP;}
	int ServerPort(){return m_nServerPort;}
	std::string & LocalID(){return m_strID;}
	std::string & LocalIP(){return m_strLocalIP;}
	int LocalPort(){return m_nLocalPort;}


protected:

	//
	//��Ϣ
	//

	std::string m_strServerID;				//������ID
	std::string m_strServerIP;				//������IP
	int m_nServerPort;						//�������˿�
	std::string m_strID;					//�˻�ID	
	std::string m_strLocalIP;				//����IP
	int m_nLocalPort;						//���ض˿�
	std::string m_strPassword;				//����
	int m_nExpire;							//����ʱ�䣬��λ����
	int m_nHeartInterval;					//�����������λ����
	int m_nHeartCount;						//ʧȥ����������Ϊ�Ͽ�

	//
	//״̬
	//

	//״̬ö��ֵ
	enum
	{
		eSInit = 0,			//��ʼ
		eSRegister,			//�ѷ���ע��
		eSHeart,			//�ѷ�������
		eSUnregister,			//�ѷ���ע��
	};	
	int m_nState;						//��ǰ״̬
	DWORD m_dwTime;						//��ǰ״̬��ʱ��
	bool m_bAuth;						//�ϴ�ע���Ƿ��Ѿ�����ע����Ϣ
	DWORD m_dwRegOK;					//���һ��ע��ɹ���ʱ��
	std::string m_strCallID;			//���һ�ε������CallID
	DWORD m_nFaildCount;				//�ȴ�����Ӧ��ʧ�ܼ���
	std::string m_strRegCallID;			//��ǰע���Call-ID
	std::string m_strRegFromTag;		//��ǰע���Tag

	//ע��״̬�ı�ص������ͻ�������
	CT_Register m_pfn;
	LPVOID m_lpContext;

	static std::atomic<int> m_nRegisterCount;


protected:

	//��״̬�Ĵ�����
	void OnInit();
	void OnRegister();
	void OnHeart();
	void OnUnRegister();

	//����������Ϣ�Ĵ�����
	void OnRegisterR(CMsgRBase *pCmd, CSubMsg *pSubMsg);
	void OnHeartR(CMsgRBase *pCmd, CSubMsg *pSubMsg);
	
	//���õ�ǰ����״̬
	void SetState(int nState);

	//����ע��״̬
	void SetRegister(bool bRegister);

	//����ע����Ϣ
	void Register(
		int nExpire,
		const std::string &strNonce = "", 
		const std::string &strRealm = "",
		const std::string &strFromTag = "",
		const std::string &strCallID = "",
		const std::string &strOpaque = ""
		);

	//����������Ϣ
	void Heart();

};

}
