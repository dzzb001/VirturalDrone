#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace Sip
{


//sip命令子类型
enum
{
	//无意义的初始值
	eSubCmdNone = 0,
	eSubCmdKeepAlive,			//保持心跳
	eSubCmdCatalogQ,			//目录查询
	eSubCmdCatalogR,			//目录应答
	eSubCmdSDP,					//SDP
	eSubCmdAlarmQ,				//报警查询
	eSubCmdAlarmN,				//报警上报
	eSubCmdAlarmR,				//报警确认应答
	eSubCmdRecordInfo,			//录像文件查询
	eSubCmdRecordInfoR,			//录像文件查询应答
	eSubCmdRTSP,				//录像播放控制
	eSubCmdDeviceInfoQ,			//设备查询
	eSubCmdDeviceInfoR			//设备查询应答
};

//
enum
{
	eStatusOK = 0,
};

//消息基类
class CSubMsg
{
public:
	CSubMsg(void);

	virtual ~CSubMsg(void);

	//检查命令类型
	static int CheckType(std::string &strContentType, const std::string &strMsg);

	//解析命令
	virtual bool Parse(const std::string &strMsg) = 0;

	//生成命令
	virtual void Make() = 0;

	//获取命令字符串
	std::vector<std::string> &Str(){return m_vecMsgStr;}

	//获取相关的设备或通道ID
	std::string &ID(){return m_strID;}

public:

	//命令类型
	int m_nType;

	//命令序号
	int m_nSN;

	//相关的ID
	std::string m_strID;

	//命令字符串
	std::vector<std::string> m_vecMsgStr;
};

//心跳主动消息
class CSubMsgKeepAlive : public CSubMsg
{
public:

	CSubMsgKeepAlive();

	~CSubMsgKeepAlive();

	//设置参数
	void SetParam(
		int nSN,
		const std::string &strDeviceID,
		int nState
		);

	//解析
	virtual bool Parse(const std::string &strMsg){return false;}

	//生成
	virtual void Make();

public:
	int m_nStatus;
};

//目录查询
class CSubMsgCatalogQ : public CSubMsg
{
public:

	CSubMsgCatalogQ();

	~CSubMsgCatalogQ();

	//解析
	virtual bool Parse(const std::string &strMsg);

	//生成
	virtual void Make(){}	
};

//目录信息节点
struct CatalogNode
{
	std::string strDeviceID;//必选，编号
	std::string strName;//必选，名称
	std::string strManufacturer;//必选，为设备时设备厂商
	std::string strModel;//必选，型号
	std::string strOwner;//必选，当为设备时，设备归属
	std::string strCivilCode;//必选，行政区域
	std::string strBlock;//可选，警区
	std::string strAddress;//必选，当为设备时，安装地址
	int nParental;//是否有子设备，1：有，0：没有
	std::string strParentID;//可选，父设备ID（有父设备时必填）
	int nSafetyWay;//可选，信令安全模式，缺省为0,0：不采用，2：S/MIME签名方式 3：S/MIME加密签名同事采用方式 4：数字摘要方式
	int nRegisterWay;//必选：缺省为1， 1：符合sip3261标准的认证注册模式 2：基于口令的双向认证注册模式 3：基于数字证书的双向认证注册模式
	std::string strCertNmu;//证书序列号，有证书的设备必选
	int nCertifiable;//证书有效标识，有证书的设备必选，缺省为0,0：无效 1：有效
	int nErrCode;//证书无效原因码，有证书且无效的设备必选
	std::string strEndTime;//证书终止有效期
	int nSecrecy;//必选：保密属性，缺省为0；0：不涉密 1：涉密
	std::string strIP;//可选，IP地址
	int nPort;//可选，设备端口
	std::string strPassword;//可选，设备口令
	std::string strStatus;//必选设备状态， tg:statusType 值为ON、OFF
	double fLongitude;//可选，经度
	double fLatitude;//可选，纬度

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

//目录上报
class CSubMsgCatalogR : public CSubMsg
{
public:

	CSubMsgCatalogR();
	~CSubMsgCatalogR();

	//设置参数
	void SetParam(
		int nSN,
		const std::string strDeviceID,
		const CatalogVec &vecCatalog
		);

	//解析
	virtual bool Parse(const std::string &strMsg){return false;}

	//生成
	virtual void Make();

protected:

	//目录列表
	CatalogVec m_vecCatalog;
};

//SDP
class CSubMsgSdp : public CSubMsg
{
public:

	CSubMsgSdp();

	~CSubMsgSdp();

	//设置参数
	void SetParam(
		const std::string &strID,					//设备ID
		const std::string &strIP,					//地址
		const std::string &strPassword,				//密码
		const std::string &strType,					//recvonly、sendonly
		const std::string &strProtocol,				//传输协议
		const std::string &strMode,					//active 或者 passive
		int nRtpPort,								//视频发送或者接收Rtp端口
		const std::string &strPlayType,				//Play：实时点播 Playback：历史回放 DownLoad：文件下载
		const std::string &strSSRC,					//SSRC
		const std::string &strPT,					//负载类型，如H264、PS
		int nPT										//PT值
		);

	//解析
	virtual bool Parse(const std::string &strMsg);

	//生成
	virtual void Make();

public:

	//字段信息
	std::string m_strPassword;//密码
	std::string m_strIP;//地址
	std::string m_strSSRC;//SSRC
	std::string m_strType;//recvonly、sendonly
	std::unordered_map<int, std::string> m_mapRtp;
	int m_nRtpPort;
	std::string m_strPlayType;//Play：实时点播 Playback：历史回放 DownLoad：文件下载
	int m_nPSPT;//PS类型负载的PT值
	int m_nH264PT;//H264的UNLA负载类型的PT值
	std::string m_strProtocol;	//传输协议 ： RTP/AVP 或者 TCP/RTP/AVP
	std::string m_strMode;		//active 还是 passive
	std::string m_strConnection;	//参见IETFRFC4571 TCP新建还是重用连接，固定采用new

	std::string m_strStartTime;		//录像播放起始时间 时间戳
	std::string m_strEndTime;		//录像播放停止时间 时间戳

protected:

	//各行列表
	std::vector<std::pair<std::string, std::string> > m_vecLine;
	
	//解析函数定义
	typedef void (*CT_Parse)(
		CSubMsgSdp *pThis,
		const std::string &strHead,
		const std::string &strContent
		);

	//解析函数列表
	std::unordered_map<std::string, CT_Parse> m_mapParseFun;

protected:

	//各行的解析函数
	static void ParseO(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseC(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseM(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseA(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseY(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseS(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);
	static void ParseT(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent);


	//按指定的规则获取值
	static std::string GetContent(
		const std::string &strMsg,				//原始字符串
		std::string strName,					//要查找的前导名字
		const std::string &strPrefix,			//前导名和值之间的字符
		const std::string &strSuffix,			//结束字符
		const std::string &strSuffixSubby = ""	//次优先级结束字符
		);
};

struct AlarmNode
{
	std::string strPriority;	//报警级别（1-4）
	std::string strMethod;		//报警方式：1电话报警；2设备报警；3短信报警；4GPS报警；5视频报警；6设备故障报警；7其他报警
	std::string strTime;		//报警时间 2016-05-01T12:10:59
	std::string strDescript;	//报警描述
	std::string strLonitude;	//精度例如23.6
	std::string strLatitude;	//纬度
	std::vector<std::string> vecInfo;	//扩展信息	
};

//报警上报
class CSubMsgCatalogN : public CSubMsg
{
public:

	CSubMsgCatalogN();

	~CSubMsgCatalogN();

	//设置参数
	void SetParam(
		const std::string &strID,
		int nSN,
		const AlarmNode &node
		);

	//解析
	virtual bool Parse(const std::string &strMsg){return false;}

	//生成
	virtual void Make();

protected:

	//报警信息
	AlarmNode m_Alarm;
};

//报警应答
class CSubMsgAlarmR : public CSubMsg
{
public:

	CSubMsgAlarmR();

	~CSubMsgAlarmR();

	//解析
	virtual bool Parse(const std::string &strMsg);

	//生成
	virtual void Make(){}

protected:

	//应答结果，比如OK
	std::string m_strResult;
};

//录像查询
class CSubMsgRecordInfo : public CSubMsg 
{
public:
	CSubMsgRecordInfo() {
		m_nType = eSubCmdRecordInfo;
	};
	~CSubMsgRecordInfo() {};

	//解析
	virtual bool Parse(const std::string &strMsg);

	//生成
	virtual void Make() {}

public:
	std::string m_strStartTime;				//录像起始时间
	std::string m_strEndTime;				//录像结束时间
	std::string m_strType;					//录像类型
};

//录像文件上报
class CSubMsgRecordInfoR : public CSubMsg 
{
public:
	CSubMsgRecordInfoR() {
		m_nType = eSubCmdRecordInfoR;
	};
	~CSubMsgRecordInfoR() {};

	//解析
	virtual bool Parse(const std::string &strMsg);

	//生成
	virtual void Make() {}	
};

//录像控制 INFO 消息处理 
class CSubMsgInfo : public CSubMsg
{
public:
	CSubMsgInfo() {
		m_nType = eSubCmdRTSP;
	};
	~CSubMsgInfo() {};

	//解析
	virtual bool Parse(const std::string &strMsg);

	//生成
	virtual void Make();

public:
	//录像控制命令
	std::string m_strMode;						// PLAY PAUSE
	std::string m_strSeq;
	std::string	m_strScale;						// 播放速度
};

//设备信息查询
class CSubMsgDeviceInfoQ : public CSubMsg
{
public:

	CSubMsgDeviceInfoQ();

	~CSubMsgDeviceInfoQ();

	//解析
	virtual bool Parse(const std::string& strMsg);

	//生成
	virtual void Make() {}
};

//设备信息查询响应
class CSubMsgDeviceInfoR : public CSubMsg
{
public:

	CSubMsgDeviceInfoR();
	~CSubMsgDeviceInfoR();

	//设置参数
	void SetParam(
		int nSN,
		const std::string strDeviceID,
		const CatalogVec& vecCatalog
	);

	//解析
	virtual bool Parse(const std::string& strMsg) { return false; }

	//生成
	virtual void Make();

protected:

	//目录列表
	CatalogVec m_vecCatalog;
};


}
