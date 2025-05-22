/*******************************************************************************

    File : BaseDefine.h
    
*******************************************************************************/
#ifndef MEGA_BASE_DEFINE_H
#define MEGA_BASE_DEFINE_H


#include <string.h>
#include <stdlib.h>
#include <stdio.h>


//------------------------------------------------------------------------------
// define base type
//------------------------------------------------------------------------------
typedef unsigned char       BYTE;
typedef unsigned char       UCHAR;
typedef unsigned char       uchar;

typedef unsigned short      WORD;
typedef unsigned short      USHORT;
typedef unsigned short      ushort;

typedef unsigned int        UINT;
typedef unsigned int        uint;

typedef unsigned long       DWORD;
typedef unsigned long       ULONG;
typedef unsigned long       ulong;

typedef int SOCKET;
typedef int HANDLE;

#ifndef NULL
#define NULL (0)
#endif

//------------------------------------------------------------------------------
// define function call back value
//------------------------------------------------------------------------------
#define RET_OK                          0
#define RET_ERR                         -1
#define RET_BLOCK                       -101
#define RET_TIMEOUT                     -102
#define RET_CONTENT_ERR                 -103

//------------------------------------------------------------------------------
// define others
//------------------------------------------------------------------------------
#define LOG_QUEUE_SIZE                  2000
#define CONDITION_WAIT_MILSECOND        5000
#define SOCKET_REACTOR_EVENT_SIZE       10000


enum
{
	PLAY_TYPE_INVALID = 0x00,
	PLAY_TYPE_RTSP = 0x01,
	PLAY_TYPE_SIP = 0x02,
	PLAY_TYPE_FTP = 0x04,
	PLAY_TYPE_TCP = 0x08,
	PLAY_TYPE_HXHT = 0x10
};

inline int get_play_type(int val)
{
	if(val & PLAY_TYPE_RTSP)
		return PLAY_TYPE_RTSP;
	else if(val & PLAY_TYPE_SIP)
		return PLAY_TYPE_SIP;
	else if(val & PLAY_TYPE_FTP)
		return PLAY_TYPE_FTP;
	else if(val & PLAY_TYPE_TCP)
		return PLAY_TYPE_TCP;
	else if(val & PLAY_TYPE_HXHT)
		return PLAY_TYPE_HXHT;

	return PLAY_TYPE_INVALID;
}

inline const char* get_play_type_str(int val)
{
	switch (val)
	{
	case PLAY_TYPE_RTSP:
		return "RTSP";
	case PLAY_TYPE_SIP:
		return "SIP";
	case PLAY_TYPE_FTP:
		return "FTP";
	case PLAY_TYPE_TCP:
		return "TCP";
	case PLAY_TYPE_HXHT:
		return "HXHT";
	default:
		break;
	}
	
	return "";
}


#endif
