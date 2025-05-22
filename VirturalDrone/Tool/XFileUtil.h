// 2008-09-23
// XFile.h
// guoshanhe
// �ļ�ϵͳ������

#pragma once

#ifndef _X_FILE_H_
#define _X_FILE_H_

#include "XDefine.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XFileFinder
////////////////////////////////////////////////////////////////////////////////
class XFileFinder
{
public:
	enum
	{
		X_TYPE_NONE			= 0x00000000,		// û��
		X_TYPE_DIR			= 0x00000001,		// Ŀ¼
		X_TYPE_REGULAR		= 0x00000002,		// ��ͨ�ļ�
		X_TYPE_LINK			= 0x00000004,		// ��������
		X_TYPE_CHAR			= 0x00000008,		// �ַ������ļ�
		X_TYPE_BLOCK		= 0x00000010,		// �������ļ�
		X_TYPE_FIFO			= 0x00000020,		// �ܵ�
		X_TYPE_SOCKET		= 0x00000040,		// �׽���
		X_TYPE_OTHER		= 0x00000080,		// ��������
		X_TYPE_ALL			= 0x000000FF,		// �����ļ�����

		X_ATTR_HIDE			= 0x00000100,		// ����
		X_ATTR_SYSTEM		= 0x00000200,		// ϵͳ�ļ�
		X_ATTR_ALL			= 0x00000300		// ��������
	};

public:
	XFileFinder();
	~XFileFinder();

	BOOL FindFirst(const string& strSearchPath, uint32 mask = X_TYPE_ALL | X_ATTR_ALL);
	int  FindNext(string& strName);
	void FindClose();

private:
	uint32				m_mask;
	
#ifdef __WINDOWS__
	HANDLE				m_hFind;
	WIN32_FIND_DATA		m_FindData;
#endif//__WINDOWS__
#ifdef __GNUC__
	DIR*				m_pDir;
	struct dirent*		m_dirFile;
#endif//__GNUC__
};


////////////////////////////////////////////////////////////////////////////////
// XFileUtil
////////////////////////////////////////////////////////////////////////////////
class XFileUtil
{
public:
	// ����Ŀ¼����·������Ŀ¼�����ڣ���ʧ��
	static BOOL MakeDir(const string &strDirPath);
	// ����Ŀ¼����·������Ŀ¼�����ڣ����Զ���ȫ
	static BOOL MakeFullDir(const string &strDirPath);
	// ɾ����Ŀ¼
	static BOOL RemoveEmptyDir(const string &strDirPath);
	// ɾ��Ŀ¼����ݹ�ɾ��Ŀ¼����Ŀ¼���ļ�
	static BOOL RemoveDir(const string &strDirPath);
	// ɾ������Ŀ¼������������ļ�
	static BOOL RemoveFile(const string &strFilePath);
	// (linux)���������ӣ���·������Ŀ¼�����ڣ���ʧ��
	static BOOL MakeLink(const string &strTargetPath, const string &strNewPath);
	// (linux)���������ӣ���·������Ŀ¼�����ڣ����Զ���ȫ
	static BOOL MakeFullLink(const string &strTargetPath, const string &strNewPath);

	// �ж��ļ�����
	static BOOL IsLink(const string &strpath, /*out*/string &strTargetPath);
	static BOOL IsDir(const string &strPath);
	static BOOL IsRegular(const string &strPath);
	static BOOL IsExist(const string &strPath);
	
	// ����·��ΪĿ¼·�����ļ�����֧�־���·�������·����Ŀ¼·�����ļ���������Ϊ���ַ���
	static void ParseFilePath(const string &strFilePath, /*out*/string &strDirPath, /*out*/string &strFileName);
	// �����ļ���Ϊ�ļ���ǰ׺�ͺ�׺���֣��ļ���Ҳ������·�������ǽ�����Ŀ¼·������
	// windowsƽ̨�£������һ��'.'���沿��Ϊ��׺
	// linuxƽ̨�£������ļ�(��һ���ַ�Ϊ'.')�ڶ���'.'ǰ�沿��Ϊǰ׺������Ϊ��׺���������ļ���һ��'.'ǰ�沿��Ϊǰ׺������Ϊ��׺
	static void ParseFileName(const string &strFileName, /*out*/string &strPerfix, /*out*/string &strPostfix);

	// ��ȡ�����õ�ǰ���̵Ĺ���Ŀ¼
	static string GetCurrentDir();
	static BOOL SetCurrentDir(const string &strDir);
	// ��ȡ��ģ��·��������
	static BOOL GetModuleFile(string &strDirPath, string &strName);
	// ��ȡϵͳĿ¼
	static string GetSystemDir();
};

} // namespace xbase

using namespace xbase;

#endif//_X_FILE_H_
