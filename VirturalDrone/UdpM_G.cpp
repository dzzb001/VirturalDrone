#include "stdafx.h"
#include "UdpM_G.h"

namespace Tool {
	//¾²Ì¬ÊµÀý¶ÔÏó
	CUdpM_G CUdpM_G::s_udp;

	bool CUdpM_G::Init() {
		m_pUdpM = CUdpM::Create<CUdpM>();
		bInit = true;
		return true;
	}

	std::shared_ptr<CUdpM> CUdpM_G::GetUdpInstance() {
		if (!bInit)
			return nullptr;
		return m_pUdpM;
	}
}
