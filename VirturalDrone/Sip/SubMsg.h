#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace Sip
{


//sip����������
enum
{
	//������ĳ�ʼֵ
	eSubCmdNone = 0,
	eSubCmdKeepAlive,			//��������
	eSubCmdCatalogQ,			//Ŀ¼��ѯ
	eSubCmdCatalogR,			//Ŀ¼Ӧ��
	eSubCmdSDP,					//SDP
	eSubCmdAlarmQ,				//������ѯ
	eSubCmdAlarmN,				//�����ϱ�
	eSubCmdAlarmR,				//����ȷ��Ӧ��
	eSubCmdRecordInfo,			//¼���ļ���ѯ
	eSubCmdRecordInfoR,			//¼���ļ���ѯӦ��
	eSubCmdRTSP,				//¼�񲥷ſ���
	eSubCmdDeviceInfoQ,			//�豸��ѯ
	eSubCmdDeviceInfoR			//�豸��ѯӦ��
};

//
enum
{
	eStatusOK = 0,
};

//��Ϣ����
class CSubMsg
{
public:
	CSubMsg(void);

	virtual ~CSubMsg(void);

	//�����������
	static int CheckType(std::string &strContentType, const std::string &strMsg);

	//��������
	virtual bool Parse(const std::string &strMsg) = 0;

	//��������
	virtual void Make() = 0;

	//��ȡ�����ַ���
	std::vector<std::string> &Str(){return m_vecMsgStr;}

	//��ȡ��ص��豸��ͨ��ID
	std::string &ID(){return m_strID;}

public:

	//��������
	int m_nType;

	//�������
	int m_nSN;

	//��ص�ID
	std::string m_strID;

	//�����ַ���
	std::vector<std::string> m_vecMsgStr;
};

//����������Ϣ
class CSubMsgKeepAlive : public CSubMsg
{
public:

	CSubMsgKeepAlive();

	~CSubMsgKeepAlive();

	//���ò���
	void SetParam(
		int nSN,
		const std::string &strDeviceID,
		int nState
		);

	//����
	virtual bool Parse(const std::string &strMsg){return false;}

	//����
	virtual void Make();

public:
	int m_nStatus;
};

//Ŀ¼��ѯ
class CSubMsgCatalogQ : public CSubMsg
{
public:

	CSubMsgCatalogQ();

	~CSubMsgCatalogQ();

	//����
	virtual bool Parse(const std::string &strMsg);

	//����
	virtual void Make(){}	
};

//Ŀ¼��Ϣ�ڵ�
struct CatalogNode
{
	std::string strDeviceID;//��ѡ�����
	std::string strName;//��ѡ������
	std::string strManufacturer;//��ѡ��Ϊ�豸ʱ�豸����
	std::string strModel;//��ѡ���ͺ�
	std::string strOwner;//��ѡ����Ϊ�豸ʱ���豸����
	std::string strCivilCode;//��ѡ����������
	std::string strBlock;//��ѡ������
	std::string strAddress;//��ѡ����Ϊ�豸ʱ����װ��ַ
	int nParental;//�Ƿ������豸��1���У�0��û��
	std::string strParentID;//��ѡ�����豸ID���и��豸ʱ���
	int nSafetyWay;//��ѡ�����ȫģʽ��ȱʡΪ0,0�������ã�2��S/MIMEǩ����ʽ 3��S/MIME����ǩ��ͬ�²��÷�ʽ 4������ժҪ��ʽ
	int nRegisterWay;//��ѡ��ȱʡΪ1�� 1������sip3261��׼����֤ע��ģʽ 2�����ڿ����˫����֤ע��ģʽ 3����������֤���˫����֤ע��ģʽ
	std::string strCertNmu;//֤�����кţ���֤����豸��ѡ
	int nCertifiable;//֤����Ч��ʶ����֤����豸��ѡ��ȱʡΪ0,0����Ч 1����Ч
	int nErrCode;//֤����Чԭ���룬��֤������Ч���豸��ѡ
	std::string strEndTime;//֤����ֹ��Ч��
	int nSecrecy;//��ѡ���������ԣ�ȱʡΪ0��0�������� 1������
	std::string strIP;//��ѡ��IP��ַ
	int nPort;//��ѡ���豸�˿�
	std::string strPassword;//��ѡ���豸����
	std::string strStatus;//��ѡ�豸״̬�� tg:statusType ֵΪON��OFF
	double fLongitude;//��ѡ������
	double fLatitude;//��ѡ��γ��

	CatalogNode()
		: nParental(0)
		, nSafetyWay(0)
		, nRegisterWay(1)
		, nCertifiable(0)
		, nErrCode(1)
		, nSecrecy(0)
		, nPort(0)
		, strStatus("ON")
		, strManufacturer("Syz")
	{
		strManufacturer = "Syz";
		strModel = "1";
		strOwner = "Owner";
		strCivilCode = "CivilCode";
		strAddress = "Address";

	}
};
typedef std::vector<CatalogNode> CatalogVec;

//Ŀ¼�ϱ�
class CSubMsgCatalogR : public CSubMsg
{
public:

	CSubMsgCatalogR();
	~CSubMsgCatalogR();

	//���ò���
	void SetParam(
		int nSN,
		const std::string strDeviceID,
		const CatalogVec &vecCatalog
		);

	//����
	virtual bool Parse(const std::string &strMsg){return false;}

	//����
	virtual void Make();

protected:

	//Ŀ¼�б�
	CatalogVec m_vecCatalog;
};

//SDP
class CSubMsgSdp : public CSubMsg
{
public:

	CSubMsgSdp();

	~CSubMsgSdp();

	//���ò���
	void SetParam(
		const std::string &strID,					//�豸ID
		const std::string &strIP,					//��ַ
		const std::string &strPassword,				//����
		const std::string &strType,					//recvonly��sendonly
		const std::string &strProtocol,				//����Э��
		const std::string &strMode,					//active ���� passive
		int nRtpPort,								//��Ƶ���ͻ��߽���Rtp�˿�
		const std::string &strPlayType,				//Play��ʵʱ�㲥 Playback����ʷ�ط� DownLoad���ļ�����
		const std::string &strSSRC,					//SSRC
		const std::string &strPT,					//�������ͣ���H264��PS
		int nPT										//PTֵ
		);

	//����
	virtual bool Parse(const std::string &strMsg);

	//����
	virtual void Make();

public:

	//�ֶ���Ϣ
	std::string m_strPassword;//����
	std::string m_strIP;//��ַ
	std::string m_strSSRC;//SSRC
	std::string m_strType;//recvonly��sendonly
	std::unordered_map<int, std::string> m_mapRtp;
	int m_nRtpPort;
	std::string m_strPlayType;//Play��ʵʱ�㲥 Playback����ʷ�ط� DownLoad���ļ�����
	int m_nPSPT;//PS���͸��ص�PTֵ
	int m_nH264PT;//H264��UNLA�������͵�PTֵ
	std::string m_strProtocol;	//����Э�� �� RTP/AVP ���� TCP/RTP/AVP
	std::string m_strMode;		//active ���� passive
	std::string m_strConnection;	//�μ�IETFRFC4571 TCP�½������������ӣ��̶�����new

	std::string m_strStartTime;		//¼�񲥷���ʼʱ�� ʱ���
	std::string m_strEndTime;		//¼�񲥷�ֹͣʱ�� ʱ���

protected:

	//�����б�
	std::vector<std::pair<std::string, std::string> > m_vecLine;
	
	//������������
	typedef void (*CT_Parse)(
		CSubMsgSdp *pThis,
		const std::string &strHead,
		const std::string &strContent
		);

	//���������б�
	std::unordered_map<std::string, CT_Parse> m_mapParseFun;

protected:

	//���еĽ�������
	static void ParseO(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseC(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseM(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseA(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseY(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseS(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseT(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);


	//��ָ���Ĺ����ȡֵ
	static std::string GetContent(
		const std::string &strMsg,				//ԭʼ�ַ���
		std::string strName,					//Ҫ���ҵ�ǰ������
		const std::string &strPrefix,			//ǰ������ֵ֮����ַ�
		const std::string &strSuffix,			//�����ַ�
		const std::string &strSuffixSubby = ""	//�����ȼ������ַ�
		);
};

struct AlarmNode
{
	std::string strPriority;	//��������1-4��
	std::string strMethod;		//������ʽ��1�绰������2�豸������3���ű�����4GPS������5��Ƶ������6�豸���ϱ�����7��������
	std::string strTime;		//����ʱ�� 2016-05-01T12:10:59
	std::string strDescript;	//��������
	std::string strLonitude;	//��������23.6
	std::string strLatitude;	//γ��
	std::vector<std::string> vecInfo;	//��չ��Ϣ	
};

//�����ϱ�
class CSubMsgCatalogN : public CSubMsg
{
public:

	CSubMsgCatalogN();

	~CSubMsgCatalogN();

	//���ò���
	void SetParam(
		const std::string &strID,
		int nSN,
		const AlarmNode &node
		);

	//����
	virtual bool Parse(const std::string &strMsg){return false;}

	//����
	virtual void Make();

protected:

	//������Ϣ
	AlarmNode m_Alarm;
};

//����Ӧ��
class CSubMsgAlarmR : public CSubMsg
{
public:

	CSubMsgAlarmR();

	~CSubMsgAlarmR();

	//����
	virtual bool Parse(const std::string &strMsg);

	//����
	virtual void Make(){}

protected:

	//Ӧ����������OK
	std::string m_strResult;
};

//¼���ѯ
class CSubMsgRecordInfo : public CSubMsg 
{
public:
	CSubMsgRecordInfo() {
		m_nType = eSubCmdRecordInfo;
	};
	~CSubMsgRecordInfo() {};

	//����
	virtual bool Parse(const std::string &strMsg);

	//����
	virtual void Make() {}

public:
	std::string m_strStartTime;				//¼����ʼʱ��
	std::string m_strEndTime;				//¼�����ʱ��
	std::string m_strType;					//¼������
};

//¼���ļ��ϱ�
class CSubMsgRecordInfoR : public CSubMsg 
{
public:
	CSubMsgRecordInfoR() {
		m_nType = eSubCmdRecordInfoR;
	};
	~CSubMsgRecordInfoR() {};

	//����
	virtual bool Parse(const std::string &strMsg);

	//����
	virtual void Make() {}	
};

//¼����� INFO ��Ϣ���� 
class CSubMsgInfo : public CSubMsg
{
public:
	CSubMsgInfo() {
		m_nType = eSubCmdRTSP;
	};
	~CSubMsgInfo() {};

	//����
	virtual bool Parse(const std::string &strMsg);

	//����
	virtual void Make();

public:
	//¼���������
	std::string m_strMode;						// PLAY PAUSE
	std::string m_strSeq;
	std::string	m_strScale;						// �����ٶ�
};

//�豸��Ϣ��ѯ
class CSubMsgDeviceInfoQ : public CSubMsg
{
public:

	CSubMsgDeviceInfoQ();

	~CSubMsgDeviceInfoQ();

	//����
	virtual bool Parse(const std::string& strMsg);

	//����
	virtual void Make() {}
};

//�豸��Ϣ��ѯ��Ӧ
class CSubMsgDeviceInfoR : public CSubMsg
{
public:

	CSubMsgDeviceInfoR();
	~CSubMsgDeviceInfoR();

	//���ò���
	void SetParam(
		int nSN,
		const std::string strDeviceID,
		const CatalogVec& vecCatalog
	);

	//����
	virtual bool Parse(const std::string& strMsg) { return false; }

	//����
	virtual void Make();

protected:

	//Ŀ¼�б�
	CatalogVec m_vecCatalog;
};


}
