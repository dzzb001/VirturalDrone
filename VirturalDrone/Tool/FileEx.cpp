#include "stdafx.h"
#include "FileEx.h"
#include <algorithm>
#include <string.h>

namespace Tool
{

#ifndef _WINDOWS_
int CFileEx::m_snDepth = -1;
bool CFileEx::m_sbFile = true;	//TRUE for file and FALSE for folder
std::vector<std::string> CFileEx::m_svecFile;
#else
#include <shlobj.h>
#endif

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取文件分隔符。
* 输入参数：	
* 输出参数：	
* 返 回 值：	对于windows返回'\\'，linux返回'/'。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-03-01	周锋	      创建
*******************************************************************************/
char CFileEx::Separator()
{
#ifdef _WINDOWS_
	return '\\';
#else
	return '/';
#endif
}


/*******************************************************************************
* 函数名称：	
* 功能描述：	获取当前目录。
* 输入参数：	
* 输出参数：	
* 返 回 值：	当前目录全路径，不包括末尾的“\\”或“/”。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-05	周锋	      创建
*******************************************************************************/
std::string CFileEx::GetCurDirectory()
{
	std::string strRet;
#ifdef _WINDOWS_
	char buff[256];
	GetCurrentDirectory(256, buff);
	strRet = buff;
#else
	strRet = get_current_dir_name();
#endif
	return strRet;
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取可执行程序所在目录。
* 输入参数：	
* 输出参数：	
* 返 回 值：	返回可执行程序所在目录，返回值不包括末尾的“\\”或“/”。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-05	周锋	      创建
*******************************************************************************/
std::string CFileEx::GetExeDirectory()
{
	std::string strRet;
	char buff[256] = {0};
#ifdef _WINDOWS_
	GetModuleFileName(NULL, buff, 256);
#else
	readlink("/proc/self/exe", buff, 256); 
#endif
	strRet = buff;
	strRet = strRet.substr(0, strRet.rfind(Separator()));
	return strRet;
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	设置当前工作目录。
* 输入参数：	lpszFolder	-- 待设置的工作目录。
* 输出参数：	
* 返 回 值：	执行成功返回TRUE，执行失败返回FALSE。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-02-26	周锋	      创建
*******************************************************************************/
bool CFileEx::SetCurDirectory(const char * lpszFolder)
{
#ifdef _WIN32
	return !!SetCurrentDirectory(lpszFolder);
#else
	return (chdir(lpszFolder) == 0);
#endif
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	创建指定的多级文件目录。
* 输入参数：	
* 输出参数：	
* 返 回 值：	在windows环境中，如果目录创建成功或目录已存在返回true，否则返回false。
*				linux环境中总是返回true。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-05	周锋	      创建
*******************************************************************************/
bool CFileEx::CreateFolder(const char * lpszFolder)
{
	if (0 == lpszFolder)
	{
		return false;
	}
	std::string strFolder = lpszFolder;
	if (strFolder.empty())
	{
		return false;
	}
	if (Separator() != strFolder[0] && std::string::npos == strFolder.find(':'))
	{
		std::string strCurDir = GetCurDirectory();
		strCurDir += Separator();
		strFolder.insert(strFolder.begin(), strCurDir.begin(), strCurDir.end());
	}
#ifdef _WINDOWS_
	int nRet = SHCreateDirectoryEx(
		NULL, 
		strFolder.c_str(),
		NULL
		);
	return ERROR_SUCCESS == nRet || ERROR_ALREADY_EXISTS == nRet;
#else
	std::string strCmd = "mkdir -p \"" + strFolder + std::string("\"");
	system(strCmd.c_str());
	return true;
#endif
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	为创建指定的文件创建必要的文件目录。
* 输入参数：	
* 输出参数：	
* 返 回 值：	在windows环境中，如果目录创建成功或目录已存在返回true，否则返回false。
*				linux环境中总是返回true。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-05	周锋	      创建
*******************************************************************************/
bool CFileEx::CreateFolderForFile(const char * lpszFile)
{
	std::string strFolder = lpszFile;
	strFolder = strFolder.substr(0, strFolder.rfind(Separator()));
	return CreateFolder(strFolder.c_str());
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取指定目录下的所有文件（不包括目录）。
* 输入参数：	
* 输出参数：	
* 返 回 值：	
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-05	周锋	      创建
*******************************************************************************/
void CFileEx::GetSubFiles(const char * lpszFolder, std::vector<std::string> &vecFile)
{
	std::string strFolder = lpszFolder;
	if (strFolder.empty())
	{
		return;
	}
	if (*strFolder.rbegin() == Separator())
	{
		strFolder.erase(strFolder.length(), 1);
	}
	vecFile.clear();
#ifdef _WINDOWS_
	strFolder += "\\*.*";
	CFileFind finder;
	BOOL bWorking = finder.FindFile(strFolder.c_str());
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDirectory() || finder.IsDots())
		{
			continue;
		}
		vecFile.push_back(std::string((const char *)finder.GetFilePath()));
	}
#else
	m_svecFile.clear();
	m_snDepth = -1;
	m_sbFile = true;
	ftw(strFolder.c_str(), FileFunc, 500);
	vecFile = m_svecFile;
#endif
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取指定目录下的所有目录（不包括文件）。
* 输入参数：	
* 输出参数：	
* 返 回 值：	
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-05	周锋	      创建
*******************************************************************************/
void CFileEx::GetSubFoders(const char * lpszFolder, std::vector<std::string> &vecFolder)
{
	std::string strFolder = lpszFolder;
	if (strFolder.empty())
	{
		return;
	}
	if (*strFolder.rbegin() == Separator())
	{
		strFolder.erase(strFolder.length(), 1);
	}
	vecFolder.clear();
#ifdef _WINDOWS_
	strFolder += "\\*.*";
	CFileFind finder;
	BOOL bWorking = finder.FindFile(strFolder.c_str());
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (!finder.IsDirectory() || finder.IsDots())
		{
			continue;
		}
		vecFolder.push_back(std::string((const char *)finder.GetFilePath()));
	}
#else
	m_svecFile.clear();
	m_snDepth = -1;
	m_sbFile = false;
	ftw(strFolder.c_str(), FileFunc, 500);
	vecFolder = m_svecFile;
#endif
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	根据全路径获取文件名。
* 输入参数：	
* 输出参数：	
* 返 回 值：	返回获取的文件名。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-05	周锋	      创建
*******************************************************************************/
std::string CFileEx::Path2FileName(const char *lpszPath)
{
	std::string strRet = lpszPath;
	if (strRet.empty())
	{
		return strRet;
	}
	if (*strRet.rbegin() == Separator())
	{
		strRet.erase(strRet.length() - 1);
	}
	std::string::size_type pos = strRet.rfind(Separator());
	if (std::string::npos == pos)
	{
		return strRet;
	}
	return strRet.substr(pos + 1);
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取磁盘剩余空间。
* 输入参数：	lpszPath -- 磁盘目录。
* 输出参数：	
* 返 回 值：	返回磁盘剩余空间的大小，获取失败返回0。单位MB。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2007-11-17	周锋	      创建
*******************************************************************************/
unsigned int CFileEx::GetFreeDiskSpace(const char* lpszPath)
{
#ifdef _WINDOWS_
	//检测硬盘空间
	ULARGE_INTEGER ulUserFree;
	ULARGE_INTEGER ulTotal;
	if(!GetDiskFreeSpaceEx(lpszPath, &ulUserFree, &ulTotal, NULL))
	{
		//监测硬盘失败，一般是因为存盘路径不存在
		return 0;
	}
	else
	{
		unsigned int nMb = 1024 * 1024;
		unsigned int nUserFree = ulUserFree.HighPart * 4096 + ulUserFree.LowPart / nMb;
		//UINT nTotal = ulTotal.HighPart * 4096 + ulTotal.LowPart / nMb;
		return nUserFree;
	}
	return 0;
#else
	struct statfs stVfs;
	memset( &stVfs, 0, sizeof( stVfs ) );
	if( -1 == statfs(lpszPath, &stVfs ) )
	{
		return 0;
	}
	return (unsigned int)(stVfs.f_bfree / 1024 / 1024 * stVfs.f_bsize);
#endif
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	删除指定的文件夹。
* 输入参数：	
* 输出参数：	
* 返 回 值：	执行成功返回TRUE，执行失败返回FALSE。
* 其它说明：	循环删除里面所有的文件内容。
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-05	周锋	      创建
*******************************************************************************/
bool CFileEx::DelFolder(const char * lpszFolder)
{
#ifdef _WINDOWS_
	CString strPath = CString(lpszFolder) + '\0';
	SHFILEOPSTRUCT fs;
	fs.hwnd = NULL;
	fs.wFunc = FO_DELETE;
	fs.pFrom = strPath;
	fs.pTo = NULL;
	fs.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	fs.fAnyOperationsAborted = TRUE;
	return (0 == SHFileOperation(&fs)); 
#else
	std::string strCmd = std::string("rm -rfd \"") + lpszFolder + std::string("\"");
	system(strCmd.c_str());
	return true;
#endif
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	删除指定的文件。
* 输入参数：	
* 输出参数：	
* 返 回 值：	执行成功返回true，执行失败返回false。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2008-01-07	周锋	      创建
*******************************************************************************/
bool CFileEx::DelFile(const char* lpszPath)
{
#ifdef _WINDOWS_
	CFileStatus fs;
	if (!CFile::GetStatus(lpszPath, fs))
	{
		return false;
	}
	fs.m_attribute = 0;
	CFile::SetStatus(lpszPath, fs);
	return !!DeleteFile(lpszPath);
#else
	return 0 == remove(lpszPath);
#endif
}



#ifndef _WINDOWS_
int CFileEx::FileFunc(const char *file, const struct stat *sb, int flag)
{
	int nDepth = std::count(file, file + strlen(file), '/');
	if (-1 == m_snDepth)
	{
		m_snDepth = nDepth;
	}
	if (nDepth - m_snDepth == 1)
	{
		if (m_sbFile && FTW_F == flag || !m_sbFile && FTW_D == flag)
		{
			m_svecFile.push_back(std::string(file));
		}
	}
	return 0;
}
#endif

}

