#pragma once
#include "UdpM.h"

namespace Tool
{

	// 全局sip 交互udp类
	class CUdpM_G
	{
	public:
		CUdpM_G() { bInit = false; };
		~CUdpM_G() {};

		static CUdpM_G &GetInstance() { return s_udp; }

		std::shared_ptr<CUdpM> GetUdpInstance();
		bool Init();


		void SetParam(const std::string &strLocalIP, int nLocalPort, const std::string &strServerIP, int nServerPort)
		{
			m_strLocalIP = strLocalIP;
			m_strServerIP = strServerIP;
			m_nLocalPort = nLocalPort;
			m_nServerPort = nServerPort;
		}
		std::string GetLocalIP() { return m_strLocalIP; }
		std::string GetServerIP() { return m_strServerIP; }
		int GetLocalPort() { return m_nLocalPort; }
		int GetServerPort() { return m_nServerPort; }

	public:
		std::shared_ptr<CUdpM> m_pUdpM;

	protected:
		//静态实例对象
		static CUdpM_G s_udp;
		bool bInit;


		std::string m_strLocalIP;
		int			m_nLocalPort;
		std::string m_strServerIP;
		int			m_nServerPort;

	};
};

