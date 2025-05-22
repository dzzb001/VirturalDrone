// 2008-09-23
// XFile.h
// guoshanhe
// 文件系统操作类

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
		X_TYPE_NONE			= 0x00000000,		// 没有
		X_TYPE_DIR			= 0x00000001,		// 目录
		X_TYPE_REGULAR		= 0x00000002,		// 普通文件
		X_TYPE_LINK			= 0x00000004,		// 符号连接
		X_TYPE_CHAR			= 0x00000008,		// 字符特殊文件
		X_TYPE_BLOCK		= 0x00000010,		// 块特殊文件
		X_TYPE_FIFO			= 0x00000020,		// 管道
		X_TYPE_SOCKET		= 0x00000040,		// 套接字
		X_TYPE_OTHER		= 0x00000080,		// 其他类型
		X_TYPE_ALL			= 0x000000FF,		// 所有文件类型

		X_ATTR_HIDE			= 0x00000100,		// 隐藏
		X_ATTR_SYSTEM		= 0x00000200,		// 系统文件
		X_ATTR_ALL			= 0x00000300		// 所有属性
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
	// 创建目录，若路径中有目录不存在，将失败
	static BOOL MakeDir(const string &strDirPath);
	// 创建目录，若路径中有目录不存在，将自动补全
	static BOOL MakeFullDir(const string &strDirPath);
	// 删除空目录
	static BOOL RemoveEmptyDir(const string &strDirPath);
	// 删除目录，会递归删除目录内子目录和文件
	static BOOL RemoveDir(const string &strDirPath);
	// 删除除了目录外的所有类型文件
	static BOOL RemoveFile(const string &strFilePath);
	// (linux)创建软连接，若路径中有目录不存在，将失败
	static BOOL MakeLink(const string &strTargetPath, const string &strNewPath);
	// (linux)创建软连接，若路径中有目录不存在，将自动补全
	static BOOL MakeFullLink(const string &strTargetPath, const string &strNewPath);

	// 判断文件类型
	static BOOL IsLink(const string &strpath, /*out*/string &strTargetPath);
	static BOOL IsDir(const string &strPath);
	static BOOL IsRegular(const string &strPath);
	static BOOL IsExist(const string &strPath);
	
	// 解析路径为目录路径和文件名，支持绝对路径和相对路径，目录路径和文件名均可能为空字符串
	static void ParseFilePath(const string &strFilePath, /*out*/string &strDirPath, /*out*/string &strFileName);
	// 解析文件名为文件名前缀和后缀部分，文件名也可以是路径，但是将舍弃目录路径部分
	// windows平台下，以最后一个'.'后面部分为后缀
	// linux平台下，隐藏文件(第一个字符为'.')第二个'.'前面部分为前缀，后面为后缀，非隐藏文件第一个'.'前面部分为前缀，后面为后缀
	static void ParseFileName(const string &strFileName, /*out*/string &strPerfix, /*out*/string &strPostfix);

	// 获取和设置当前进程的工作目录
	static string GetCurrentDir();
	static BOOL SetCurrentDir(const string &strDir);
	// 获取本模块路径和名字
	static BOOL GetModuleFile(string &strDirPath, string &strName);
	// 获取系统目录
	static string GetSystemDir();
};

} // namespace xbase

using namespace xbase;

#endif//_X_FILE_H_
