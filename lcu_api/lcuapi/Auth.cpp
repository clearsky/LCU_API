#include "Auth.h"
#include "utils/utils.h"
#include "NtQueryInfoProc.h"

bool Auth::GetRiotClientInfo()
{
	std::string sAuth = utils->WstringToString(GetProcessCommandLine("RiotClientUx.exe"));
	if (sAuth.empty())
	{
		//MessageBoxA(0, "Client not found", 0, 0);
		return 0;
	}
	std::string appPort = "--app-port=";
	size_t nPos = sAuth.find(appPort);
	if (nPos != std::string::npos)
		riotPort = std::stoi(sAuth.substr(nPos + appPort.size(), 5)); // port is always 5 numbers long
	std::string remotingAuth = "--remoting-auth-token=";
	nPos = sAuth.find(remotingAuth) + strlen(remotingAuth.c_str());
	if (nPos != std::string::npos)
	{
		std::string token = "riot:" + sAuth.substr(nPos, 22); // token is always 22 chars long
		unsigned char m_Test[50];
		strncpy((char*)m_Test, token.c_str(), sizeof(m_Test));
		riotToken = base64.Encode(m_Test, token.size()).c_str();
	}
	else
	{
		MessageBoxA(0, "Couldn't connect to client", 0, 0);

		return 0;
	}
	MakeRiotHeader();
	return 1;
}

void Auth::MakeRiotHeader()
{
	riotHeader = "Host: 127.0.0.1:" + std::to_string(riotPort) + "\r\n" +
		"Connection: keep-alive" + "\r\n" +
		"Authorization: Basic " + riotToken + "\r\n" +
		"Accept: application/json" + "\r\n" +
		"Access-Control-Allow-Credentials: true" + "\r\n" +
		"Access-Control-Allow-Origin: 127.0.0.1" + "\r\n" +
		"Content-Type: application/json" + "\r\n" +
		"Origin: https://127.0.0.1:" + std::to_string(riotPort) + "\r\n" +
		"User-Agent: Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) RiotClient/34.0.2 (CEF 74) Safari/537.36" + "\r\n" +
		"Referer: https://127.0.0.1:" + std::to_string(riotPort) + "/index.html" + "\r\n" +
		"Accept-Encoding: gzip, deflate, br" + "\r\n" +
		"Accept-Language: en-US,en;q=0.9";
}

bool  Auth::GetLeagueClientInfo()
{
	// Get client port and auth code from it's command line
	std::string sAuth = utils->WstringToString(GetProcessCommandLine("LeagueClientUx.exe"));
	if (sAuth.empty())
	{
		//MessageBoxA(0, "Client not found", 0, 0);
		if (successed) {
			return 1;
		}
		return 0;
	}
	std::string appPort = "\"--app-port=";
	size_t nPos = sAuth.find(appPort);
	if (nPos != std::string::npos)
		leaguePort = std::stoi(sAuth.substr(nPos + appPort.size(), 5));
	
	std::string remotingAuth = "--remoting-auth-token=";
	nPos = sAuth.find(remotingAuth) + strlen(remotingAuth.c_str());
	if (nPos != std::string::npos)
	{
		std::string token = "riot:" + sAuth.substr(nPos, 22);
		printf("%s\n", token.c_str());
		unsigned char m_Test[50];
		strncpy((char*)m_Test, token.c_str(), sizeof(m_Test));
		leagueToken = base64.Encode(m_Test, token.size()).c_str();
	}
	else
	{
		MessageBoxA(0, "Couldn't connect to client", 0, 0);

		return 0;
	}
	
	MakeLeagueHeader();
	
	successed = true;

	return 1;
}

void Auth::MakeLeagueHeader()
{
	leagueHeader = "Host: 127.0.0.1:" + std::to_string(leaguePort) + "\n" +
		"Connection: keep-alive" + "\n" +
		"Authorization: Basic " + leagueToken + "\n" +
		"Accept: application/json" + "\n" +
		"Content-Type: application/json" + "\n" +
		"Origin: https://127.0.0.1:" + std::to_string(leaguePort) + "\n" +
		"User-Agent: Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) LeagueOfLegendsClient/11.13.382.1241 (CEF 74) Safari/537.36" + "\n" +
		"X-Riot-Source: rcp-fe-lol-social" + "\n" +
		"Referer: https://127.0.0.1:" + std::to_string(leaguePort) + "/index.html" + "\n" +
		"Accept-Encoding: gzip, deflate, br" + "\n" +
		"Accept-Language: zh-CN,zh;q=0.9";
}

std::wstring Auth::GetProcessCommandLine(std::string sProcessName)
{
	std::wstring wstrResult;
	HANDLE Handle;
	DWORD ProcessID = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(snapshot, &entry))
		{
			do
			{
				char temp[260];
				sprintf(temp, "%ws", entry.szExeFile);
				if (!stricmp(temp, sProcessName.c_str()))
				{
					ProcessID = entry.th32ProcessID;
					Handle = OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);

					PROCESS_BASIC_INFORMATION pbi;
					PEB peb = { 0 };
					tNtQueryInformationProcess NtQueryInformationProcess =
						(tNtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
					NTSTATUS status = NtQueryInformationProcess(Handle, ProcessBasicInformation, &pbi, sizeof(pbi), 0);

					if (NT_SUCCESS(status))
					{
						ReadProcessMemory(Handle, pbi.PebBaseAddress, &peb, sizeof(peb), 0);
					}
					PRTL_USER_PROCESS_PARAMETERS pRtlProcParam = peb.ProcessParameters;
					PRTL_USER_PROCESS_PARAMETERS pRtlProcParamCopy =
						(PRTL_USER_PROCESS_PARAMETERS)malloc(sizeof(RTL_USER_PROCESS_PARAMETERS));

					bool result = ReadProcessMemory(Handle,
						pRtlProcParam,
						pRtlProcParamCopy,
						sizeof(RTL_USER_PROCESS_PARAMETERS),
						NULL);
					PWSTR wBuffer = pRtlProcParamCopy->CommandLine.Buffer;
					USHORT len = pRtlProcParamCopy->CommandLine.Length;
					PWSTR wBufferCopy = (PWSTR)malloc(len);
					result = ReadProcessMemory(Handle,
						wBuffer,
						wBufferCopy,
						len,
						NULL);

					wstrResult = std::wstring(wBufferCopy);

					CloseHandle(Handle);
					break;
				}
			} while (Process32Next(snapshot, &entry));
		}
	}
	CloseHandle(snapshot);
	return wstrResult;
}

Auth* auth = new Auth();