// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#ifdef _WIN32

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // �� Windows ͷ���ų�����ʹ�õ�����
#endif

#include <afx.h>
#include <afxwin.h>         // MFC ��������ͱ�׼���
#include <afxext.h>         // MFC ��չ
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC �� Internet Explorer 4 �����ؼ���֧��
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC �� Windows �����ؼ���֧��
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <atlbase.h>
#include <atlstr.h>

#include <iostream>


#endif // _WIN32

// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�

#include <string.h>
#include "Tool/TLog.h"

