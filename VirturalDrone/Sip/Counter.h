#pragma once
#include <string>
#include <unordered_map>
#include "../Tool/XMutex.h"

class CCounter
{
public:
	CCounter(void);

	~CCounter(void);

	static CCounter & GetInstanse()
	{
		return s_Counter;
	}

	int Get(const std::string &strName);
	
protected:

	//∑√Œ ±£ª§
	XMutex m_lock;
	std::unordered_map<std::string, int> m_mapCounter;
	static CCounter s_Counter;
};
