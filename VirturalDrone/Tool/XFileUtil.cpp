// 2008-09-18
// File.cpp
// guoshanhe
// 文件系统操作类


#include "XFileUtil.h"
#include "XStrUtil.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XFileFinder
////////////////////////////////////////////////////////////////////////////////
#ifdef __WINDOWS__
XFileFinder::XFileFinder()
{
	m_hFind = INVALID_HANDLE_VALUE;
}

XFileFinder::~XFileFinder()
{
	FindClose();
}

BOOL XFileFinder::FindFirst(const string& strSearchPath, uint32 mask/* = X_TYPE_ALL | X_ATTR_ALL*/)
{
	m_mask = mask & (X_TYPE_ALL | X_ATTR_ALL);
	FindClose();
	
	string strTmp = strSearchPath;
	XStrUtil::Chop(strTmp, " \t\r\n\\");
	m_hFind = ::FindFirstFile((strSearchPath + "\\*").c_str(), &m_FindData);
	if (INVALID_HANDLE_VALUE == m_hFind)
	{
		return FALSE;
	}
	return TRUE;
}

int XFileFinder::FindNext(string& strName)
{
	if ((INVALID_HANDLE_VALUE == m_hFind) || (0 == (m_mask & X_TYPE_ALL)))
	{
		return X_TYPE_NONE;
	}

	while (::FindNextFile(m_hFind, &m_FindData))
	{
		if (strcmp(m_FindData.cFileName, ".") == 0 || strcmp(m_FindData.cFileName, "..") == 0)
		{
			continue;
		}
		if ((m_FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !(m_mask & X_ATTR_HIDE))
		{
			continue;
		}
		if ((m_FindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) && !(m_mask & X_ATTR_SYSTEM))
		{
			continue;
		}
		
		if(m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			strName = m_FindData.cFileName;
			return X_TYPE_DIR;
		}
		if (m_FindData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
		{
			strName = m_FindData.cFileName;
			return X_TYPE_REGULAR;
		}
		if (m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)
		{
			strName = m_FindData.cFileName;
			return X_TYPE_BLOCK;
		}

		strName = m_FindData.cFileName;
		return X_TYPE_OTHER;
	}
	return X_TYPE_NONE;
}

void XFileFinder::FindClose()
{
	if (m_hFind != INVALID_HANDLE_VALUE)
	{
		::FindClose(m_hFind);
		m_hFind = INVALID_HANDLE_VALUE;
	}
}
#endif//__WINDOWS__



////////////////////////////////////////////////////////////////////////////////
// XFileFinder
////////////////////////////////////////////////////////////////////////////////
#ifdef __GNUC__
XFileFinder::XFileFinder()
	: m_pDir(NULL)
	, m_dirFile(NULL)
{
	// empty
}

XFileFinder::~XFileFinder()
{
	FindClose();
}

BOOL XFileFinder::FindFirst(const string& strSearchPath, uint32 mask/* = X_TYPE_ALL | X_ATTR_ALL*/)
{
	m_mask = mask & (X_TYPE_ALL | X_ATTR_ALL);
	FindClose();

	m_pDir = ::opendir(strSearchPath.c_str());
	if(m_pDir == NULL)
	{
		return FALSE;
	}

	return TRUE;
}

int XFileFinder::FindNext(string& strName)
{
	if ((m_pDir == NULL) || (0 == (m_mask && X_TYPE_ALL)))
	{
		return X_TYPE_NONE;
	}

	while((m_dirFile = ::readdir(m_pDir)) != NULL)
	{
		if (strcmp(m_dirFile->d_name, ".") == 0 || strcmp(m_dirFile->d_name, "..") == 0)
		{
			continue;
		}

		// skip hide file
		if ((m_dirFile->d_name[0] == '.') && (0 == m_mask & X_ATTR_HIDE))
		{
			continue;
		}

		strName = m_dirFile->d_name;
		switch (m_dirFile->d_type)
		{
		case DT_DIR:
			return X_TYPE_DIR;
		case DT_REG:
			return X_TYPE_REGULAR;
		case DT_LNK:
			return X_TYPE_LINK;
		case DT_CHR:
			return X_TYPE_CHAR;
		case DT_BLK:
			return X_TYPE_BLOCK;
		case DT_FIFO:
			return X_TYPE_FIFO;
		case DT_SOCK:
			return X_TYPE_SOCKET;
		default:
			return X_TYPE_OTHER;
		}
	}
	return X_TYPE_NONE;
}

void XFileFinder::FindClose()
{
	if (m_pDir != NULL)
	{
		::closedir(m_pDir);
		m_pDir = NULL;
	}
}
#endif//__GNUC__


////////////////////////////////////////////////////////////////////////////////
// XFileUtil
////////////////////////////////////////////////////////////////////////////////
#ifdef __WINDOWS__

BOOL XFileUtil::MakeDir(const string &strDirPath)
{
	if (!CreateDirectory(strDirPath.c_str(), NULL))
	{
		if (ERROR_ALREADY_EXISTS == GetLastError())
		{
			return TRUE;
		}
		return	FALSE;
	}

	return TRUE;
}

BOOL XFileUtil::MakeFullDir(const string &strDirPath)
{
	vector<string> vItems;
	vector<string>::iterator it;
	string strPath = strDirPath;
	string strTmp;

	XStrUtil::Chop(strPath, "\r\n\t ");
	XStrUtil::Split(strPath, vItems, "\\/");
	if (vItems.size() == 0) return TRUE;
	if (strPath.size() >= 2 && strPath[1] == ':')
	{
		// 带盘符的绝对路径
		strTmp += strPath[0];
		strTmp += ":\\";
		vItems.erase(vItems.begin());
	}
	else if (XStrUtil::Compare("\\\\", strPath, 2) == 0)
	{
		// 局限网路径
		strTmp = "\\\\";
		strTmp += vItems[0];
		strTmp += '\\';
		vItems.erase(vItems.begin());
	}

	for (it = vItems.begin(); it != vItems.end(); it++)
	{
		strTmp += (*it);
		strTmp += '\\';
		if (!MakeDir(strTmp))
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL XFileUtil::RemoveEmptyDir(const string &strDirPath)
{
	return RemoveDirectory(strDirPath.c_str());
}

BOOL XFileUtil::RemoveDir(const string &strDirPath)
{
	XFileFinder finder;
	string strName;
	int type = XFileFinder::X_TYPE_NONE;
	string strTmp = strDirPath;

	if (!finder.FindFirst(strTmp))
	{
		return FALSE;
	}
	while ((type = finder.FindNext(strName)) != XFileFinder::X_TYPE_NONE)
	{
		if (type == XFileFinder::X_TYPE_DIR)
		{
			RemoveDir(strDirPath + '\\' + strName);
		}
		else
		{
			RemoveFile(strDirPath + '\\' + strName);
		}
	}
	finder.FindClose();
	return RemoveEmptyDir(strDirPath);
}

BOOL XFileUtil::RemoveFile(const string &strFilePath)
{
	return (BOOL)(0 == remove(strFilePath.c_str()));
}

BOOL XFileUtil::MakeLink(const string &strTargetPath, const string &strNewPath)
{
	return FALSE;
}

BOOL XFileUtil::MakeFullLink(const string &strTargetPath, const string &strNewPath)
{
	return FALSE;
}

BOOL XFileUtil::IsLink(const string &strpath, /*out*/string &strTargetPath)
{
	return FALSE;
}

BOOL XFileUtil::IsDir(const string &strPath)
{
	struct _stat statinfo;
	if (0 != ::_stat(strPath.c_str(), &statinfo))
	{
		return FALSE;
	}
	if (_S_IFDIR & statinfo.st_mode)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL XFileUtil::IsRegular(const string &strPath)
{
	struct _stat statinfo;
	if (0 != ::_stat(strPath.c_str(), &statinfo))
	{
		return FALSE;
	}
	if (_S_IFREG & statinfo.st_mode)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL XFileUtil::IsExist(const string &strPath)
{
	return (BOOL)(0 == ::_access_s(strPath.c_str(), 00));
}

string XFileUtil::GetCurrentDir()
{
	char buf[PATH_MAX] = {};
	if (0 == ::GetCurrentDirectory(PATH_MAX - 1, buf))
	{
		return "";
	}
	return buf;
}

BOOL XFileUtil::SetCurrentDir(const string &strDir)
{
	return SetCurrentDirectory(strDir.c_str());
}

BOOL XFileUtil::GetModuleFile(string &strDirPath, string &strName)
{
	char buf[PATH_MAX];
	if (0 == GetModuleFileName(GetModuleHandle(NULL), buf, PATH_MAX - 1))
	{
		strDirPath = "";
		strName = "";
		return FALSE;
	}
	ParseFilePath(buf, strDirPath, strName);
	return TRUE;
}

void XFileUtil::ParseFilePath(const string &strFilePath, /*out*/string &strDirPath, /*out*/string &strFileName)
{
	vector<string> vItems;
	string strTmp = strFilePath;
	XStrUtil::Chop(strTmp, "\r\n\t ");
	XStrUtil::Split(strTmp, vItems, "\\/");

	strDirPath = "";
	strFileName = "";
	if (strTmp.empty()) return;

	if (XStrUtil::Compare("\\\\", strTmp, 2) == 0)
	{
		if (vItems.size() == 0)
		{
			// such as "\\" or "\\\\\\\" or "\\\//\"
			return;
		}
		else if (vItems.size() == 1)
		{
			// such as "\\rose" or "\\192.168.1.1" or "\\rose\" or "\\\\\rose" or "\\." or "\\..\"
			strDirPath += "\\\\";
			strDirPath += vItems[0];
			return;
		}
		else if (strTmp[strTmp.size() - 1] == '\\' || 
			vItems[vItems.size() - 1] == "." || 
			vItems[vItems.size() - 1] == "..")
		{
			// such as "\\rose\svn\tmp\" or "\\rose\." or "\\rose\tmp\..\"
			strDirPath += "\\\\";
			for (size_t i = 0; i < (size_t)vItems.size(); i++)
			{
				strDirPath += vItems[i];
				strDirPath += "\\";
			}
			XStrUtil::ChopTail(strDirPath, "\\");
			return;
		}
		else
		{
			// such as "\\rose\svn\tmp.txt"
			strDirPath += "\\\\";
			for (size_t i = 0; i < (size_t)vItems.size() - 1; i++)
			{
				strDirPath += vItems[i];
				strDirPath += "\\";
			}
			XStrUtil::ChopTail(strDirPath, "\\");
			strFileName = vItems[vItems.size() - 1];
			return;
		}
	}
	else if (strTmp[0] == '\\')
	{
		// such as "\" or "\tmp.txt" or "\svn\tmp.txt"
		return;
	}

	if (vItems.size() == 1)
	{
		if (vItems[0].size() >= 2 && vItems[0][2] == ':')
		{
			// such as "c:" or "c:\"
			strDirPath += vItems[0][0];
			strDirPath += ':';
			return;
		}
		else if (strTmp[strTmp.size() - 1] == '\\' || 
			vItems[0] == "." || 
			vItems[0] == "..")
		{
			// such as "." or ".." or "..\" or "tmp\"
			strDirPath = vItems[0];
			return;
		}
		else
		{
			// such as "t" or "tmp.txt"
			strFileName = vItems[0];
			return;
		}
	}

	if (strTmp[strTmp.size() - 1] == '\\' ||
		vItems[vItems.size() - 1] == "." ||
		vItems[vItems.size() - 1] == "..")
	{
		// such as "c:\tmp\" or ".\tmp" or "svn\tmp\log" or "svn\tmp\."
		for (size_t i = 0; i < (size_t)vItems.size(); i++)
		{
			strDirPath += vItems[i];
			strDirPath += "\\";
		}
		XStrUtil::ChopTail(strDirPath, "\\");
		return;
	}
	else
	{
		for (size_t i = 0; i < (size_t)vItems.size() - 1; i++)
		{
			strDirPath += vItems[i];
			strDirPath += "\\";
		}
		XStrUtil::ChopTail(strDirPath, "\\");
		strFileName = vItems[vItems.size() - 1];
		return;
	}
	return;
}

void XFileUtil::ParseFileName(const string &strFileName, /*out*/string &strPerfix, /*out*/string &strPostfix)
{
	string strDirPath, strTmp;
	vector<string> vItems;

	strPerfix = "";
	strPostfix = "";
	ParseFilePath(strFileName, strDirPath, strTmp);
	XStrUtil::Chop(strTmp, " ");
	XStrUtil::Split(strTmp, vItems, ".");
	if (vItems.size() == 0) return;
	if (vItems.size() == 1)
	{
		strPerfix = vItems[0];
		return;
	}
	else if (vItems.size() == 2)
	{
		strPerfix = vItems[0];
		strPostfix = vItems[1];
		return;
	}
	else if (vItems.size() > 2)
	{
		for (size_t i = 0; i < vItems.size() - 1; i++)
		{
			strPerfix += vItems[i];
			strPerfix += '.';
		}
		XStrUtil::ChopTail(strPerfix, ".");
		strPostfix = vItems[vItems.size() - 1];
		return;
	}
	
	return;
}

string XFileUtil::GetSystemDir()
{
	char buf[PATH_MAX];
	UINT ret = GetSystemDirectory(buf, PATH_MAX - 1);
	if (ret >= PATH_MAX) return "";
	buf[ret] = '\0';
	return buf;
}

#endif//__WINDOWS__


////////////////////////////////////////////////////////////////////////////////
// XFileUtil
////////////////////////////////////////////////////////////////////////////////
#ifdef __GNUC__

BOOL XFileUtil::MakeDir(const string &strDirPath)
{
	if (-1 ==mkdir(strDirPath.c_str(), 0777))
	{
		if ((EEXIST == errno) && IsDir(strDirPath))
		{
			return TRUE;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL XFileUtil::MakeFullDir(const string &strDirPath)
{
	vector<string> vItems;
	vector<string>::iterator it;
	string strPath = strDirPath;
	string strTmp;

	XStrUtil::Chop(strPath, "\r\n\t ");
	XStrUtil::Split(strPath, vItems, "\\/");
	if (vItems.size() == 0) return TRUE;

	if (strPath[0] == '/') strTmp = "/";
	for (size_t i = 0; i < vItems.size(); i++)
	{
		strTmp += vItems[i];
		strTmp += '/';
		if (!MakeDir(strTmp)) return FALSE;
	}
	return TRUE;
}

BOOL XFileUtil::RemoveEmptyDir(const string &strDirPath)
{
	return (BOOL)(0 == rmdir(strDirPath.c_str()));
}

BOOL XFileUtil::RemoveDir(const string &strDirPath)
{
	XFileFinder finder;
	string strName;
	int type = XFileFinder::X_TYPE_NONE;
	string strTmp = strDirPath;

	if (!finder.FindFirst(strTmp))
	{
		return FALSE;
	}
	while ((type = finder.FindNext(strName)) != XFileFinder::X_TYPE_NONE)
	{
		if (type == XFileFinder::X_TYPE_DIR)
		{
			RemoveDir(strDirPath + '/' + strName);
		}
		else
		{
			RemoveFile(strDirPath + '/' + strName);
		}
	}
	finder.FindClose();
	return RemoveEmptyDir(strDirPath);
}

BOOL XFileUtil::RemoveFile(const string &strFilePath)
{
	return (BOOL)(0 == unlink(strFilePath.c_str()));
}

BOOL XFileUtil::MakeLink(const string &strTargetPath, const string &strNewPath)
{
	return (BOOL)(0 == symlink(strTargetPath.c_str(), strNewPath.c_str()));
}

BOOL XFileUtil::MakeFullLink(const string &strTargetPath, const string &strNewPath)
{
	string strDirPath, strFileName, strTmp;
	ParseFilePath(strNewPath, strDirPath, strFileName);
	MakeFullDir(strDirPath);
	if (0 != symlink(strTargetPath.c_str(), strNewPath.c_str()))
	{
		if (EEXIST == errno)
		{
			if (!IsLink(strNewPath, strTmp) || (strTmp != strTargetPath))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}


BOOL XFileUtil::IsLink(const string &strpath, /*out*/string &strTargetPath)
{
	char buf[PATH_MAX] = {};
	if (-1 == readlink(strpath.c_str(), buf, PATH_MAX))
	{
		return FALSE;
	}
	strTargetPath = buf;
	return TRUE;
}

BOOL XFileUtil::IsDir(const string &strPath)
{
	struct stat statinfo;
	if (0 != ::lstat(strPath.c_str(), &statinfo))
	{
		return FALSE;
	}
	if (!S_ISDIR(statinfo.st_mode))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XFileUtil::IsRegular(const string &strPath)
{
	struct stat statinfo;
	if (0 != ::lstat(strPath.c_str(), &statinfo))
	{
		return FALSE;
	}
	if (!S_ISREG(statinfo.st_mode))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XFileUtil::IsExist(const string &strPath)
{
	return (BOOL)(0 == ::access(strPath.c_str(), F_OK));
}

string XFileUtil::GetCurrentDir()
{
	char buf[PATH_MAX] = {};
	if (NULL == getcwd(buf, PATH_MAX - 1))
	{
		return "";
	}
	return buf;
}

BOOL XFileUtil::SetCurrentDir(const string &strDir)
{
	return (BOOL)(0 == chdir(strDir.c_str()));
}

BOOL XFileUtil::GetModuleFile(string &strDirPath, string &strName)
{
	int ret = -1;
	char buf[MAX_PATH];
	ret = readlink("/proc/self/exe", buf, MAX_PATH - 1);
	if (-1 == ret)
	{
		strDirPath = "";
		strName = "";
		return FALSE;
	}
	buf[ret] = '\0';
	ParseFilePath(buf, strDirPath, strName);
	return TRUE;
}

void XFileUtil::ParseFilePath(const string &strFilePath, /*out*/string &strDirPath, /*out*/string &strFileName)
{
	vector<string> vItems;
	string strTmp = strFilePath;
	XStrUtil::Chop(strTmp, "\r\n\t ");
	XStrUtil::Split(strTmp, vItems, "/");

	strDirPath = "";
	strFileName = "";
	if (strTmp.empty()) return;

	if (vItems.size() == 0)
	{
		// such as "/"
		strDirPath = "/";
		return;
	}

	if (vItems.size() == 1)
	{
		if (strTmp[0] == '/')
		{
			// such as "/." or "/.." or "/./"
			if (vItems[0] == "." || vItems[0] == "..")
			{
				strDirPath = "/";
				return;
			}
			else if (strTmp[strTmp.size() - 1] == '/')
			{
				// such as "/root/"
				strDirPath = "/";
				strDirPath += vItems[0];
				return;
			}
			else
			{
				// such as "/tmp" or "/tmp.txt"
				strDirPath = "/";
				strFileName = vItems[0];
				return;
			}
		}
		else
		{
			// such as "." or ".." or "./"
			if (vItems[0] == "." || vItems[0] == "..")
			{
				strDirPath = vItems[0];
				return;
			}
			else if (strTmp[strTmp.size() - 1] == '/')
			{
				// such as "tmp/"
				strDirPath = vItems[0];
				return;
			}
			else
			{
				// such as "tmp" or "tmp.txt"
				strFileName = vItems[0];
				return;
			}
		}
	}

	if (strTmp[strTmp.size() - 1] == '/' || 
		vItems[vItems.size() - 1] == "." || 
		vItems[vItems.size() - 1] == "..")
	{
		// such as "/root/svn/" or "/root/." or "/root/.." or "/root/./"
		for (size_t i = 0; i < vItems.size(); i++)
		{
			strDirPath += '/';
			strDirPath += vItems[i];
		}
		return;
	}
	else
	{
		// such as "/root/svn/tmp" or "/root/tmp.txt"
		for (size_t i = 0; i < vItems.size() - 1; i++)
		{
			strDirPath += '/';
			strDirPath += vItems[i];
		}
		strFileName = vItems[vItems.size() -1];
		return;
	}
	return;
}

void XFileUtil::ParseFileName(const string &strFileName, /*out*/string &strPerfix, /*out*/string &strPostfix)
{
	string strDirPath, strTmp;
	vector<string> vItems;

	strPerfix = "";
	strPostfix = "";
	ParseFilePath(strFileName, strDirPath, strTmp);
	XStrUtil::Chop(strTmp, " ");
	XStrUtil::Split(strTmp, vItems, ".");
	if (vItems.size() == 0) return;
	if (vItems.size() == 1)
	{
		// such as ".tmp" or ".tmp." or "tmp" or "tmp."
		if (strTmp[0] == '.') strPerfix = ".";
		strPerfix += vItems[0];
		return;
	}
	else if (vItems.size() > 2)
	{
		if (strTmp[0] == '.') strPerfix = ".";
		if (strTmp[strTmp.size() - 1] == '.')
		{
			// such as "tmp.txt." or ".tmp.txt."
			for (size_t i = 0; i < vItems.size(); i++)
			{
				strPerfix += vItems[i];
				strPerfix += '.';
			}
			XStrUtil::ChopTail(strPerfix, ".");
			return;
		}
		else
		{
			// such as "tmp.txt" or ".tmp.txt"
			for (size_t i = 0; i < vItems.size() - 1; i++)
			{
				strPerfix += vItems[i];
				strPerfix += '.';
			}
			XStrUtil::ChopTail(strPerfix, ".");
			strPostfix = vItems[vItems.size() - 1];
			return;
		}
	}
	return;
}

// 获取系统目录
string XFileUtil::GetSystemDir()
{
	return "/";
}
#endif//__GNUC__

} // namespace xbase
