#include <locale>
#include <codecvt>
#include <sstream>
#include <filesystem>
#include <array>
#include <algorithm>
#include "utils.h"
#include <windows.h>
#include <Tlhelp32.h>

Utils* utils = new Utils();

DWORD Utils::GetProcessidFromName(LPCWSTR name)
{
	PROCESSENTRY32 pe;
	DWORD id = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
		return 0;
	while (1)
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32Next(hSnapshot, &pe) == FALSE)
			break;
		if (wcscmp(pe.szExeFile, name) == 0)
		{
			id = pe.th32ProcessID;

			break;
		}
	}
	CloseHandle(hSnapshot);
	return id;
}

double Utils::getDpi()
{
	double dDpi = 1;
	// Get desktop dc
	HDC desktopDc = GetDC(NULL);
	// Get native resolution
	float horizontalDPI = GetDeviceCaps(desktopDc, LOGPIXELSX);
	float verticalDPI = GetDeviceCaps(desktopDc, LOGPIXELSY);
	int dpi = (horizontalDPI + verticalDPI) / 2;
	dDpi = 1 + ((dpi - 96) / 24) * 0.25;
	if (dDpi < 1)
	{
		dDpi = 1;
	}
	::ReleaseDC(NULL, desktopDc);
	return dDpi;
}

BOOL  Utils::setNotSmoothState(BOOL isSet) {
	std::string regContent = "0";
	if (!isSet) {
		regContent = "2";
	}
	HKEY hKey;
	CString  strRegPath = _T("Control Panel\\Desktop");

	//1、找到系统的启动项  
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) ///打开启动项       
	{
		DWORD nLength = 4;
		char data[4];
		long result = RegGetValue(hKey, nullptr, _T("FontSmoothing"), RRF_RT_REG_SZ, NULL, &data, &nLength);
		if (result == ERROR_SUCCESS) {
			if (data == regContent) {
				return TRUE;
			}
		}
		SystemParametersInfo(SPI_SETFONTSMOOTHING, !isSet, NULL, SPIF_SENDCHANGE);
		const char* content = regContent.c_str();
		DWORD cbData = 4;
		result = RegSetValueEx(hKey, _T("FontSmoothing"), NULL, RRF_RT_REG_SZ, (LPBYTE)content, cbData);
		if (result == ERROR_SUCCESS) {
			return TRUE;
		}
	}
	return FALSE;
}

 DWORD Utils::read_ini(const TCHAR* app_name, const TCHAR* key_name, const TCHAR* file_name, TCHAR* ret, size_t max_size) {
	return GetPrivateProfileString(app_name, key_name, _T(""), ret, max_size, file_name);
}
 bool Utils::write_ini(const TCHAR* app_name, const TCHAR* key_name, const TCHAR* data, const TCHAR* file_name) {
	return WritePrivateProfileString(app_name, key_name, data, file_name);
}
 DWORD Utils::get_ini_sections(const TCHAR* file_name, TCHAR** ret, int max_count) {
	TCHAR str_buffer[2048];
	DWORD n_count = GetPrivateProfileSectionNames(str_buffer, 2048, file_name);
	int last_pos = 0;
	int last_p_pos = 0;
	int count = 0;
	// 解析数据
	for (DWORD i = 0; i < n_count; i++) {
		if (*(str_buffer + i) == _T('\0')) {
			int size = i - last_pos;
			wmemcpy_s(*(ret + count++), size, str_buffer + last_p_pos, size);
			last_pos = i;
			last_p_pos = i + 1;
			if (count >= max_count) {
				break;
			}
		}
	}
	return count;
}


 std::string Utils::GbkToUtf8(const char* src_str)
 {
	 int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
	 wchar_t* wstr = new wchar_t[len + 1];
	 memset(wstr, 0, len + 1);
	 MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
	 len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	 char* str = new char[len + 1];
	 memset(str, 0, len + 1);
	 WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	 std::string strTemp = str;
	 if (wstr) delete[] wstr;
	 if (str) delete[] str;
	 return strTemp;
 }

 std::string Utils::Utf8ToGbk(const char* src_str)
 {
	 int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
	 wchar_t* wszGBK = new wchar_t[len + 1];
	 memset(wszGBK, 0, len * 2 + 2);
	 MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
	 len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	 char* szGBK = new char[len + 1];
	 memset(szGBK, 0, len + 1);
	 WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	 std::string strTemp(szGBK);
	 if (wszGBK) delete[] wszGBK;
	 if (szGBK) delete[] szGBK;
	 return strTemp;
 }

std::wstring Utils::GetGPathFromReg() {
	 const wchar_t* szPath = L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\英雄联盟";
	 const wchar_t* szName = L"InstallSource";
	 PTCHAR szValue = new wchar_t[256];
	 memset(szValue, 0, 256 * sizeof(wchar_t));
	 DWORD cchSize = (DWORD)256;
	 std::wstring ret = L"";
	 HKEY hKey;
	 if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	 {
		 DWORD dwType = REG_SZ;
		 DWORD cbSize = 0;
		 LONG lRet = RegQueryValueEx(hKey, szName, NULL, &dwType, NULL, &cbSize);
		 if (lRet != ERROR_SUCCESS)
		 {
			 RegCloseKey(hKey);
		 }
		 else {
			 if (cbSize > 0 && cbSize <= cchSize * sizeof(TCHAR))
			 {
				 lRet = RegQueryValueEx(hKey, szName, NULL, &dwType, (LPBYTE)szValue, &cbSize);

			 }
			 RegCloseKey(hKey);
			 if (lRet == ERROR_SUCCESS) {
				 ret += szValue;
				 ret += L"\\TCLS";
				 ret += L"\\Client.exe";
			 }
		 }
	 }
	 delete[] szValue;
	 return ret;
 }

BOOL Utils::open_auto_start() {
	HKEY hKey;
	CString strRegPath = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	CString regName = _T("fdasdxx");
	//1、找到系统的启动项  
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) ///打开启动项       
	{
		//2、得到本程序自身的全路径
		CString path;
		TCHAR exeFullPath[MAX_PATH]; // Full path
		GetModuleFileName(NULL, exeFullPath, MAX_PATH);
		path = CString(exeFullPath);
		path += _T(" START");
		//3、判断注册表项是否已经存在
		TCHAR strDir[MAX_PATH] = {};
		DWORD nLength = MAX_PATH;
		long result = RegGetValue(hKey, nullptr, regName, RRF_RT_REG_SZ, 0, strDir, &nLength);
		//4、已经存在
		if (result != ERROR_SUCCESS || wcscmp(path, strDir) != 0)
		{
			//5、添加一个子Key,并设置值,"GISRestart"是应用程序名字（不加后缀.exe） 
			RegSetValueExW(hKey, regName, 0, REG_SZ, (BYTE*)path.GetBuffer(), path.GetLength() * 2);
			//6、关闭注册表
			RegCloseKey(hKey);
		}
		return TRUE;
	}
	else
	{
		::MessageBox(0, _T("开机自启设置失败"), _T(""), MB_OK);
		return FALSE;
	}
}
void Utils::close_auto_start() {
	HKEY hKey;
	CString strRegPath = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	CString regName = _T("fdasdxx");
	//1、找到系统的启动项  
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		//2、删除值
		RegDeleteValue(hKey, regName);

		//3、关闭注册表
		RegCloseKey(hKey);
	}
}
BOOL Utils::isSetStartSelf() {
	CString regName = _T("fdasdxx");
	HKEY hKey;
	CString strRegPath = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");

	//1、找到系统的启动项  
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		TCHAR strDir[MAX_PATH] = {};
		DWORD nLength = MAX_PATH;
		long result = RegGetValue(hKey, nullptr, regName, RRF_RT_REG_SZ, 0, strDir, &nLength);
		if (result == ERROR_SUCCESS) {
			return TRUE;
		}
	}
	return FALSE;
}

std::string Utils::WstringToString(std::wstring wstr)
{
	int len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)(LPCTSTR)wstr.c_str(), -1, NULL, 0, NULL, NULL);
	char* str = new char[len];
	memset(str, 0, len);
	WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)(LPCTSTR)wstr.c_str(), -1, str, len, NULL, NULL);
	std::string cstrDestA = str;
	delete[] str;

	return cstrDestA;
}

std::wstring Utils::StringToWstring(std::string str)
{
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len];
	memset(wstr, 0, len * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wstr, len);
	std::wstring cstrDest = wstr;
	delete[] wstr;
	return cstrDest;
}

int Utils::rand_(int v_min, int v_max)
{
	int rNum = 0;
	srand(GetTickCount());
	for (int i = 0; i < 31; i++)
		rNum |= (rand() & 1) << i;
	return v_min + rNum % (v_max - v_min + 1);
}

std::string Utils::RandomString(int size)
{
	std::string str = "";

	for (int i = 0; i < size; i++)
		str += rand_(0, 1) ? rand_(48, 57) : rand_(97, 122);

	return str;
}

std::string Utils::FormatString(const char* c, const char* args...)
{
	char buff[200];
	sprintf_s(buff, c, args);

	return std::string(buff);
}

std::string Utils::ToLower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::tolower);
	return str;
}

std::wstring Utils::ToLower(std::wstring str)
{
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::tolower);
	return str;
}

std::string Utils::ToUpper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::toupper);
	return str;
}

bool Utils::StringContains(std::string strA, std::string strB, bool ignore_case)
{
	if (strA.empty() || strB.empty())
		return true;

	if (ignore_case)
	{
		strA = ToLower(strA);
		strB = ToLower(strB);
	}

	if (strA.find(strB) != std::string::npos)
		return true;

	return false;
}

bool Utils::StringContains(std::wstring strA, std::wstring strB, bool ignore_case)
{
	if (strA.empty() || strB.empty())
		return true;

	if (ignore_case)
	{
		strA = ToLower(strA);
		strB = ToLower(strB);
	}

	if (strA.find(strB) != std::wstring::npos)
		return true;

	return false;
}

void Utils::split(const std::string& s, std::vector<std::string>& tokens, char delim) {
	tokens.clear();
	auto string_find_first_not = [s, delim](size_t pos = 0) -> size_t {
		for (size_t i = pos; i < s.size(); i++) {
			if (s[i] != delim) return i;
		}
		return std::string::npos;
	};
	size_t lastPos = string_find_first_not(0);
	size_t pos = s.find(delim, lastPos);
	while (lastPos != std::string::npos) {
		tokens.emplace_back(s.substr(lastPos, pos - lastPos));
		lastPos = string_find_first_not(pos);
		pos = s.find(delim, lastPos);
	}
}

void Utils::wsplit(const std::wstring& s, std::vector<std::wstring>& tokens, wchar_t delim) {
	tokens.clear();
	auto string_find_first_not = [s, delim](size_t pos = 0) -> size_t {
		for (size_t i = pos; i < s.size(); i++) {
			if (s[i] != delim) return i;
		}
		return std::wstring::npos;
	};
	size_t lastPos = string_find_first_not(0);
	size_t pos = s.find(delim, lastPos);
	while (lastPos != std::wstring::npos) {
		tokens.emplace_back(s.substr(lastPos, pos - lastPos));
		lastPos = string_find_first_not(pos);
		pos = s.find(delim, lastPos);
	}
}

unsigned char CharToHex(unsigned char x) {
	return (unsigned char)(x > 9 ? x + 55 : x + 48);
}

bool IsAlphaNumber(unsigned char c) {
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
		return true;
	return false;
}
void Utils::UrlEncode(const std::string& src, std::string& str_encode) {
	unsigned char* p = (unsigned char*)src.c_str();
	unsigned char ch;
	while (*p) {
		ch = (unsigned char)*p;
		if (*p == ' ') {
			str_encode += '+';
		}
		else if (IsAlphaNumber(ch) || strchr("-_.~!*'();:@&=+$,/?#[]", ch)) {
			str_encode += *p;
		}
		else {
			str_encode += '%';
			str_encode += CharToHex((unsigned char)(ch >> 4));
			str_encode += CharToHex((unsigned char)(ch % 16));
		}
		++p;
	}
}

std::string getLevelStr(int level)
{
	std::string levelStr = "";
	for (int i = 0; i < level; i++)
	{
		levelStr += "\t"; //这里可以\t换成你所需要缩进的空格数
	}
	return levelStr;

}
std::string Utils::formatJson(const std::string& json)
{
	std::string result = "";
	int level = 0;
	for (std::string::size_type index = 0; index < json.size(); index++)
	{
		char c = json[index];

		if (level > 0 && '\n' == json[json.size() - 1])
		{
			result += getLevelStr(level);
		}

		switch (c)
		{
		case '{':
		case '[':
			result = result + c + "\n";
			level++;
			result += getLevelStr(level);
			break;
		case ',':
			result = result + c + "\n";
			result += getLevelStr(level);
			break;
		case '}':
		case ']':
			result += "\n";
			level--;
			result += getLevelStr(level);
			result += c;
			break;
		default:
			result += c;
			break;
		}

	}
	return result;
}