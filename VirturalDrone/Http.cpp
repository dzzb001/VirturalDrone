#include "stdafx.h"
#include "Http.h"

#ifdef _WIN32
#include "Tool\TLog.h"
#include "Tool\XEvent.h"
#include ".\zlib\zlib.h"
#pragma comment(lib, ".\\zlib\\x64\\zlib.lib")
#else
#include "Tool/TLog.h"
#include "Tool/XEvent.h"
#include "./zlib/zlib.h"
#endif

#include <sstream>
#include <regex>


#define Log LogN(232)

CHttp::CHttp() : m_bConneted(false)
, m_hEvent(nullptr)
, m_hReciveData(nullptr)
, m_lpTcpClient(nullptr)
, m_bPaserHead(false)
, m_nRecvMsgLen(0)
, m_nContentLen(0)
{
}

CHttp::~CHttp()
{
	DisConnect();
	Log(Tool::Debug, "~CHttp()");
}

int CHttp::Connect(std::shared_ptr<CMonitor> p,std::string strIp, unsigned short nPort, const char *url,int nMilliseconds)
{
	if (p == nullptr)
		return -1;

	m_ip = strIp;
	m_url = url;
	m_nPort = nPort;

	//����tcp client
	m_lpTcpClient = CTcpClient::Create<CTcpClient>();
 	m_lpTcpClient->RegCallback(tcpDataCb_s, m_pThis.lock(), tcpConnectCb, m_pThis.lock());
	m_lpTcpClient->SetMonitor(p);

	
	if (!m_lpTcpClient->Start(strIp.c_str(), nPort, 1024 * 1024))
	{
		Log(Tool::Error, "���ӷ�����ʧ��");
		m_lpTcpClient->QStop();
		return -1;
	}

	m_hEvent = std::make_shared<XEvent>(true);
	m_hReciveData = std::make_shared<XEvent>(true);

	m_hEvent->Reset();
	m_hReciveData->Reset();

	if(!m_hEvent->TryWait(nMilliseconds))
	{
		Log(Tool::Error, "���ӷ�������ʱ");
		m_lpTcpClient->QStop();
		return -1;
	}


	m_bConneted = true;
	return 0;
}
int CHttp::DisConnect()
{
	if (m_lpTcpClient) {
		m_lpTcpClient->QStop();
	}
	m_lpTcpClient.reset();
	m_lpTcpClient = nullptr;
	m_bConneted = false;

	if (m_hEvent) {
		m_hEvent.reset();
		m_hEvent = nullptr;
	}

	if (m_hReciveData) {
		m_hReciveData.reset();
		m_hReciveData = nullptr;
	}
	return 0;
}

int CHttp::Post(const char *SendData, int nDataLen, const char* cDataType, int nMilliseconds, const char *Authorization)
{
	//HTTP ͷ
	std::stringstream stream;
	stream << "POST " << m_url.c_str() << " HTTP/1.1\r\n";
	stream << "Host: " << m_ip.c_str() << ":" << m_nPort << "\r\n";
	stream << "Accept: */*\r\n";

	if(Authorization)
		stream << "Authorization: "<< Authorization << "\r\n";

	stream << "Connection: close\r\n";
	stream << "Content-Type: " << cDataType << "\r\n";
	stream << "Content-Length: " << nDataLen << "\r\n\r\n";
	std::string strHead = stream.str();

	std::string strSendImpl;

	strSendImpl.resize(strHead.size() + nDataLen);
	memcpy(&strSendImpl[0], &strHead[0], strHead.size());

	if(nDataLen>0)
		memcpy(&strSendImpl[strHead.size()], SendData, nDataLen);

	if (m_bConneted)
	{
		long ret = m_lpTcpClient->Send(strSendImpl.c_str(), strSendImpl.size());
		if (ret != strSendImpl.size())
			Log(Tool::Error, "���������������ʧ��");
	}

	if(!m_hReciveData->TryWait(nMilliseconds))
	{
		Log(Tool::Error, "�������ݳ�ʱ");
		return -1;
	}
	else {
		m_hReciveData->Reset();
	}

	return 0;
}

int CHttp::Post(std::vector<std::pair<std::string, std::string>> head, const char *SendData, int nDataLen, int nMilliseconds)
{
	//HTTP ͷ
	std::stringstream stream;
	stream << "POST " << m_url.c_str() << " HTTP/1.1\r\n";
	stream << "Host: " << m_ip.c_str() << ":" << m_nPort << "\r\n";
	for (int i = 0; i < head.size(); i++)
	{
		stream << head[i].first.c_str() << ": " << head[i].second.c_str() << "\r\n";
	}
	stream << "Content-Length: " << nDataLen << "\r\n\r\n";

	std::string strHead = stream.str();

	std::string strSendImpl;

	strSendImpl.resize(strHead.size() + nDataLen);
	memcpy(&strSendImpl[0], &strHead[0], strHead.size());

	if (nDataLen>0)
		memcpy(&strSendImpl[strHead.size()], SendData, nDataLen);

	if (m_bConneted)
	{
		long ret = m_lpTcpClient->Send(strSendImpl.c_str(), strSendImpl.size());
		if (ret != strSendImpl.size())
			Log(Tool::Error, "���������������ʧ��");
	}

	if (!m_hReciveData->TryWait(nMilliseconds))
	{
		Log(Tool::Error, "�������ݳ�ʱ");
		return -1;
	}
	else {
		m_hReciveData->Reset();
		return 0;
	}
}

int CHttp::Put(const char *SendData, int nDataLen, const char* cDataType, int nMilliseconds, const char *Authorization)
{
	//HTTP ͷ
	std::stringstream stream;
	stream << "PUT " << m_url.c_str() << " HTTP/1.1\r\n";
	stream << "Host: " << m_ip.c_str() << ":" << m_nPort << "\r\n";
	stream << "Accept: */*\r\n";
	stream << "Connection: close\r\n";

	if (Authorization)
		stream << "Authorization: " << Authorization << "\r\n";

	stream << "accept-encoding: gzip, deflate\r\n";
	stream << "Content-Type: " << cDataType << "\r\n";
	stream << "Content-Length: " << nDataLen << "\r\n\r\n";
	std::string strHead = stream.str();

	std::string strSendImpl;

	strSendImpl.resize(strHead.size() + nDataLen);
	memcpy(&strSendImpl[0], &strHead[0], strHead.size());

	if (nDataLen>0)
		memcpy(&strSendImpl[strHead.size()], SendData, nDataLen);

	if (m_bConneted)
	{
		long ret = m_lpTcpClient->Send(strSendImpl.c_str(), strSendImpl.size());
		if (ret != strSendImpl.size())
			Log(Tool::Error, "���������������ʧ��");
	}

	if (!m_hReciveData->TryWait(nMilliseconds))
	{
		Log(Tool::Error, "�������ݳ�ʱ");
		return -1;
	}
	else {
		m_hReciveData->Reset();
	}
	return 0;
}
int CHttp::Get(int nMilliseconds,const char *Authorization)
{
	//HTTP ͷ
	std::stringstream stream;
	stream << "GET " << m_url.c_str() << " HTTP/1.1\r\n";
	stream << "Host: " << m_ip.c_str() << ":" << m_nPort << "\r\n";
	stream << "Accept: */*\r\n";
	if (Authorization)
		stream << "Authorization: " << Authorization << "\r\n";
	stream << "Connection: close\r\n";
	stream << "Accept-Language: zh-CN,zh;q=0.9\r\n\r\n";
	std::string strHead = stream.str();

	if (m_bConneted)
	{
		long ret = m_lpTcpClient->Send(strHead.c_str(), strHead.size());
		if (ret != strHead.size())
			Log(Tool::Error, "Get ������ʧ��");
	}

	if(!m_hReciveData->TryWait(nMilliseconds))
	{
		Log(Tool::Error, "�������ݳ�ʱ");
		return -1;
	}
	else {
		m_hReciveData->Reset();
	}
	return 0;
}

int CHttp::Get(std::vector<std::pair<std::string, std::string>> params, int nMilliseconds, const char *Authorization)
{
	//HTTP ͷ
	std::stringstream stream;
	if(params.size()==0)
		stream << "GET " << m_url.c_str() << " HTTP/1.1\r\n";
	else {
		std::string strParams;
		std::stringstream streamParam;
		streamParam << "?";
		for (int i = 0; i < params.size(); i++)
		{
			streamParam << params[i].first.c_str() << "=" << params[i].second.c_str() << "&";
		}
		strParams = streamParam.str();
		strParams = strParams.substr(0, strParams.size() - 1);

		stream << "GET " << m_url.c_str() << strParams .c_str() << " HTTP/1.1\r\n";
	}
	stream << "Host: " << m_ip.c_str() << ":" << m_nPort << "\r\n";
	stream << "Accept: */*\r\n";
	if (Authorization)
		stream << "Authorization: " << Authorization << "\r\n";
	stream << "Connection: close\r\n";
	stream << "Accept-Language: zh-CN,zh;q=0.9\r\n\r\n";
	std::string strHead = stream.str();

	if (m_bConneted)
	{
		long ret = m_lpTcpClient->Send(strHead.c_str(), strHead.size());
		if (ret != strHead.size())
			Log(Tool::Error, "Get ������ʧ��");
	}

	if(!m_hReciveData->TryWait(nMilliseconds))
	{
		Log(Tool::Error, "�������ݳ�ʱ");
		return -1;
	}
	else {
		m_hReciveData->Reset();
	}
	return 0;
}

int CHttp::Get(std::vector<std::pair<std::string, std::string>> params, std::vector<std::pair<std::string, std::string>> head, int nMilliseconds)
{
	//HTTP ͷ
	std::stringstream stream;
	if (params.size() == 0)
		stream << "GET " << m_url.c_str() << " HTTP/1.1\r\n";
	else {
		std::string strParams;
		std::stringstream streamParam;
		streamParam << "?";
		for (int i = 0; i < params.size(); i++)
		{
			streamParam << params[i].first.c_str() << "=" << params[i].second.c_str() << "&";
		}
		strParams = streamParam.str();
		strParams = strParams.substr(0, strParams.size() - 1);

		stream << "GET " << m_url.c_str() << strParams.c_str() << " HTTP/1.1\r\n";
	}
	stream << "Host: " << m_ip.c_str() << ":" << m_nPort << "\r\n";

	for (int i = 0; i < head.size(); i++)
	{
		stream << head[i].first.c_str() << ": " << head[i].second.c_str() << "\r\n";
	}

	stream << "\r\n";

	std::string strHead = stream.str();
	if (m_bConneted)
	{
		long ret = m_lpTcpClient->Send(strHead.c_str(), strHead.size());
		if (ret != strHead.size())
			Log(Tool::Error, "Get ������ʧ��");
	}

	if (!m_hReciveData->TryWait(nMilliseconds))
	{
		Log(Tool::Error, "�������ݳ�ʱ");
		return -1;
	}
	else {
		m_hReciveData->Reset();
	}
	return 0;
}
int CHttp::tcpDataCb_s(std::shared_ptr<CContextBase> pContext, Tool::TBuff<char> &Data)
{
	auto pThis = (CHttp *)pContext.get();

	if (pThis)
	{
		return pThis->tcpDataCb(Data);
	}
	return Data.size();
}

int CHttp::tcpDataCb(Tool::TBuff<char> &Data)
{
	int nSize = ProcessMsg(Data);
	return nSize;
	//return std::make_pair(true, nSize);
}

int CHttp::ProcessMsg(Tool::TBuff<char> &Data)
{
	if (!m_bPaserHead)
	{
		std::string strTmp = &Data[0];

		size_t pos = strTmp.find("\r\n\r\n");//�ж�head�Ƿ�������

		if (pos <= 0)
			return 0;

		int nHeadLen = m_httpRespone.PaserHttpHead(&Data[0]);
		m_bPaserHead = true;

		//���Ȳ鿴���������Ƿ���chunked
		if (m_httpRespone.GetHead("Content-Length") != "")
		{
			//��msg ��Content-Length
			m_nContentLen = atoi(m_httpRespone.GetHead("Content-Length").c_str());
			Log("Content-Length %d", m_nContentLen);
			if (Data.size() > nHeadLen)
			{
				int nCurLen = Data.size() - nHeadLen;
				m_nRecvMsgLen = nCurLen;
				m_bufReciveContent.append(&Data[nHeadLen], nCurLen);
				//Log("m_nRecvMsgLen %d [%s]", m_nRecvMsgLen, &m_httpRespone.m_bufRecvData[0]);
				if (m_nRecvMsgLen == m_nContentLen)
				{
					//m_bufReciveContent.append(&m_httpRespone.m_bufRecvData[0], m_httpRespone.m_bufRecvData.size());
					Unzip_();
					if (m_hReciveData)
						m_hReciveData->Set();
					m_nRecvMsgLen = 0;
					//���ս���
				}
				return Data.size();
			}
		}else if (m_httpRespone.GetHead("Transfer-Encoding") == "chunked" || m_httpRespone.GetHead("transfer-encoding") == "chunked")
		{
			//��msg ��chunked
			Tool::TBuff<char> m_bufTmp;
			int nSeek = 0;
			if (Data.size() > nHeadLen)
			{
				int nCurLen = Data.size() - nHeadLen;

				m_bufTmp.append(&Data[nHeadLen], nCurLen);

				std::string strReciveTmp; //δ�����chunked����
#if 0
				strReciveTmp = (&m_bufTmp[0]);
				int nChunked = 0;

				std::string::size_type nStart = strReciveTmp.find_first_of("\r\n");
				if (std::string::npos == nStart)  //msg chunked ͷδ��ȫ
					return nHeadLen;

				nStart = strReciveTmp.find_first_not_of("\r\n");
				//����chunked
				while (std::string::npos != nStart)
#else
				std::string::size_type nStart;
				int nChunckLen = 0;//chunk���ݳ���
				do
				{
					strReciveTmp = (&m_bufTmp[nSeek]);

					nStart = strReciveTmp.find_first_of("\r\n");
					if (std::string::npos == nStart)  //msg chunked ͷδ��ȫ
						return nHeadLen;

					nStart = strReciveTmp.find_first_not_of("\r\n");
					std::string::size_type nEnd = strReciveTmp.find_first_of("\r\n", nStart);
					
					std::string strLen = strReciveTmp.substr(nStart, nEnd - nStart);

					Log("nStart %d nEnd %d", nStart, nEnd);

					if (nEnd > 6)
					{
						/*if (m_hReciveData)
							m_hReciveData->Set();*/
						Log("chunked len too long %d��Incomplete data", nEnd);
						return /*nHeadLen + nSeek*/Data.size();
					}
#ifdef _WIN32
					sscanf_s(strLen.c_str(), "%x", &nChunckLen);
#else
					sscanf(strLen.c_str(), "%x", &nChunckLen);
#endif // _WIN32
					Log("chunk Len <%s -- %d>", strLen.c_str(), nChunckLen);
					//�жϴ�chunked�Ƿ�����
					if (nChunckLen == 0)
					{
						Unzip_();
						//chunks������ 
						if (m_hReciveData)
							m_hReciveData->Set();
						Log("chunks end"/*, m_httpRespone.m_strContent.c_str()*/);
						return Data.size();
					}

					nSeek += strLen.size();
					nSeek += 2;

					int nLen = m_bufTmp.size() - nSeek; //ʣ�µ����ݳ���

					if (nLen >= nChunckLen+2)//+2 �ѽ�β\r\n ����
					{
						m_bufReciveContent.append(&m_bufTmp[nSeek], nChunckLen);
						nSeek += nChunckLen;
						nSeek += 2; //first chunked ��������
					}
					else {
						nSeek -= strLen.size();
						nSeek -= 2;
						return nHeadLen + nSeek;
					}
					strReciveTmp.clear();
				} while (std::string::npos != nStart);
#endif
				return nHeadLen + nSeek;
			}
			else
				return 0;
		}
		else
		{
			//û��msg ���ս���
			if (m_hReciveData)
				m_hReciveData->Set();
			return pos;
		}
	}
	else //����ʣ������
	{
		if (m_httpRespone.GetHead("Content-Length") != "")
		{
			//int nRecv = m_httpRespone.m_bufRecvData.append(&Data[0],Data.size());
			int nRecv = Data.size();
			m_bufReciveContent.append(&Data[0], Data.size());
			m_nRecvMsgLen += nRecv;
			Log("Content-Length %d,m_nRecvMsgLen %d", m_nContentLen, m_nRecvMsgLen);
			if (m_nRecvMsgLen >= m_nContentLen)
			{
				//m_bufReciveContent.append(&m_httpRespone.m_bufRecvData[0], m_httpRespone.m_bufRecvData.size());
				Unzip_();
				if (m_hReciveData)
					m_hReciveData->Set();
				m_nRecvMsgLen = 0;
			}
			return Data.size();
		}
		else if (m_httpRespone.GetHead("Transfer-Encoding") == "chunked" || m_httpRespone.GetHead("transfer-encoding") == "chunked")
		{
			Tool::TBuff<char> m_bufTmp;
			int nSeek = 0;
			//��msg ��chunked
			m_bufTmp.append(Data);

			std::string strReciveTmp; //δ�����chunked����
			strReciveTmp = (&m_bufTmp[0]);
			int nChunked = 0;

			std::string::size_type nStart = strReciveTmp.find_first_of("\r\n");
			if (std::string::npos == nStart)  //msg chunked ͷδ��ȫ
				return 0;

			nStart = strReciveTmp.find_first_not_of("\r\n");

			//����chunked
			while (std::string::npos != nStart)
			{
				std::string::size_type nEnd = strReciveTmp.find_first_of("\r\n", nStart);

				int nChunckLen = 0;//chunk���ݳ���
				std::string strLen = strReciveTmp.substr(nStart, nEnd - nStart);

				//Log("nStart %d nEnd %d", nStart, nEnd);
#ifdef _WIN32
				sscanf_s(strLen.c_str(), "%x", &nChunckLen);
#else
				sscanf(strLen.c_str(), "%x", &nChunckLen);
#endif // _WIN32
				//Log("chunk Len <%s -- %d>", strLen.c_str(), nChunckLen);
				//�жϴ�chunked�Ƿ�����
				if (nChunckLen == 0)
				{
					Unzip_();
					//chunks������ 
					if (m_hReciveData)
						m_hReciveData->Set();
					Log("chunks end");
					return Data.size();
				}

				nSeek += strLen.size();
				nSeek += 2;

				int nLen = m_bufTmp.size() - nSeek; //ʣ�µ����ݳ���

				if (nLen >= nChunckLen + 2)//+2 �ѽ�β\r\n ����
				{
					m_bufReciveContent.append(&m_bufTmp[nSeek], nChunckLen);
				}
				else {
					nSeek -= strLen.size();
					nSeek -= 2;
					return nSeek;
				}

				nSeek += nChunckLen;
				nSeek += 2; //first chunked ��������

				//Log("nStart %d nEnd %d", nStart, nEnd);
				strReciveTmp = (&m_bufTmp[nSeek]);
				nStart = strReciveTmp.find_first_of("\r\n");
				if (std::string::npos == nStart)  //msg chunked ͷδ��ȫ
					return nSeek;
				strLen = strReciveTmp.substr(0, nStart);
				nStart = strReciveTmp.find_first_not_of("\r\n");
				//Log("+ %s", strLen.c_str());
				//Log("nStart %d nEnd %d", nStart, nEnd);
			}
			return nSeek;
		}
		else
			return Data.size();
	}
}
//tcp ����״̬�ص�
void CHttp::tcpConnectCb(std::shared_ptr<CContextBase> pContext, bool bConnected)
{
	auto pThis = (CHttp *)pContext.get();
	if (pThis)
	{
		if (bConnected) {
			Log(Tool::Info, "Http client ���ӷ������ɹ�");
			if (pThis->m_hEvent)
				pThis->m_hEvent->Set();
		}
		else {
			pThis->m_bConneted = false;
			Log(Tool::Info, "Http client ����������ӶϿ�");
		}
		return;
	}
}
bool CHttp::Unzip(const unsigned char *bufGzip, int nLen, std::string &bufOut)
{
	//��ʼ�������
	z_stream strm = { 0 };
	int ret = inflateInit2(&strm, 47);
	if (Z_OK != ret)
	{
		//������ʼ��ʧ��
		return false;
	}

	//���´���������
	strm.avail_in = nLen;
	strm.next_in = (unsigned char *)bufGzip;

	//ѭ�����н���ֱ�����������û�б�����
#define CHUNK 16384
	bufOut.clear();
	unsigned char buff[CHUNK];
	do
	{
		strm.avail_out = CHUNK;
		strm.next_out = buff;
		ret = inflate(&strm, Z_NO_FLUSH);
		switch (ret)
		{
		case Z_STREAM_ERROR:    //����������
		case Z_NEED_DICT:      //��Ҫ����ʵ�
		case Z_DATA_ERROR:      //���ݴ���
		case Z_MEM_ERROR:      //�ڴ����
			inflateEnd(&strm);
			return false;
		}
		bufOut.append((char *)buff, CHUNK - strm.avail_out);
	} while (0 == strm.avail_out);

	//�رս�����
	inflateEnd(&strm);
	return true;
}

bool CHttp::Unzip_()
{
	if (m_httpRespone.GetHead("Content-Encoding") == "gzip")
	{
		std::string strUnzipData;
		bool bZip = Unzip((const unsigned char *)m_bufReciveContent.c_str(), m_bufReciveContent.size(), strUnzipData);
		if (bZip) {
			m_httpRespone.m_strContent.clear();
			m_httpRespone.m_strContent.append(strUnzipData.c_str(), strUnzipData.size());
			//Log("recv chunks format data: %s", m_httpRespone.m_strContent.c_str());
		}
		else
		{
			Log("Unzip failed");
			return false;
		}
	}
	else {
		m_httpRespone.m_strContent.clear();
		m_httpRespone.m_strContent.append(m_bufReciveContent.c_str(), m_bufReciveContent.size());
	}
	return true;
}

int CHttpRespone::PaserHttpHead(char *buf)
{
	std::string strHead = buf;
	int nHeadLength = 0;
	int pos = strHead.find("\r\n\r\n");

	if (pos > 0)
	{
		nHeadLength = pos + 4;
		strHead = strHead.substr(0, nHeadLength);
		if (Parse2Line(strHead)) {
			if (!ParseFirstLine(m_strFirstLine))
			{
				Log("����ͷ������ʧ��");
			}
		}
		else {
			Log("����ͷ��ʧ��");
		}
	}
	return nHeadLength;
}

//�������к͸�ͷ��
bool CHttpRespone::Parse2Line(const std::string &strMsg)
{
	char buff[2048];
	std::stringstream ss(strMsg);
	bool bFirst = true;
	while (ss.getline(buff, 2048))
	{
		std::string strLine = buff;
		strLine.erase(0, strLine.find_first_not_of(" \t\r\n"));
		strLine.erase(strLine.find_last_not_of(" \t\r\n") + 1);
		if (strLine.empty())
		{
			if (bFirst)
			{
				//��Ϣǰ��Ŀհ���
				continue;
			}
			//ͷ�������
			break;
		}
		if (bFirst)
		{
			m_strFirstLine = strLine;
			bFirst = false;
			//Log("���У�<%s>", m_strFirstLine.c_str());
			continue;
		}
		std::string::size_type nColon = strLine.find(':');
		if (std::string::npos == nColon)
		{
			Log("�����Ҳ���ͷ������<%s>", strLine.c_str());
			continue;
		}
		std::string strHead = strLine.substr(0, nColon);
		std::string strContent = GetContent(strLine, strHead, ":", "\r\n");
		//Log("<%s> - <%s>", strHead.c_str(), strContent.c_str());
		m_vecHead.push_back(std::make_pair(strHead, strContent));
	}
	return !m_vecHead.empty();
}

//��������
//�������к͸�ͷ��
bool CHttpRespone::ParseFirstLine(const std::string &strLine)
{
	std::regex r("^[[:s:]]*HTTP/1.[0-1][[:s:]]+([0-9]+)[[:s:]]+([[:print:]]+)$");
	//std::regex r("^[[:s:]]*HTTP/1.[0-1][[:s:]]+([0-9]+)");
	std::smatch m;
	if (std::regex_match(strLine, m, r))
	{
		m_FirstLine.Reply = static_cast<HttpResponse>(std::atoi(m[1].str().c_str()));
		m_FirstLine.strReply = m[2].str();
		Log("parse first line %d %s", m_FirstLine.Reply, m_FirstLine.strReply.c_str());
		return true;
	}

	std::regex r2("^[[:s:]]*HTTP/1.[0-1][[:s:]]+([0-9]+)");
	//std::smatch m;
	if (std::regex_match(strLine, m, r2))
	{
		m_FirstLine.Reply = static_cast<HttpResponse>(std::atoi(m[1].str().c_str()));
		m_FirstLine.strReply = m[2].str();
		Log("parse first line %d %s", m_FirstLine.Reply, m_FirstLine.strReply.c_str());
		return true;
	}
	return false;
}

//��ȡͷ����������ַ���
std::string CHttpRespone::GetContent(const std::string &strMsg, std::string strName, const std::string &strPrefix,
	const std::string &strSuffix, const std::string &strSuffixSubby /* = "" */)
{
	strName.append(strPrefix);
	std::string::size_type nStart = strMsg.find(strName);
	if (std::string::npos == nStart)
	{
		return std::string();
	}
	nStart += strName.size();
	std::string::size_type nEnd = strMsg.find(strSuffix, nStart);
	if (std::string::npos == nEnd && !strSuffixSubby.empty())
	{
		nEnd = strMsg.find(strSuffixSubby, nStart);
	}
	if (std::string::npos == nEnd)
	{
		std::string strEnd = "\r\n";
		nEnd = strMsg.find(strEnd, nStart);
	}
	if (std::string::npos == nEnd)
	{
		nEnd = strMsg.size();
	}
	std::string strRet = strMsg.substr(nStart, nEnd - nStart);
	strRet.erase(0, strRet.find_first_not_of(" "));
	strRet.erase(strRet.find_last_not_of(" ") + 1);
	return strRet;
}

//��ȡָ��ͷ���ֵ
std::string CHttpRespone::GetHead(std::string strHead)
{
	std::transform(strHead.begin(), strHead.end(), strHead.begin(), ::tolower);
	for (size_t i = 0; i < m_vecHead.size(); ++i)
	{
		std::string strNodeHead = m_vecHead[i].first;
		std::transform(strNodeHead.begin(), strNodeHead.end(), strNodeHead.begin(), ::tolower);
		if (strNodeHead == strHead)
		{
			return m_vecHead[i].second;
		}
	}
	return std::string();
}

//��ȡָ��ͷ���ֵ
void CHttpRespone::GetHead(std::string strHead, std::vector<std::string> &vecValue)
{
	std::transform(strHead.begin(), strHead.end(), strHead.begin(), ::tolower);
	for (size_t i = 0; i < m_vecHead.size(); ++i)
	{
		std::string strNodeHead = m_vecHead[i].first;
		std::transform(strNodeHead.begin(), strNodeHead.end(), strNodeHead.begin(), ::tolower);
		if (strNodeHead == strHead)
		{
			vecValue.push_back(m_vecHead[i].second);
		}
	}
	return ;
}