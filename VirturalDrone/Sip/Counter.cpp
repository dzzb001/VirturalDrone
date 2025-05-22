#include "stdafx.h"
#include "Counter.h"
#include <unordered_map>

CCounter CCounter::s_Counter;

CCounter::CCounter()
{

}

CCounter::~CCounter()
{

}

int CCounter::Get(const std::string &strName)
{
	int nRet = 1;
	m_lock.Lock();
	std::unordered_map<std::string, int>::iterator it = m_mapCounter.find(strName);
	if (m_mapCounter.end() == it)
	{
		m_mapCounter[strName] = 1;
		it = m_mapCounter.find(strName);
	}
	if (it->second <= 0)
	{
		it->second = 1;
	}
	nRet = it->second++;	
	m_lock.UnLock();
	return nRet;
}
