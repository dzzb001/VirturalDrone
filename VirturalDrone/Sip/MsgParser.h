#pragma once
#include "SipMsg.h"
#include "SubMsg.h"

namespace Sip
{

class CMsgParser
{
public:

	CMsgParser(void);

	~CMsgParser(void);

	//��������
	static std::pair<CMsg*, CSubMsg*> Parser(char *pBuff, int nLen);
	
};

}