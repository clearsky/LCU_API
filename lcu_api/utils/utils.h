#pragma once
#include <windows.h>
#include <atlstr.h>
#include <vector>
#include <chrono>
#include <string>

class Utils
{
private:

public:
	Utils(){}
	~Utils() = default;
public:
	double getDpi();
	BOOL  setNotSmoothState(BOOL isSet);
	DWORD read_ini(const TCHAR* app_name, const TCHAR* key_name, const TCHAR* file_name, TCHAR* ret, size_t max_size);
	bool write_ini(const TCHAR* app_name, const TCHAR* key_name, const TCHAR* data, const TCHAR* file_name);
	DWORD get_ini_sections(const TCHAR* file_name, TCHAR** ret, int max_count);
	std::wstring GetGPathFromReg();
	BOOL open_auto_start();
	void close_auto_start();
	BOOL isSetStartSelf();
	std::wstring StringToWstring(std::string str);
	std::string WstringToString(std::wstring wstr);
	std::string RandomString(int size);
	std::string FormatString(const char* c, const char* args...);
	//converts whole string to lowercase
	std::string ToLower(std::string str);
	std::wstring ToLower(std::wstring str);

	//converts whole string to uppercase
	std::string ToUpper(std::string str);

	//check if strA contains strB
	bool StringContains(std::string strA, std::string strB, bool ignore_case = false);
	bool StringContains(std::wstring strA, std::wstring strB, bool ignore_case = false);

	void split(const std::string& s, std::vector<std::string>& tokens, char delim = ' ');
	void wsplit(const std::wstring& s, std::vector<std::wstring>& tokens, wchar_t delim = _T(' '));
	int rand_(int v_min, int v_max);

	void UrlEncode(const std::string& src, std::string& str_encode);
	std::string GbkToUtf8(const char* src_str);
	std::string Utf8ToGbk(const char* src_str);
	std::string formatJson(const std::string& json);
	DWORD GetProcessidFromName(LPCWSTR name);
};

extern Utils* utils;